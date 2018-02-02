
#include <opencv2/opencv.hpp>
#include <iterator>
#include <vector>

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

void loadimage(int yRows, int yCols, int chroma, unsigned char imgData[], uchar *buffer){

    int i, r, g, b, y, u, v;

    for (i = 0; i < yRows * yCols; i++) {
        /* Accessing to planar info */


        if(chroma == 444){
            y = imgData[i];
            u = imgData[i + (yRows * yCols)];
            v = imgData[i + (yRows * yCols) * 2];
        }
        if(chroma == 422){
            y = imgData[i];
            u = imgData[(yRows * yCols) + (i / 2)];
            v = imgData[(yRows * yCols) * 3 / 2 + (i / 2)];
        }
        if(chroma == 420){
            y = imgData[i];
            int nRow = i / yCols / 2;
            u = imgData[i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols)];
            v = imgData[i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4];
        }

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
        buffer[i * 3] = b;
        buffer[i * 3 + 1] = g;
        buffer[i * 3 + 2] = r;

    }

}

void calculatePredicted(int yRows, int yCols, int chroma, unsigned char imgData[], uchar *predicted ){

    uchar min, max;
    uchar a, b, c, d;
    bool firstline=false, secline=false, firstcol=false, lastcol=false;
    int yindex, uindex, vindex;
    int i;
    for (i = 0; i < yRows * yCols; i += 1) {

        /* Accessing to planar info */
        if(chroma == 444){
            yindex = i;
            uindex = i + (yRows * yCols);
            vindex = i + (yRows * yCols) * 2;
        }

        if(chroma == 422){
            yindex = i;
            uindex = (yRows * yCols) + (i / 2);
            vindex = (yRows * yCols) * 3 / 2 + (i / 2);
        }
        if(chroma == 420){
            yindex = i;
            int nRow = i / yCols / 2;
            uindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols);
            vindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4;


        }

        //Y

        if (yindex < yCols) firstline = true;
        if (yindex >= yCols && yindex < 2*yCols) secline = true;
        if (!(yindex % yCols)) firstcol = true;
        if (yindex % yCols == yCols - 1) lastcol = true;

        if (firstline) {
            c = b = d = 0;
            if (firstcol) a = 0;
            else a = imgData[yindex - 1];
        }

        if (firstcol && !firstline) {
            a = b = imgData[yindex - yCols];
            if(secline) c = 0;
            else c = imgData[yindex - 2*yCols];
            d = imgData[yindex - yCols + 1];
        }

        if (lastcol && !firstline) {
            a = imgData[yindex - 1];
            d = b = imgData[yindex - yCols];
            c = imgData[yindex - yCols - 1];
        }

        if(!firstline && !firstcol && !lastcol) {
            a = imgData[yindex - 1];
            b = imgData[yindex - yCols];
            c = imgData[yindex - yCols - 1];
            d = imgData[yindex - yCols + 1];
        }



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
            predicted[yindex] = a + b - c;
        }



        //U


        switch(chroma) {
            case 444:
                break;
            case 422:
                firstline = (uindex - yRows * yCols) < yCols/2;
                secline = (uindex - yRows * yCols) >= yCols/2 && (uindex - yRows * yCols) < yCols;
                firstcol = !(uindex % (yCols/2));
                lastcol = uindex % (yCols/2) == yCols/2 - 1;
                break;
            case 420:
                firstline = (uindex - yRows * yCols) < yCols/2;
                secline = (uindex - yRows * yCols) >= yCols/2 && (uindex - yRows * yCols) < yCols;
                firstcol = !(uindex % (yCols/2));
                lastcol = uindex % (yCols/2) == yCols/2 - 1;
                break;
        }

        if (firstline) {
            c = b = d = 0;
            if (firstcol) a = 0;
            else a = imgData[uindex - 1];
        }

        if (firstcol && !firstline) {
            a = b = imgData[uindex - yCols/2];
            if(secline) c = 0;
            else c = imgData[uindex - yCols];
            d = imgData[uindex - yCols/2 + 1];
        }

        if (lastcol && !firstline) {
            a = imgData[uindex - 1];
            d = b = imgData[uindex - yCols/2];
            c = imgData[uindex - yCols/2 - 1];
        }


        if(!firstline && !firstcol && !lastcol) {
            a = imgData[uindex - 1];
            b = imgData[uindex - yCols/2];
            c = imgData[uindex - yCols/2 - 1];
            d = imgData[uindex - yCols/2 + 1];
        }


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
        switch(chroma) {
            case 444:
                break;
            case 422:
                firstline = vindex - yRows * yCols * (3/2) < yCols/2;
                secline = (vindex - yRows * yCols * (3/2)) >= yCols/2 && (vindex - yRows * yCols * (3/2)) < yCols;
                firstcol = !(vindex % (yCols/2));
                lastcol = vindex % (yCols/2) == yCols/2 - 1;
                break;
            case 420:
                firstline = vindex - yRows * yCols * (5/4) < yCols/2;
                secline = (vindex - yRows * yCols * (5/4)) >= yCols/2 && (vindex - yRows * yCols * (5/4)) < yCols;
                firstcol = !(vindex % (yCols/2));
                lastcol = vindex % (yCols/2) == yCols/2 - 1;
                break;
        }

        if (vindex < yCols) firstline = true;
        if (vindex >= yCols && vindex < 2*yCols) secline = true;
        if (!(vindex % yCols)) firstcol = true;
        if (vindex % yCols == yCols - 1) lastcol = true;

        if (firstline) {
            c = b = d = 0;
            if (firstcol) a = 0;
            else a = imgData[vindex - 1];
        }

        if (firstcol && !firstline) {
            a = b = imgData[vindex - yCols];
            if(secline) c = 0;
            else c = imgData[vindex - 2*yCols];
            d = imgData[vindex - yCols + 1];
        }

        if (lastcol && !firstline) {
            a = imgData[vindex - 1];
            d = b = imgData[vindex - yCols];
            c = imgData[vindex - yCols - 1];
        }


        if(!firstline && !firstcol && !lastcol) {
            a = imgData[vindex - 1];
            b = imgData[vindex - yCols/2];
            c = imgData[vindex - yCols/2 - 1];
            d = imgData[vindex - yCols/2 + 1];
        }

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
}

