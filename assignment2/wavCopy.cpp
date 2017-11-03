#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <sndfile.hh>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>

using namespace std;

void drawHistogram(map<short,int> sndmap) {

    int histsize = 256*256;

    int arr[256*256];

    for (int value = 0; value < 256*256; value++) {
        arr[value] = sndmap.count(value) == 0 ? 0 : sndmap[value];
        //cout << value << " -> " << arr[value] << endl;
    }

    cout << "Array filled.\n";


    cv::Mat in(1, 256*256, CV_8U, arr);

    /*for (int value = 0; value < 20; value++) {
        cout << value << " -> " << in.at<int>(0, 65535-value) << endl;
    }*/


    // cv::Mat im(320, 240, CV_8UC3, cv::Scalar(0,0,0));
    cout << "Matrix created.\n";

    //cv::Mat b_hist = in;
    cv::Mat b_hist(256*256, 1, CV_8U, cv::Scalar(0, 0,0));
    b_hist = in;
    int max = -1, new_val;
    for (int i = 0; i < 256*256 /*256*/; i++) {
        //cout << i << " -> " << in.at<int>(0, i) << endl;
        new_val = in.at<int>(0, i);
        if (new_val > max) max = new_val;
        //b_hist.at<int>(i, 0) = new_val;
    }



    //for (int value = 0; value < 20; value++) {
    //    cout << value << " -> " << b_hist.at<int>(value, 0) << endl;
    //}


    cv::namedWindow("Histogram", cv::WINDOW_AUTOSIZE);// Create a window for display.

    cout << "Matrix displayed.\n";
    //cerr << "Size: (in)" << in.cols << "x" << in.rows << endl;
    //cv::calcHist( &in, 1, 0, cv::Mat(), b_hist, 1, &histsize, &histRange, uniform, accumulate );
    //cerr << "Size: (hist)" << b_hist.cols << "x" << b_hist.rows << endl;

    //for(int value = 0; value < 256/*256*/; value++) {
    //    cout << value << " -> " << b_hist.at<int>(value, 0) << endl;
    //}

    //int hist_w = 512; int hist_h = 400;
    int hist_w = 512;
    int hist_h = 400;

    int bin_w = cvRound((double) histsize / hist_w); //256;//cvRound((double) hist_w / histsize);

    cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(0, 0));
    //normalize(b_hist, b_hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());

    //for (int value = 0; value < 20; value++) {
    //    cout << value << " -> " << b_hist.at<int>(0, value) << endl;
    //}


    /*for (int i = 1; i < histsize; i++) {
        //line(histImage,
        //     cv::Point(bin_w * (i - 1), hist_h - b_hist.at<int>(i - 1)),
        //     cv::Point(bin_w * (i), hist_h - b_hist.at<int>(i)),
        //     cv::Scalar(250, 255, 0), 1, 8, 0);
    }*/

    int points = histsize/bin_w;
    cout << "Generating an histogram with " << points << " points.\n";
    double avg = 0, last_avg = 0;
    for(int i = 0; i < points; i++) {
        last_avg = avg;
        avg = 0;
        for(int j = 0; j < bin_w; j++) {
            avg += b_hist.at<int>(i*bin_w+j);

        }
        avg /= bin_w;

        line(histImage,
             cv::Point(i, hist_h - cvRound(last_avg)),
             cv::Point(i+1, hist_h - cvRound(avg)),
             cv::Scalar(250, 255, 0), 1, 8, 0);

    }


    int num_digits = 0;
    int temp_max = max;
    do {
        num_digits += 1;
        temp_max /= 10;
    } while (temp_max);


    int top = (int) (0.07 * histImage.rows);
    int bottom = (int) (0.07 * histImage.rows);
    int left = (int) (0.019 * num_digits * histImage.cols);
    int right = (int) (0.1 * histImage.cols);
    cv::Mat with_border;
    with_border = histImage;


    /*cv::copyMakeBorder(histImage, with_border, top, bottom, left, right, cv::BORDER_CONSTANT,
                       cv::Scalar(255, 255, 255));


    putText(with_border, to_string(max), cv::Point(3, 35), cv::FONT_HERSHEY_DUPLEX, 0.4, cv::Scalar(0, 0, 0), 1, 8);
    putText(with_border, to_string(0), cv::Point(7 * num_digits, with_border.rows - 17), cv::FONT_HERSHEY_DUPLEX, 0.4, cv::Scalar(0, 0, 0), 1, 8);
    putText(with_border, to_string(histsize-1), cv::Point(with_border.cols - 63, with_border.rows - 17), cv::FONT_HERSHEY_DUPLEX, 0.4, cv::Scalar(0, 0, 0), 1, 8);
    */cv::imshow("Histogram", with_border);

}

double calcEntropy(map<short,int> sndmap) {
    map<short, int>::iterator it;

    double total = 0;

    for (it = sndmap.begin(); it != sndmap.end(); ++it) {
        total += sndmap[it->first];
    }

    //12.82
    double p;
    double alpha = 0.7;
    double entropy = 0;
    for (it = sndmap.begin(); it != sndmap.end(); ++it) {
        p = (sndmap[it->first]+alpha)/(total*1.0+alpha*65536);
        //cout << p*log2(p) <<endl;
        entropy += p*log2(p);
    }






    cout << "Entropy: " << -entropy << endl;
    return -1.0;

}


int main(int argc, char **argv) {
    SndfileHandle soundFileIn;
    SndfileHandle soundFileOut;

    int i;
    short *sample = new short[2];
    sf_count_t nSamples = 2;

    map<short, int> sndmap;

    if (argc < 3) {
        fprintf(stderr, "Usage: wavCopy <input file> <output file>\n");
        return -1;
    }

    soundFileIn = SndfileHandle(argv[1]);

    int channels = 2;
    int srate = 44100;
    soundFileOut = SndfileHandle(argv[2], SFM_WRITE,
                                 SF_FORMAT_WAV | SF_FORMAT_PCM_16, channels, srate);

    fprintf(stderr, "Frames (samples): %d\n", (int) soundFileIn.frames());
    fprintf(stderr, "Samplerate: %d\n", soundFileIn.samplerate());
    fprintf(stderr, "Channels: %d\n", soundFileIn.channels());

    int max = -1;
    for (i = 0; i < soundFileIn.frames(); i++) {
        if (soundFileIn.read(sample, nSamples) == 0) {
            fprintf(stderr, "Error: Reached end of file %d\n", i);
            break;
        }

        if (sample[0] > max) max = sample[0];
        if (sample[1] > max) max = sample[1];


        if (sndmap.count(sample[0]) == 0) sndmap[sample[0]] = 0;
        if (sndmap.count(sample[1]) == 0) sndmap[sample[1]] = 0;
        sndmap[sample[0]] = sndmap[sample[0]] + 1;
        sndmap[sample[1]] = sndmap[sample[1]] + 1;


        if (soundFileOut.write(sample, nSamples) != 2) {
            fprintf(stderr, "Error writing frames to the output:\n");
            return -1;
        }
    }

    cerr << "Will now calculate entropy!\n";
    calcEntropy(sndmap);
    drawHistogram(sndmap);
    cvWaitKey(0);


}


