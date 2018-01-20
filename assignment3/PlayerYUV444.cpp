
#include <opencv2/opencv.hpp>
#include <iterator>
#include <vector>
using namespace cv;
using namespace std;

/*static void drawOptFlowMap(const Mat& flow, Mat& cflowmap, int step,
                           double, const Scalar& color)
{
    for(int y = 0; y < cflowmap.rows; y += step)
        for(int x = 0; x < cflowmap.cols; x += step)
        {
            const Point2f& fxy = flow.at<Point2f>(y, x);
            line(cflowmap, Point(x,y), Point(cvRound(x+fxy.x),
                                             cvRound(y+fxy.y)), color);
            circle(cflowmap, Point(x,y), 2, color, -1);
        }
}*/



unsigned int bitsinbuffer = 0;
unsigned int buffer = 0;
vector<int> res;

int buildOnes(int n) {
    unsigned int tmp = ~0;
    return (tmp >> (32-n));
}


void flush(ofstream& file) {
    char towrite[1];
    while (bitsinbuffer >= 8) {
        towrite[0] = (char) ((buffer >> (bitsinbuffer - 8)) & 0xFF);
        //towrite[0] = (char) 0xBA;
        //if(ctr > 0) cout << "Writing: " << hex << (int) towrite[0] << dec << endl;
        //if(ctr > 0) cout << "Buffer: " << hex << buffer << dec << " (" << bitsinbuffer << ")" << endl;
        file.write(towrite, 1);
        bitsinbuffer -= 8;
        buffer = buffer & buildOnes(bitsinbuffer);

    }
}



void forceFlush(ofstream& file) {
    char towrite[1];
    while (bitsinbuffer >= 8) {
        towrite[0] = (char) ((buffer >> (bitsinbuffer - 8)) & 0xFF);
        file.write(towrite, 1);
        bitsinbuffer -= 8;
        buffer = buffer & buildOnes(bitsinbuffer);
    }
    if (bitsinbuffer > 0) {
        towrite[0] = (char) ((buffer << (8 - bitsinbuffer)) & 0xFF);
        file.write(towrite, 1);
        buffer = 0;
        bitsinbuffer = 0;
    }

}

void writeToFile(int data, int length, char type, ofstream& file) {

    if (type == 'q') {
        int length_to_go = length;
        while(length_to_go >= 8) {
            buffer = (buffer << 8) | 0xFF;
            bitsinbuffer += 8;
            flush(file);
            length_to_go -= 8;
        }
        if(length_to_go > 0) {
            buffer = ((buffer << length_to_go) | ((0x00FF >> (8 - length_to_go)) & 0xFF));
            bitsinbuffer += length_to_go;
            flush(file);
        }
        buffer = buffer << 1;
        bitsinbuffer += 1;
        flush(file);
    }
    else if (type == 'r') {
        buffer = (buffer << length) | (data & buildOnes(length));
        bitsinbuffer += length;
        flush(file);
    }
    else if (type == 'p') {
        buffer = (buffer << length) | (data & 0x03);
        bitsinbuffer += length;
        flush(file);
    }
    else if (type == 'm') {
        buffer = (buffer << length) | (data & 0xFF);
        bitsinbuffer += length;
        flush(file);
    }
    else if (type == 'f') {
        forceFlush(file);
    }
}

void encodeGolombToFile(vector<int>& input, ofstream& outfile) {
    vector<int>::iterator it;
    int target;
    int q, r;
    int m = 128;
    int nbits = (int) log2(m);
    //long estsize = 0;
    //cout << "R values: ";
    int i = 0;
    for(it = input.begin(); it != input.end(); it++, i++) {

        if(*it < 0) target = -(1 + *it * 2);
        else target = 2 * *it;


        q = target/m;

        writeToFile(q, q, 'q', outfile);
        //estsize += (q+1);

        r = target - q * m;

        //if(blockcounter == 52) cout << predictors.at(i/bs) << " ";

        writeToFile(r, nbits, 'r', outfile);

        //estsize += nbits;
    }
    //cout << endl;

    writeToFile(0, 0, 'f', outfile);
    //cout << "Wrote a total of " << partition_index << " partitions.\n";
    //cout << "Estimated size: " << estsize/8 << endl;
}


int rescounter = 0;