void calculateResidues(int yRows, int yCols, int chroma, unsigned char imgData[], uchar *residues, uchar *predicted){

    int i, yindex, uindex, vindex;
    for (i = 0; i < yRows * yCols; i += 1) { // RESIDUES = ORIGINAL - PREDICTED

        /* Accessing to planar info */
        if(chroma == 444){
            yindex = i;
            uindex = i + (yRows * yCols);
            vindex = i + (yRows * yCols) * 2;
        }
        if(chroma == 422){
            yindex = i;
            uindex = (yRows * yCols) + (i / 2);
            vindex = (yRows * yCols) * 3 / 2 + (i / 2);
        }
        if(chroma == 420){
            yindex = i;
            int nRow = i / yCols / 2;
            uindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols);
            vindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4;
        }

        bool yskip, uskip, vskip;
        yskip = uskip = vskip = false;

        /*if(chroma == 444){
            if (i < yCols || !(i % yCols)) {
                residues[yindex] = imgData[yindex];
                residues[uindex] = imgData[uindex];
                residues[vindex] = imgData[vindex];
                yskip = uskip = vskip = true;
            }
        }

        if(chroma == 422){
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
        }

        if(chroma == 420){
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
        }*/

        if (!yskip)
            residues[yindex] = imgData[yindex] - predicted[yindex];

        if (!uskip)
            residues[uindex] = imgData[uindex] - predicted[uindex];

        if (!vskip)
            residues[vindex] = imgData[vindex] - predicted[vindex];

    }

}

