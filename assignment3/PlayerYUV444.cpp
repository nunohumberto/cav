#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <fstream>

using namespace cv;
using namespace std;

static void drawOptFlowMap(const Mat& flow, Mat& cflowmap, int step,
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
}

int main(int argc, char** argv)
{
    string line; // store the header
    int chroma;
    int yCols, yRows; /* frame dimension */
    int fps = 60; /* frames per second */
    int i, n, r, g, b, y, u, v; /* auxiliary variables */
    int yindex, uindex, vindex; /* indexes */
    unsigned char *imgData, *predicted, *residues; // file data buffer
    uchar *buffer; // unsigned char pointer to the Mat data
    char inputKey = '?'; /* parse the pressed key */
    int end = 0, playing = 1, loop = 0, encode = 0; /* control variables */

    /* check for the mandatory arguments */
    if( argc < 2 ) {
        cerr << "Usage: PlayerYUV444 filename" << endl;
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

        if(!strcmp("-e", argv[n]))
        {
            encode = 1;
        }
    }

    /* data structure for the OpenCv image */
    Mat img = Mat(Size(yCols, yRows), CV_8UC3);

    Mat imgyuv;
    Mat predictorimg = Mat(Size(yCols, yRows*3), CV_8UC1);

    Mat residuesimg = Mat(Size(yCols, yRows*3), CV_8UC1);

    /* buffer to store the frame */
    imgData = new unsigned char[yCols * yRows * 3];

    /* create a window */
    cvNamedWindow( "rgb", CV_WINDOW_AUTOSIZE );


    // Optical Flow stuff
    Mat flow, cflow, frame;
    UMat gray, prevgray, uflow;
    namedWindow("flow", 1);

    while(!end)
    {
        /* load a new frame, if possible */
        getline (myfile,line); // Skipping word FRAME
        myfile.read((char *)imgData, yCols * yRows * 3);

        if(myfile.gcount() == 0)
        {
            if(loop)
            {
                myfile.clear();
                myfile.seekg(0);
                getline (myfile,line); // read the header
                continue;
            }
            else
            {
                end = 1;
                break;
            }
        }

        /* The video is stored in YUV planar mode but OpenCv uses packed modes*/
        buffer = (uchar*)img.ptr();

        switch(chroma){
            case 444:
                for(i = 0 ; i < yRows * yCols * 3 ; i += 3)
                {
                    /* Accessing to planar info */
                    y = imgData[i / 3];
                    u = imgData[(i / 3) + (yRows * yCols)];
                    v = imgData[(i / 3) + (yRows * yCols) * 2];

                    /* convert to RGB */
                    b = (int)(1.164*(y - 16) + 2.018*(u-128));
                    g = (int)(1.164*(y - 16) - 0.813*(u-128) - 0.391*(v-128));
                    r = (int)(1.164*(y - 16) + 1.596*(v-128));

                    /* clipping to [0 ... 255] */
                    if(r < 0) r = 0;
                    if(g < 0) g = 0;
                    if(b < 0) b = 0;
                    if(r > 255) r = 255;
                    if(g > 255) g = 255;
                    if(b > 255) b = 255;

                    /* if you need the inverse formulas */
                    //y = r *  .299 + g *  .587 + b *  .114 ;
                    //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                    //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                    /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                    buffer[i] = b;
                    buffer[i + 1] = g;
                    buffer[i + 2] = r;

                }

                if(encode){
                    predicted = (uchar*)predictorimg.ptr();

                    for(i = 0 ; i < yRows * yCols ; i += 1) {

                        /* Accessing to planar info */
                        yindex = i;
                        uindex = i + (yRows * yCols);
                        vindex = i + (yRows * yCols) * 2;

                        if(i < yCols || !(i%yCols)){
                            predicted[yindex] = imgData[yindex];
                            predicted[uindex] = imgData[uindex];
                            predicted[vindex] = imgData[vindex];
                        }

                        else{
                            uchar min, max;

                            //Y
                            if(imgData[yindex-1] < imgData[yindex-yCols]){ // if a < b
                                min = imgData[yindex-1];
                                max = imgData[yindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[yindex-yCols];
                                max = imgData[yindex-1];
                            }
                            if(imgData[yindex-yCols-1] >= max){ // c >= max
                                predicted[yindex] = min;
                            }
                            else if(imgData[yindex-yCols-1] <= min){ //c <= min
                                predicted[yindex] = max;
                            }
                            else{
                                predicted[yindex] = imgData[yindex-1] + imgData[yindex-yCols] - imgData[yindex-yCols-1]; // x = a + b - c
                            }

                            //U
                            if(imgData[uindex-1] < imgData[uindex-yCols]){ // if a < b
                                min = imgData[uindex-1];
                                max = imgData[uindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[uindex-yCols];
                                max = imgData[uindex-1];
                            }
                            if(imgData[uindex-yCols-1] >= max){ // c >= max
                                predicted[uindex] = min;
                            }
                            else if(imgData[uindex-yCols-1] <= min){ //c <= min
                                predicted[uindex] = max;
                            }
                            else{
                                predicted[uindex] = imgData[uindex-1] + imgData[uindex-yCols] - imgData[uindex-yCols-1]; // x = a + b - c
                            }

                            //V
                            if(imgData[vindex-1] < imgData[vindex-yCols]){ // if a < b
                                min = imgData[vindex-1];
                                max = imgData[vindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[vindex-yCols];
                                max = imgData[vindex-1];
                            }
                            if(imgData[vindex-yCols-1] >= max){ // c >= max
                                predicted[vindex] = min;
                            }
                            else if(imgData[vindex-yCols-1] <= min){ //c <= min
                                predicted[vindex] = max;
                            }
                            else{
                                predicted[vindex] = imgData[vindex-1] + imgData[vindex-yCols] - imgData[vindex-yCols-1]; // x = a + b - c
                            }

                        }

                    }
                }

                break;
            case 422:
                for(i = 0 ; i < yRows * yCols ; i += 1)
                {
                    /* Accessing to planar info */
                    y = imgData[i];
                    u = imgData[(yRows * yCols) + (i/2)];
                    v = imgData[(yRows * yCols)*3/2 + (i/2)];

                    /* convert to RGB*/
                    r = (int)(y + 1.28033*(v-128));
                    g = (int)(y - 0.21482*(u-128) - 0.38059*(v-128));
                    b = (int)(y + 2.12798*(u-128));

                    //r = y + 1.28033*v;
                    //g = y - 0.21482*u - 0.38059*v;
                    //b = y + 2.12798*u;

                    /* clipping to [0 ... 255] */
                    if(r < 0) r = 0;
                    if(g < 0) g = 0;
                    if(b < 0) b = 0;
                    if(r > 255) r = 255;
                    if(g > 255) g = 255;
                    if(b > 255) b = 255;

                    /* if you need the inverse formulas */
                    //y = r *  .299 + g *  .587 + b *  .114 ;
                    //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                    //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                    /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                    buffer[i*3] = b;
                    buffer[i*3 + 1] = g;
                    buffer[i*3 + 2] = r;
                }

                if(encode){
                    predicted = (uchar*)predictorimg.ptr();

                    for(i = 0 ; i < yRows * yCols ; i += 1) {

                        /* Accessing to planar info */
                        yindex = i;
                        uindex = (yRows * yCols) + (i/2);
                        vindex = (yRows * yCols)*3/2 + (i/2);

                        if(i < yCols || !(i%yCols)){
                            predicted[yindex] = imgData[yindex];
                            predicted[uindex] = imgData[uindex];
                            predicted[vindex] = imgData[vindex];
                        }

                        else{
                            uchar min, max;

                            //Y
                            if(imgData[yindex-1] < imgData[yindex-yCols]){ // if a < b
                                min = imgData[yindex-1];
                                max = imgData[yindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[yindex-yCols];
                                max = imgData[yindex-1];
                            }
                            if(imgData[yindex-yCols-1] >= max){ // c >= max
                                predicted[yindex] = min;
                            }
                            else if(imgData[yindex-yCols-1] <= min){ //c <= min
                                predicted[yindex] = max;
                            }
                            else{
                                predicted[yindex] = imgData[yindex-1] + imgData[yindex-yCols] - imgData[yindex-yCols-1]; // x = a + b - c
                            }

                            //U
                            if(imgData[uindex-1] < imgData[uindex-yCols]){ // if a < b
                                min = imgData[uindex-1];
                                max = imgData[uindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[uindex-yCols];
                                max = imgData[uindex-1];
                            }
                            if(imgData[uindex-yCols-1] >= max){ // c >= max
                                predicted[uindex] = min;
                            }
                            else if(imgData[uindex-yCols-1] <= min){ //c <= min
                                predicted[uindex] = max;
                            }
                            else{
                                predicted[uindex] = imgData[uindex-1] + imgData[uindex-yCols] - imgData[uindex-yCols-1]; // x = a + b - c
                            }

                            //V
                            if(imgData[vindex-1] < imgData[vindex-yCols]){ // if a < b
                                min = imgData[vindex-1];
                                max = imgData[vindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[vindex-yCols];
                                max = imgData[vindex-1];
                            }
                            if(imgData[vindex-yCols-1] >= max){ // c >= max
                                predicted[vindex] = min;
                            }
                            else if(imgData[vindex-yCols-1] <= min){ //c <= min
                                predicted[vindex] = max;
                            }
                            else{
                                predicted[vindex] = imgData[vindex-1] + imgData[vindex-yCols] - imgData[vindex-yCols-1]; // x = a + b - c
                            }

                        }

                    }
                }

                break;
            case 420:
                for(i = 0 ; i < yRows * yCols ; i += 1)
                {
                    /* Accessing to planar info */
                    y = imgData[i];
                    int nRow = i/yCols/2;
                    u = imgData[i/2%yCols + ((nRow-(nRow%2))/2)*yCols + (yRows * yCols)];
                    v = imgData[i/2%yCols + ((nRow-(nRow%2))/2)*yCols + (yRows * yCols)*5/4];

                    /* convert to RGB*/
                    r = (int)(y + 1.28033*(v-128));
                    g = (int)(y - 0.21482*(u-128) - 0.38059*(v-128));
                    b = (int)(y + 2.12798*(u-128));

                    //r = y + 1.28033*v;
                    //g = y - 0.21482*u - 0.38059*v;
                    //b = y + 2.12798*u;

                    /* clipping to [0 ... 255] */
                    if(r < 0) r = 0;
                    if(g < 0) g = 0;
                    if(b < 0) b = 0;
                    if(r > 255) r = 255;
                    if(g > 255) g = 255;
                    if(b > 255) b = 255;

                    /* if you need the inverse formulas */
                    //y = r *  .299 + g *  .587 + b *  .114 ;
                    //u = r * -.169 + g * -.332 + b *  .500  + 128.;
                    //v = r *  .500 + g * -.419 + b * -.0813 + 128.;

                    /* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
                    buffer[i*3] = b;
                    buffer[i*3 + 1] = g;
                    buffer[i*3 + 2] = r;
                }

                if(encode){
                    predicted = (uchar*)predictorimg.ptr();

                    for(i = 0 ; i < yRows * yCols ; i += 1) {

                        /* Accessing to planar info */
                        yindex = i;
                        int nRow = i/yCols/2;
                        uindex = i/2%yCols + ((nRow-(nRow%2))/2)*yCols + (yRows * yCols);
                        vindex = i/2%yCols + ((nRow-(nRow%2))/2)*yCols + (yRows * yCols)*5/4;

                        if(i < yCols || !(i%yCols)){
                            predicted[yindex] = imgData[yindex];
                            predicted[uindex] = imgData[uindex];
                            predicted[vindex] = imgData[vindex];
                        }

                        else{
                            uchar min, max;

                            //Y
                            if(imgData[yindex-1] < imgData[yindex-yCols]){ // if a < b
                                min = imgData[yindex-1];
                                max = imgData[yindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[yindex-yCols];
                                max = imgData[yindex-1];
                            }
                            if(imgData[yindex-yCols-1] >= max){ // c >= max
                                predicted[yindex] = min;
                            }
                            else if(imgData[yindex-yCols-1] <= min){ //c <= min
                                predicted[yindex] = max;
                            }
                            else{
                                predicted[yindex] = imgData[yindex-1] + imgData[yindex-yCols] - imgData[yindex-yCols-1]; // x = a + b - c
                            }

                            //U
                            if(imgData[uindex-1] < imgData[uindex-yCols]){ // if a < b
                                min = imgData[uindex-1];
                                max = imgData[uindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[uindex-yCols];
                                max = imgData[uindex-1];
                            }
                            if(imgData[uindex-yCols-1] >= max){ // c >= max
                                predicted[uindex] = min;
                            }
                            else if(imgData[uindex-yCols-1] <= min){ //c <= min
                                predicted[uindex] = max;
                            }
                            else{
                                predicted[uindex] = imgData[uindex-1] + imgData[uindex-yCols] - imgData[uindex-yCols-1]; // x = a + b - c
                            }

                            //V
                            if(imgData[vindex-1] < imgData[vindex-yCols]){ // if a < b
                                min = imgData[vindex-1];
                                max = imgData[vindex-yCols];
                            }
                            else{ // if a > b
                                min = imgData[vindex-yCols];
                                max = imgData[vindex-1];
                            }
                            if(imgData[vindex-yCols-1] >= max){ // c >= max
                                predicted[vindex] = min;
                            }
                            else if(imgData[vindex-yCols-1] <= min){ //c <= min
                                predicted[vindex] = max;
                            }
                            else{
                                predicted[vindex] = imgData[vindex-1] + imgData[vindex-yCols] - imgData[vindex-yCols-1]; // x = a + b - c
                            }

                        }

                    }

                }

                break;
        }

        residues = imgData - predicted;

        // Optical Flow stuff
        cvtColor(img, gray, COLOR_BGR2GRAY);
        if( !prevgray.empty() )
        {
            calcOpticalFlowFarneback(prevgray, gray, uflow,
                                     0.5, 3, 15, 3, 5, 1.2, 0);
            cvtColor(prevgray, cflow, COLOR_GRAY2BGR);
            uflow.copyTo(flow);
            drawOptFlowMap(flow, cflow, 16, 1.5, Scalar(0, 255, 0));
            imshow("flow", cflow);
        }

        /* display the image */
        imshow( "rgb", img );

        if(encode){
            /* predictor matrix */
            imshow( "intra", predictorimg );

            imgyuv = Mat(Size(yCols, yRows*3), CV_8UC1, imgData);
            imshow( "yuv" , imgyuv);
        }

        if(playing)
        {
            /* wait according to the frame rate */
            inputKey = waitKey(1.0 / fps * 1000);
        }
        else
        {
            /* wait until user press a key */
            inputKey = waitKey(0);
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

    return 0;
}