int main(int argc, char** argv)
{
    string line; // store the header
    int chroma;
    int yCols, yRows; /* frame dimension */
    int fps = 30; /* frames per second */
    int i, n, r, g, b, y, u, v; /* auxiliary variables */
    int yindex, uindex, vindex; /* indexes */
    unsigned char *imgData, *predicted, *residues, *decoded, *decodedrgb; // file data buffer
    uchar *buffer; // unsigned char pointer to the Mat data
    char inputKey = '?'; /* parse the pressed key */
    int end = 0, playing = 1, loop = 0, encode = 1, output = 1, playfile = 0; /* control variables */

    /* check for the mandatory arguments */
    if( argc < 2 ) {
        cerr << "Usage: PlayerYUV444 filename [-playfile]" << endl;
        return 1;
    }

    /* Opening video file */
    ifstream myfile (argv[1]);

    /* Processing header */
    getline (myfile,line);
    cout << line << endl;
    //cout << "'" << line.substr(line.find(" W") + 2, line.find(" H") - line.find(" W") - 2) << "'" << endl;
    yCols = stoi(line.substr(line.find(" W") + 2, line.find(" H") - line.find(" W") - 2));
    yRows = stoi(line.substr(line.find(" H") + 2, line.find(" F") - line.find(" H") - 2));
    try{
        chroma = stoi(line.substr(line.find(" C") + 2 , line.length()));
    }
    catch (std::invalid_argument){
        chroma = 420;
    }
    double multiplier;
    switch(chroma) {
        case 444:
            multiplier = 3;
            break;
        case 422:
            multiplier = 2;
            break;
        default:
            multiplier = 1.5;
    }
    //cout << yCols << ", " << yRows << endl;

    /* Parse other command line arguments */
    for(n = 1 ; n < argc ; n++)
    {
        if(!strcmp("-fps", argv[n]))
        {
            fps = atof(argv[n+1]);
            n++;
        }

        if(!strcmp("-wait", argv[n]))
        {
            playing = 0;
        }

        if(!strcmp("-l", argv[n]))
        {
            loop = 1;
        }

        if(!strcmp("-playfile", argv[n]))
        {
            playfile = 1;
        }
    }

    /* data structure for the OpenCv image */
    Mat img = Mat(Size(yCols, yRows), CV_8UC3);

    Mat imgyuv;
    Mat predictorimg = Mat(Size(yCols, yRows*3), CV_8UC1);

    Mat residuesimg = Mat(Size(yCols, yRows*3), CV_8UC1);

    Mat decodedimg = Mat(Size(yCols, yRows*3), CV_8UC1);

    Mat decodedimgrgb = Mat(Size(yCols, yRows), CV_8UC3);

    /* buffer to store the frame */
    imgData = new unsigned char[yCols * yRows * 3];

    /* create a window */
    cvNamedWindow( "rgb", CV_WINDOW_AUTOSIZE );


    // Optical Flow stuff
    Mat flow, cflow, frame;
    UMat gray, prevgray, uflow;
    namedWindow("flow", 1);
    int nframes = 0;
    while(!end) {
        nframes++;
        /* load a new frame, if possible */
        getline(myfile, line); // Skipping word FRAME
        myfile.read((char *) imgData, yCols * yRows * multiplier);

        if (myfile.gcount() == 0) {
            if (loop) {
                myfile.clear();
                myfile.seekg(0);
                getline(myfile, line); // read the header
                continue;
            } else {
                end = 1;
                break;
            }
        }

        /* The video is stored in YUV planar mode but OpenCv uses packed modes*/
        buffer = (uchar *) img.ptr();
        residues = (uchar *) residuesimg.ptr();
        decoded = (uchar *) decodedimg.ptr();
        decodedrgb = (uchar *) decodedimgrgb.ptr();

        switch (chroma) {
            case 444:
                for (i = 0; i < yRows * yCols * 3; i += 3) {
                    /* Accessing to planar info */
                    y = imgData[i / 3];
                    u = imgData[(i / 3) + (yRows * yCols)];
                    v = imgData[(i / 3) + (yRows * yCols) * 2];

                    /* convert to RGB */
                    b = (int) (1.164 * (y - 16) + 2.018 * (u - 128));
                    g = (int) (1.164 * (y - 16) - 0.813 * (u - 128) - 0.391 * (v - 128));
                    r = (int) (1.164 * (y - 16) + 1.596 * (v - 128));

                    /* clipping to [0 ... 255] */
                    if (r < 0) r = 0;
                    if (g < 0) g = 0;
                    if (b < 0) b = 0;
                    if (r > 255) r = 255;
                    if (g > 255) g = 255;
                    if (b > 255) b = 255;

                    /* if you need the inverse formulas */
                    //y = r *  .299 + g *  .587 + b *  .114 ;
                    //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                    //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                    /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                    buffer[i] = b;
                    buffer[i + 1] = g;
                    buffer[i + 2] = r;

                }

                if (encode) {
                    predicted = (uchar *) predictorimg.ptr();

                    for (i = 0; i < yRows * yCols; i += 1) {

                        /* Accessing to planar info */
                        yindex = i;
                        uindex = i + (yRows * yCols);
                        vindex = i + (yRows * yCols) * 2;

                        if (i < yCols || !(i % yCols)) {
                            predicted[yindex] = imgData[yindex];
                            predicted[uindex] = imgData[uindex];
                            predicted[vindex] = imgData[vindex];
                        } else {
                            uchar min, max;

                            //Y
                            if (imgData[yindex - 1] < imgData[yindex - yCols]) { // if a < b
                                min = imgData[yindex - 1];
                                max = imgData[yindex - yCols];
                            } else { // if a > b
                                min = imgData[yindex - yCols];
                                max = imgData[yindex - 1];
                            }
                            if (imgData[yindex - yCols - 1] >= max) { // c >= max
                                predicted[yindex] = min;
                            } else if (imgData[yindex - yCols - 1] <= min) { //c <= min
                                predicted[yindex] = max;
                            } else {
                                predicted[yindex] = imgData[yindex - 1] + imgData[yindex - yCols] -
                                                    imgData[yindex - yCols - 1]; // x = a + b - c
                            }

                            //U
                            if (imgData[uindex - 1] < imgData[uindex - yCols]) { // if a < b
                                min = imgData[uindex - 1];
                                max = imgData[uindex - yCols];
                            } else { // if a > b
                                min = imgData[uindex - yCols];
                                max = imgData[uindex - 1];
                            }
                            if (imgData[uindex - yCols - 1] >= max) { // c >= max
                                predicted[uindex] = min;
                            } else if (imgData[uindex - yCols - 1] <= min) { //c <= min
                                predicted[uindex] = max;
                            } else {
                                predicted[uindex] = imgData[uindex - 1] + imgData[uindex - yCols] -
                                                    imgData[uindex - yCols - 1]; // x = a + b - c
                            }

                            //V
                            if (imgData[vindex - 1] < imgData[vindex - yCols]) { // if a < b
                                min = imgData[vindex - 1];
                                max = imgData[vindex - yCols];
                            } else { // if a > b
                                min = imgData[vindex - yCols];
                                max = imgData[vindex - 1];
                            }
                            if (imgData[vindex - yCols - 1] >= max) { // c >= max
                                predicted[vindex] = min;
                            } else if (imgData[vindex - yCols - 1] <= min) { //c <= min
                                predicted[vindex] = max;
                            } else {
                                predicted[vindex] = imgData[vindex - 1] + imgData[vindex - yCols] -
                                                    imgData[vindex - yCols - 1]; // x = a + b - c
                            }

                        }

                    }

                    for (i = 0; i < yRows * yCols; i += 1) { // RESIDUES = ORIGINAL - PREDICTED

                        /* Accessing to planar info */
                        yindex = i;
                        uindex = i + (yRows * yCols);
                        vindex = i + (yRows * yCols) * 2;

                        if (i < yCols || !(i % yCols)) {
                            residues[yindex] = imgData[yindex];
                            residues[uindex] = imgData[uindex];
                            residues[vindex] = imgData[vindex];
                        } else {
                            residues[yindex] = imgData[yindex] - predicted[yindex];
                            residues[uindex] = imgData[uindex] - predicted[uindex];
                            residues[vindex] = imgData[vindex] - predicted[vindex];
                        }

                    }

                    for (i = 0; i < yRows * yCols; i += 1) { // DECODING

                        /* Accessing to planar info */
                        yindex = i;
                        uindex = i + (yRows * yCols);
                        vindex = i + (yRows * yCols) * 2;

                        if (i < yCols || !(i % yCols)) {
                            decoded[yindex] = residues[yindex];
                            decoded[uindex] = residues[uindex];
                            decoded[vindex] = residues[vindex];
                        } else {
                            uchar min, max;

                            //Y
                            if (decoded[yindex - 1] < decoded[yindex - yCols]) { // if a < b
                                min = decoded[yindex - 1];
                                max = decoded[yindex - yCols];
                            } else { // if a > b
                                min = decoded[yindex - yCols];
                                max = decoded[yindex - 1];
                            }

                            if (decoded[yindex - yCols - 1] >= max) { // c >= max
                                decoded[yindex] = min;
                            } else if (decoded[yindex - yCols - 1] <= min) { //c <= min
                                decoded[yindex] = max;
                            } else {
                                decoded[yindex] = decoded[yindex - 1] + decoded[yindex - yCols] -
                                                  decoded[yindex - yCols - 1]; // x = a + b - c
                            }

                            decoded[yindex] += residues[yindex];

                            //U
                            if (decoded[uindex - 1] < decoded[uindex - yCols]) { // if a < b
                                min = decoded[uindex - 1];
                                max = decoded[uindex - yCols];
                            } else { // if a > b
                                min = decoded[uindex - yCols];
                                max = decoded[uindex - 1];
                            }

                            if (decoded[uindex - yCols - 1] >= max) { // c >= max
                                decoded[uindex] = min;
                            } else if (decoded[uindex - yCols - 1] <= min) { //c <= min
                                decoded[uindex] = max;
                            } else {
                                decoded[uindex] = decoded[uindex - 1] + decoded[uindex - yCols] -
                                                  decoded[uindex - yCols - 1]; // x = a + b - c
                            }

                            decoded[uindex] += residues[uindex];

                            //V
                            if (decoded[vindex - 1] < decoded[vindex - yCols]) { // if a < b
                                min = decoded[vindex - 1];
                                max = decoded[vindex - yCols];
                            } else { // if a > b
                                min = decoded[vindex - yCols];
                                max = decoded[vindex - 1];
                            }

                            if (decoded[vindex - yCols - 1] >= max) { // c >= max
                                decoded[vindex] = min;
                            } else if (decoded[vindex - yCols - 1] <= min) { //c <= min
                                decoded[vindex] = max;
                            } else {
                                decoded[vindex] = decoded[vindex - 1] + decoded[vindex - yCols] -
                                                  decoded[vindex - yCols - 1]; // x = a + b - c
                            }

                            decoded[vindex] += residues[vindex];
                        }

                    }

                    for (i = 0; i < yRows * yCols * 3; i += 3) {
                        /* Accessing to planar info */
                        y = decoded[i / 3];
                        u = decoded[(i / 3) + (yRows * yCols)];
                        v = decoded[(i / 3) + (yRows * yCols) * 2];

                        /* convert to RGB */
                        b = (int) (1.164 * (y - 16) + 2.018 * (u - 128));
                        g = (int) (1.164 * (y - 16) - 0.813 * (u - 128) - 0.391 * (v - 128));
                        r = (int) (1.164 * (y - 16) + 1.596 * (v - 128));

                        /* clipping to [0 ... 255] */
                        if (r < 0) r = 0;
                        if (g < 0) g = 0;
                        if (b < 0) b = 0;
                        if (r > 255) r = 255;
                        if (g > 255) g = 255;
                        if (b > 255) b = 255;

                        /* if you need the inverse formulas */
                        //y = r *  .299 + g *  .587 + b *  .114 ;
                        //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                        //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                        /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                        decodedrgb[i] = b;
                        decodedrgb[i + 1] = g;
                        decodedrgb[i + 2] = r;

                    }

                }


                break;
            case 422:
                for (i = 0; i < yRows * yCols; i += 1) {
                    /* Accessing to planar info */
                    y = imgData[i];
                    u = imgData[(yRows * yCols) + (i / 2)];
                    v = imgData[(yRows * yCols) * 3 / 2 + (i / 2)];

                    /* convert to RGB*/
                    r = (int) (y + 1.28033 * (v - 128));
                    g = (int) (y - 0.21482 * (u - 128) - 0.38059 * (v - 128));
                    b = (int) (y + 2.12798 * (u - 128));

                    //r = y + 1.28033*v;
                    //g = y - 0.21482*u - 0.38059*v;
                    //b = y + 2.12798*u;

                    /* clipping to [0 ... 255] */
                    if (r < 0) r = 0;
                    if (g < 0) g = 0;
                    if (b < 0) b = 0;
                    if (r > 255) r = 255;
                    if (g > 255) g = 255;
                    if (b > 255) b = 255;

                    /* if you need the inverse formulas */
                    //y = r *  .299 + g *  .587 + b *  .114 ;
                    //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                    //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                    /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                    buffer[i * 3] = b;
                    buffer[i * 3 + 1] = g;
                    buffer[i * 3 + 2] = r;
                }

                if (encode) {
                    predicted = (uchar *) predictorimg.ptr();

                    for (i = 0; i < yRows * yCols; i += 1) {

                        /* Accessing to planar info */
                        yindex = i;
                        uindex = (yRows * yCols) + (i / 2);
                        vindex = (yRows * yCols) * 3 / 2 + (i / 2);

                        uchar min, max;
                        uchar a, b, c;
                        //Y

                        a = imgData[yindex - 1];
                        b = imgData[yindex - yCols];
                        c = imgData[yindex - yCols - 1];

                        if (a < b) { // if a < b
                            min = a;
                            max = b;
                        } else { // if a > b
                            min = b;
                            max = a;
                        }
                        if (c >= max) { // c >= max
                            predicted[yindex] = min;
                        } else if (c <= min) { //c <= min
                            predicted[yindex] = max;
                        } else {
                            predicted[yindex] = a + b - c; // x = a + b - c
                        }

                        //U

                        a = imgData[uindex - 1];
                        b = imgData[uindex - yCols];
                        c = imgData[uindex - yCols - 1];

                        if (a < b) { // if a < b
                            min = a;
                            max = b;
                        } else { // if a > b
                            min = b;
                            max = a;
                        }
                        if (c >= max) { // c >= max
                            predicted[uindex] = min;
                        } else if (c <= min) { //c <= min
                            predicted[uindex] = max;
                        } else {
                            predicted[uindex] = a + b - c; // x = a + b - c
                        }

                        //V

                        a = imgData[vindex - 1];
                        b = imgData[vindex - yCols];
                        c = imgData[vindex - yCols - 1];

                        if (a < b) { // if a < b
                            min = a;
                            max = b;
                        } else { // if a > b
                            min = b;
                            max = a;
                        }
                        if (c >= max) { // c >= max
                            predicted[vindex] = min;
                        } else if (c <= min) { //c <= min
                            predicted[vindex] = max;
                        } else {
                            predicted[vindex] = a + b - c; // x = a + b - c
                        }

                    }

                    for (i = 0; i < yRows * yCols; i += 1) {

                        /* Accessing to planar info */
                        yindex = i;
                        uindex = (yRows * yCols) + (i / 2);
                        vindex = (yRows * yCols) * 3 / 2 + (i / 2);
                        bool yskip, uskip, vskip;
                        yskip = uskip = vskip = false;

                        if (yindex < yCols || !(yindex % yCols)) {
                            residues[yindex] = imgData[yindex];
                            yskip = true;
                        }
                        if (uindex - yRows * yCols < yCols || !(uindex % yCols)) {
                            residues[uindex] = imgData[uindex];
                            uskip = true;
                        }
                        if (vindex - yRows * yCols * 3 / 2 < yCols || !(vindex % yCols)) {
                            residues[vindex] = imgData[vindex];
                            vskip = true;
                        }

                        if (!yskip)
                            residues[yindex] = imgData[yindex] - predicted[yindex];

                        if (!uskip)
                            residues[uindex] = imgData[uindex] - predicted[uindex];

                        if (!vskip)
                            residues[vindex] = imgData[vindex] - predicted[vindex];

                    }

                    for (i = 0; i < yRows * yCols; i += 1) { // DECODING

                        /* Accessing to planar info */
                        yindex = i;
                        uindex = (yRows * yCols) + (i / 2);
                        vindex = (yRows * yCols) * 3 / 2 + (i / 2);

                        bool yskip, uskip, vskip;
                        yskip = uskip = vskip = false;

                        if (yindex < yCols || !(yindex % yCols)) {
                            decoded[yindex] = residues[yindex];
                            yskip = true;
                        }
                        if (uindex - yRows * yCols < yCols || !(uindex % yCols)) { // i/2
                            decoded[uindex] = residues[uindex];
                            uskip = true;
                        }
                        if (vindex - yRows * yCols * 3 / 2 < yCols || !(vindex % yCols)) {  // i/2
                            decoded[vindex] = residues[vindex];
                            vskip = true;
                        }

                        uchar min, max;
                        uchar a, b, c;

                        //Y
                        if (!yskip) {
                            a = decoded[yindex - 1];
                            b = decoded[yindex - yCols];
                            c = decoded[yindex - yCols - 1];

                            if (a < b) { // if a < b
                                min = a;
                                max = b;
                            } else { // if a > b
                                min = b;
                                max = a;
                            }

                            if (c >= max) { // c >= max
                                decoded[yindex] = min;
                            } else if (c <= min) { //c <= min
                                decoded[yindex] = max;
                            } else {
                                decoded[yindex] = a + b - c; // x = a + b - c
                            }

                            decoded[yindex] += residues[yindex];
                        }


                        //U

                        if (!uskip) {
                            a = decoded[uindex - 1];
                            b = decoded[uindex - yCols];
                            c = decoded[uindex - yCols - 1];

                            if (a < b) { // if a < b
                                min = a;
                                max = b;
                            } else { // if a > b
                                min = b;
                                max = a;
                            }

                            if (c >= max) { // c >= max
                                decoded[uindex] = min;
                            } else if (c <= min) { //c <= min
                                decoded[uindex] = max;
                            } else {
                                decoded[uindex] = a + b - c; // x = a + b - c
                            }

                            decoded[uindex] += residues[uindex];

                        }


                        //V

                        if (!vskip) {
                            a = decoded[vindex - 1];
                            b = decoded[vindex - yCols];
                            c = decoded[vindex - yCols - 1];

                            if (a < b) { // if a < b
                                min = a;
                                max = b;
                            } else { // if a > b
                                min = b;
                                max = a;
                            }

                            if (c >= max) { // c >= max
                                decoded[vindex] = min;
                            } else if (c <= min) { //c <= min
                                decoded[vindex] = max;
                            } else {
                                decoded[vindex] = a + b - c; // x = a + b - c
                            }

                            decoded[vindex] += residues[vindex];
                        }

                    }

                    for (i = 0; i < yRows * yCols; i += 1) {
                        /* Accessing to planar info */
                        yindex = i;
                        uindex = (yRows * yCols) + (i / 2);
                        vindex = (yRows * yCols) * 3 / 2 + (i / 2);
                        y = decoded[yindex];
                        u = decoded[uindex];
                        v = decoded[vindex];

                        /* convert to RGB */
                        b = (int) (1.164 * (y - 16) + 2.018 * (u - 128));
                        g = (int) (1.164 * (y - 16) - 0.813 * (u - 128) - 0.391 * (v - 128));
                        r = (int) (1.164 * (y - 16) + 1.596 * (v - 128));

                        /* clipping to [0 ... 255] */
                        if (r < 0) r = 0;
                        if (g < 0) g = 0;
                        if (b < 0) b = 0;
                        if (r > 255) r = 255;
                        if (g > 255) g = 255;
                        if (b > 255) b = 255;

                        /* if you need the inverse formulas */
                        //y = r *  .299 + g *  .587 + b *  .114 ;
                        //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                        //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                        /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                        decodedrgb[i * 3] = b;
                        decodedrgb[i * 3 + 1] = g;
                        decodedrgb[i * 3 + 2] = r;

                    }

                }

                break;
            case 420:
                for (i = 0; i < yRows * yCols; i += 1) {
                    /* Accessing to planar info */
                    y = imgData[i];
                    int nRow = i / yCols / 2;
                    u = imgData[i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols)];
                    v = imgData[i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4];

                    /* convert to RGB*/
                    r = (int) (y + 1.28033 * (v - 128));
                    g = (int) (y - 0.21482 * (u - 128) - 0.38059 * (v - 128));
                    b = (int) (y + 2.12798 * (u - 128));

                    //r = y + 1.28033*v;
                    //g = y - 0.21482*u - 0.38059*v;
                    //b = y + 2.12798*u;

                    /* clipping to [0 ... 255] */
                    if (r < 0) r = 0;
                    if (g < 0) g = 0;
                    if (b < 0) b = 0;
                    if (r > 255) r = 255;
                    if (g > 255) g = 255;
                    if (b > 255) b = 255;

                    /* if you need the inverse formulas */
                    //y = r *  .299 + g *  .587 + b *  .114 ;
                    //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                    //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                    /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                    buffer[i * 3] = b;
                    buffer[i * 3 + 1] = g;
                    buffer[i * 3 + 2] = r;
                }

                if (encode) {
                    predicted = (uchar *) predictorimg.ptr();

                    for (i = 0; i < yRows * yCols; i += 1) {

                        /* Accessing to planar info */
                        yindex = i;
                        int nRow = i / yCols / 2;
                        uindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols);
                        vindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4;

                        uchar min, max;

                        //Y
                        if (imgData[yindex - 1] < imgData[yindex - yCols]) { // if a < b
                            min = imgData[yindex - 1];
                            max = imgData[yindex - yCols];
                        } else { // if a > b
                            min = imgData[yindex - yCols];
                            max = imgData[yindex - 1];
                        }
                        if (imgData[yindex - yCols - 1] >= max) { // c >= max
                            predicted[yindex] = min;
                        } else if (imgData[yindex - yCols - 1] <= min) { //c <= min
                            predicted[yindex] = max;
                        } else {
                            predicted[yindex] = imgData[yindex - 1] + imgData[yindex - yCols] -
                                                imgData[yindex - yCols - 1]; // x = a + b - c
                        }

                        //U
                        if (imgData[uindex - 1] < imgData[uindex - yCols]) { // if a < b
                            min = imgData[uindex - 1];
                            max = imgData[uindex - yCols];
                        } else { // if a > b
                            min = imgData[uindex - yCols];
                            max = imgData[uindex - 1];
                        }
                        if (imgData[uindex - yCols - 1] >= max) { // c >= max
                            predicted[uindex] = min;
                        } else if (imgData[uindex - yCols - 1] <= min) { //c <= min
                            predicted[uindex] = max;
                        } else {
                            predicted[uindex] = imgData[uindex - 1] + imgData[uindex - yCols] -
                                                imgData[uindex - yCols - 1]; // x = a + b - c
                        }

                        //V
                        if (imgData[vindex - 1] < imgData[vindex - yCols]) { // if a < b
                            min = imgData[vindex - 1];
                            max = imgData[vindex - yCols];
                        } else { // if a > b
                            min = imgData[vindex - yCols];
                            max = imgData[vindex - 1];
                        }
                        if (imgData[vindex - yCols - 1] >= max) { // c >= max
                            predicted[vindex] = min;
                        } else if (imgData[vindex - yCols - 1] <= min) { //c <= min
                            predicted[vindex] = max;
                        } else {
                            predicted[vindex] = imgData[vindex - 1] + imgData[vindex - yCols] -
                                                imgData[vindex - yCols - 1]; // x = a + b - c
                        }
                    }

                    for (i = 0; i < yRows * yCols; i += 1) {

                        /* Accessing to planar info */
                        yindex = i;
                        int nRow = i / yCols / 2;
                        uindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols);
                        vindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4;

                        bool yskip, uskip, vskip;
                        yskip = uskip = vskip = false;

                        if (yindex < yCols || !(yindex % yCols)) {
                            residues[yindex] = imgData[yindex];
                            yskip = true;
                        }
                        if (uindex - yRows * yCols < yCols || !(uindex % yCols)) {
                            residues[uindex] = imgData[uindex];
                            uskip = true;
                        }
                        if (vindex - yRows * yCols * 5 / 4 < yCols || !(vindex % yCols)) {
                            residues[vindex] = imgData[vindex];
                            vskip = true;
                        }

                        if (!yskip)
                            residues[yindex] = imgData[yindex] - predicted[yindex];

                        if (!uskip)
                            residues[uindex] = imgData[uindex] - predicted[uindex];

                        if (!vskip)
                            residues[vindex] = imgData[vindex] - predicted[vindex];


                    }


                    for (i = 0; i < yRows * yCols; i += 1) { // DECODING

                        /* Accessing to planar info */
                        yindex = i;
                        int nRow = i / yCols / 2;
                        uindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols);
                        vindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4;

                        bool yskip, uskip, vskip;
                        yskip = uskip = vskip = false;

                        if (yindex < yCols || !(yindex % yCols)) {
                            decoded[yindex] = residues[yindex];
                            yskip = true;
                        }
                        if (uindex - yRows * yCols < yCols || !(uindex % yCols)) { // i/2
                            decoded[uindex] = residues[uindex];
                            uskip = true;
                        }
                        if (vindex - yRows * yCols * 5 / 4 < yCols || !(vindex % yCols)) {  // i/2
                            decoded[vindex] = residues[vindex];
                            vskip = true;
                        }

                        uchar min, max;
                        uchar a, b, c;

                        //Y
                        if (!yskip) {
                            a = decoded[yindex - 1];
                            b = decoded[yindex - yCols];
                            c = decoded[yindex - yCols - 1];

                            if (a < b) { // if a < b
                                min = a;
                                max = b;
                            } else { // if a > b
                                min = b;
                                max = a;
                            }

                            if (c >= max) { // c >= max
                                decoded[yindex] = min;
                            } else if (c <= min) { //c <= min
                                decoded[yindex] = max;
                            } else {
                                decoded[yindex] = a + b - c; // x = a + b - c
                            }

                            decoded[yindex] += residues[yindex];
                        }
                        //U

                        if (!uskip) {
                            a = decoded[uindex - 1];
                            b = decoded[uindex - yCols];
                            c = decoded[uindex - yCols - 1];

                            if (a < b) { // if a < b
                                min = a;
                                max = b;
                            } else { // if a > b
                                min = b;
                                max = a;
                            }

                            if (c >= max) { // c >= max
                                decoded[uindex] = min;
                            } else if (c <= min) { //c <= min
                                decoded[uindex] = max;
                            } else {
                                decoded[uindex] = a + b - c; // x = a + b - c
                            }

                            decoded[uindex] += residues[uindex];
                        }
                        //V

                        if (!vskip) {

                            a = decoded[vindex - 1];
                            b = decoded[vindex - yCols];
                            c = decoded[vindex - yCols - 1];

                            if (a < b) { // if a < b
                                min = a;
                                max = b;
                            } else { // if a > b
                                min = b;
                                max = a;
                            }

                            if (c >= max) { // c >= max
                                decoded[vindex] = min;
                            } else if (c <= min) { //c <= min
                                decoded[vindex] = max;
                            } else {
                                decoded[vindex] = a + b - c; // x = a + b - c
                            }

                            decoded[vindex] += residues[vindex];
                        }
                    }

                    for (i = 0; i < yRows * yCols; i += 1) {
                        /* Accessing to planar info */
                        yindex = i;
                        int nRow = i / yCols / 2;
                        uindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols);
                        vindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4;
                        y = decoded[yindex];
                        u = decoded[uindex];
                        v = decoded[vindex];

                        /* convert to RGB */
                        b = (int) (1.164 * (y - 16) + 2.018 * (u - 128));
                        g = (int) (1.164 * (y - 16) - 0.813 * (u - 128) - 0.391 * (v - 128));
                        r = (int) (1.164 * (y - 16) + 1.596 * (v - 128));

                        /* clipping to [0 ... 255] */
                        if (r < 0) r = 0;
                        if (g < 0) g = 0;
                        if (b < 0) b = 0;
                        if (r > 255) r = 255;
                        if (g > 255) g = 255;
                        if (b > 255) b = 255;

                        /* if you need the inverse formulas */
                        //y = r *  .299 + g *  .587 + b *  .114 ;
                        //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                        //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                        /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                        decodedrgb[i * 3] = b;
                        decodedrgb[i * 3 + 1] = g;
                        decodedrgb[i * 3 + 2] = r;

                    }
                }

                break;
        }

        // Optical Flow stuff
        cvtColor(img, gray, COLOR_BGR2GRAY);
        if (!prevgray.empty()) {
            //calcOpticalFlowFarneback(prevgray, gray, uflow,
            //                         0.5, 3, 15, 3, 5, 1.2, 0);
            //cvtColor(prevgray, cflow, COLOR_GRAY2BGR);
            //uflow.copyTo(flow);
            //drawOptFlowMap(flow, cflow, 16, 1.5, Scalar(0, 255, 0));
            //imshow("flow", cflow);
        }

        /* display the image */
        if (playfile) imshow("rgb", img);

        if (playfile) {
            /* predictor matrix */

            //imshow("decoded", decodedimg);

            //imshow("decodedrgb", decodedimgrgb);

            //imshow( "intra", predictorimg );

            /*imgyuv = Mat(Size(yCols, yRows*3), CV_8UC1, imgData);

            imshow( "yuv" , imgyuv);*/

            //imshow("residues" , residuesimg);

        }

        switch (chroma) {
            case 444:
                for (int j = 0; j < yRows * yCols * 3; j++) {
                    rescounter++;
                    res.push_back((int) residues[j]);
                }
                break;
            case 422:
                for (int j = 0; j < yRows * yCols * 2; j++) {
                    rescounter++;
                    res.push_back((int) residues[j]);
                }
                break;
            case 420:
                for (int j = 0; j < yRows * yCols * 1.5; j++) {
                    rescounter++;
                    res.push_back((int) residues[j]);
                }
                break;
        }
        if (playfile) {
            if (playing) {
                /* wait according to the frame rate */
                inputKey = waitKey(1.0 / fps * 1000);
            } else {
                /* wait until user press a key */
                inputKey = waitKey(0);
            }
        }

        /* parse the pressed keys, if any */
        switch((char)inputKey)
        {
            case 'q':
                end = 1;
                break;

            case 'p':
                playing = playing ? 0 : 1;
                break;
        }


        // Optical Flow stuff
        std::swap(prevgray, gray);
    }

    cout << "Number of frames: " << nframes << endl;
    cout << "Number of residuals: " << rescounter << endl;
    if(output) {
        cout << "Writing encoded data to file..." << endl;
        ofstream outfile("encoded");
        encodeGolombToFile(res, outfile);
    }



    return 0;
}