void decodeResidues(int yRows, int yCols, int chroma, uchar *residues, uchar *decoded){

    int i, yindex, uindex, vindex;
    for (i = 0; i < yRows * yCols; i += 1) { // DECODING

        /* Accessing to planar info */
        if(chroma == 444){
            yindex = i;
            uindex = i + (yRows * yCols);
            vindex = i + (yRows * yCols) * 2;
        }
        if(chroma == 422){
            yindex = i;
            uindex = (yRows * yCols) + (i / 2);
            vindex = (yRows * yCols) * 3 / 2 + (i / 2);
        }
        if(chroma == 420){
            yindex = i;
            int nRow = i / yCols / 2;
            uindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols);
            vindex = i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4;
        }

        bool yskip, uskip, vskip;
        yskip = uskip = vskip = false;

        /*
        if(chroma == 444){
            if (i < yCols || !(i % yCols)) {
                decoded[yindex] = residues[yindex];
                decoded[uindex] = residues[uindex];
                decoded[vindex] = residues[vindex];
                yskip = uskip = vskip = true;
            }
        }
        if(chroma == 422){
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
        }
        if(chroma == 420){
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
        }*/

        uchar min, max;
        int a,b,c,d;
        bool firstline=false, secline=false, firstcol=false, lastcol=false;

        // Y
        if (!yskip){

            if (yindex < yCols) firstline = true;
            if (yindex >= yCols && yindex < 2*yCols) secline = true;
            if (!(yindex % yCols)) firstcol = true;
            if (yindex % yCols == yCols - 1) lastcol = true;

            if (firstline) {
                c = b = d = 0;
                if (firstcol) a = 0;
                else a = decoded[yindex - 1];
            }

            if (firstcol && !firstline) {
                a = b = decoded[yindex - yCols];
                if(secline) c = 0;
                else c = decoded[yindex - 2*yCols];
                d = decoded[yindex - yCols + 1];
            }

            if (lastcol && !firstline) {
                a = decoded[yindex - 1];
                d = b = decoded[yindex - yCols];
                c = decoded[yindex - yCols - 1];
            }

            if(!firstline && !firstcol && !lastcol) {
                a = decoded[yindex - 1];
                b = decoded[yindex - yCols];
                c = decoded[yindex - yCols - 1];
                d = decoded[yindex - yCols + 1];
            }


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
                decoded[yindex] = a + b - c;
            }

            decoded[yindex] += residues[yindex];
        }


        //U
        if(!uskip){
            switch(chroma) {
                case 444:
                    break;
                case 422:
                    firstline = (uindex - yRows * yCols) < yCols/2;
                    secline = (uindex - yRows * yCols) >= yCols/2 && (uindex - yRows * yCols) < yCols;
                    firstcol = uindex % (yCols/2) == 0;
                    lastcol = (uindex % (yCols/2)) == (yCols/2 - 1);
                    break;
                case 420:
                    firstline = (uindex - yRows * yCols) < yCols/2;
                    secline = (uindex - yRows * yCols) >= yCols/2 && (uindex - yRows * yCols) < yCols;
                    firstcol = uindex % (yCols/2) == 0;
                    lastcol = (uindex % (yCols/2)) == (yCols/2 - 1);
                    break;
            }

            if (firstline) {
                c = b = d = 0;
                if (firstcol) a = 0;
                else a = decoded[uindex - 1];
            }

            if (firstcol && !firstline) {
                a = b = decoded[uindex - yCols/2];
                if(secline) c = 0;
                else c = decoded[uindex - yCols];
                d = decoded[uindex - yCols/2 + 1];
            }

            if (lastcol && !firstline) {
                a = decoded[uindex - 1];
                d = b = decoded[uindex - yCols/2];
                c = decoded[uindex - yCols/2 - 1];
            }


            if(!firstline && !firstcol && !lastcol) {
                a = decoded[uindex - 1];
                b = decoded[uindex - yCols/2];
                c = decoded[uindex - yCols/2 - 1];
                d = decoded[uindex - yCols/2 + 1];
            }


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
        if(!vskip){
            switch(chroma) {
                case 444:
                    break;
                case 422:
                    firstline = vindex - yRows * yCols * (3/2) < yCols/2;
                    secline = (vindex - yRows * yCols * (3/2)) >= yCols/2 && (vindex - yRows * yCols * (3/2)) < yCols;
                    firstcol = !(vindex % (yCols/2));
                    lastcol = vindex % (yCols/2) == yCols/2 - 1;
                    break;
                case 420:
                    firstline = vindex - yRows * yCols * (5/4) < yCols/2;
                    secline = (vindex - yRows * yCols * (5/4)) >= yCols/2 && (vindex - yRows * yCols * (5/4)) < yCols;
                    firstcol = !(vindex % (yCols/2));
                    lastcol = vindex % (yCols/2) == yCols/2 - 1;
                    break;
            }


            if (vindex < yCols) firstline = true;
            if (vindex >= yCols && vindex < 2*yCols) secline = true;
            if (!(vindex % yCols)) firstcol = true;
            else if (vindex % yCols == yCols - 1) lastcol = true;

            if (firstline) {
                c = b = d = 0;
                if (firstcol) a = 0;
                else a = decoded[vindex - 1];
            }

            if (firstcol && !firstline) {
                a = b = decoded[vindex - yCols];
                if(secline) c = 0;
                else c = decoded[vindex - 2*yCols];
                d = decoded[vindex - yCols + 1];
            }

            if (lastcol && !firstline) {
                a = decoded[vindex - 1];
                d = b = decoded[vindex - yCols];
                c = decoded[vindex - yCols - 1];
            }

            if(!firstline && !firstcol && !lastcol) {
                a = decoded[vindex - 1];
                b = decoded[vindex - yCols/2];
                c = decoded[vindex - yCols/2 - 1];
                d = decoded[vindex - yCols/2 + 1];
            }

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
}

void decodePredicted(int yRows, int yCols, int chroma, uchar *decodedrgb, uchar *decoded){

    int i, r, g, b, y, u, v;
    for (i = 0; i < yRows * yCols; i++) {
        /* Accessing to planar info */
        if(chroma == 444){
            y = decoded[i];
            u = decoded[i + (yRows * yCols)];
            v = decoded[i + (yRows * yCols) * 2];
        }
        if(chroma == 422){
            y = decoded[i];
            u = decoded[(yRows * yCols) + (i / 2)];
            v = decoded[(yRows * yCols) * 3 / 2 + (i / 2)];
        }
        if(chroma == 420){
            y = decoded[i];
            int nRow = i / yCols / 2;
            u = decoded[i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols)];
            v = decoded[i / 2 % yCols + ((nRow - (nRow % 2)) / 2) * yCols + (yRows * yCols) * 5 / 4];
        }


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

int rescounter = 0;


int main(int argc, char** argv){
    string line; // store the header
    int chroma;
    int yCols, yRows; /* frame dimension */
    int fps = 30; /* frames per second */
    int n; /* auxiliary variables */
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


        loadimage(yRows,yCols,chroma,imgData,buffer);

        if (encode) {
            //ENCODING
            predicted = (uchar *) predictorimg.ptr();
            calculatePredicted(yRows,yCols,chroma,imgData,predicted);
            calculateResidues(yRows, yCols, chroma, imgData, residues, predicted);

            //DECODING
            decodeResidues(yRows, yCols, chroma, residues, decoded);
            decodePredicted(yRows, yCols, chroma, decodedrgb, decoded);
        }

        // Optical Flow stuff
        cvtColor(img, gray, COLOR_BGR2GRAY);
        if (!prevgray.empty()) {
            calcOpticalFlowFarneback(prevgray, gray, uflow,
                                     0.5, 3, 15, 3, 5, 1.2, 0);
            cvtColor(prevgray, cflow, COLOR_GRAY2BGR);
            uflow.copyTo(flow);
            drawOptFlowMap(flow, cflow, 16, 1.5, Scalar(0, 255, 0));
            imshow("flow", cflow);
        }

        /* display the image */
        if (playfile) imshow("rgb", img);

        if (playfile) {
            /* predictor matrix */

            imshow("decoded", decodedimg);

            imshow("decodedrgb", decodedimgrgb);

            imshow( "intra", predictorimg );

            /*imgyuv = Mat(Size(yCols, yRows*3), CV_8UC1, imgData);

            imshow( "yuv" , imgyuv);*/

            imshow("residues" , residuesimg);

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