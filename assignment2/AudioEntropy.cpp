//
// Created by Nuno Humberto on 19/11/2017.
//

#include "AudioEntropy.h"
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;

void AudioEntropy::drawHistogram(map<short,int>& sndmap, string window_name) {

    int histsize = 256*256;
    int total = 0;
    int arr[256*256];
    for (int value = 0; value < 256*256; value++) {
        arr[value] = sndmap.count(value-32768) == 0 ? 0 : sndmap[value-32768];
        //cout << value << " -> " << arr[value] << endl;
        total+= arr[value];
    }

    map<short, int>::iterator it;

    short min = 0;
    short maxim = 0;
    short val;
    for (it = sndmap.begin(); it != sndmap.end(); ++it) {
        val = it->first;
        if (val < min) min = val;
        if (val > maxim) maxim = val;

    }

    //cerr << "Minimum: " << SHRT_MIN << "\nMaximum: " << SHRT_MAX << "\n";

    //cout << "Array filled.\n";


    cv::Mat in(1, 256*256, CV_32S, arr);

    /*if(window_name == "Histogram - L") {
        for (int value = 0; value < 256 * 256; value++) {
            cout << value - 32767 << " -> " << in.at<int>(0, value) << endl;
        }
    }*/


    // cv::Mat im(320, 240, CV_8UC3, cv::Scalar(0,0,0));
    //cout << "Matrix created.\n";

    //cv::Mat b_hist = in;
    //cv::Mat b_hist(256*256, 1, CV_8U, cv::Scalar(0, 0,0));
    cv::Mat b_hist = in;
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


    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);// Create a window for display.

    //cout << "Matrix displayed.\n";
    //cerr << "Size: (in)" << in.cols << "x" << in.rows << endl;
    //cv::calcHist( &in, 1, 0, cv::Mat(), b_hist, 1, &histsize, &histRange, uniform, accumulate );
    //cerr << "Size: (hist)" << b_hist.cols << "x" << b_hist.rows << endl;

    //for(int value = 0; value < 256/*256*/; value++) {
    //    cout << value << " -> " << b_hist.at<int>(value, 0) << endl;
    //}

    //int hist_w = 512; int hist_h = 400;
    int hist_w = 1024;
    int hist_h = 200;

    int bin_w = cvRound((double) histsize / hist_w); //256;//cvRound((double) hist_w / histsize);


    //for (int value = 0; value < 256*256; value++) {
    //    cout << value << " -> " << b_hist.at<int>(0, value) << endl;
    //}

    //cout <<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";

    cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(0, 0));
    //    normalize(b_hist, b_hist, 0, histImage.rows, cv::NORM_MINMAX, -1);
    //normalize(b_hist, b_hist, 0, histImage.rows, cv::NORM_MINMAX, CV_32S);

    for (int value = 0; value < 256*256; value++) {
        //cout << value << " -> " << b_hist.at<int>(0, value) << endl;
    }


    /*for (int i = 1; i < histsize; i++) {
        //line(histImage,
        //     cv::Point(bin_w * (i - 1), hist_h - b_hist.at<int>(i - 1)),
        //     cv::Point(bin_w * (i), hist_h - b_hist.at<int>(i)),
        //     cv::Scalar(250, 255, 0), 1, 8, 0);
    }*/

    int points = histsize/bin_w;
    double pointArray[points];
    //cout << "Generating an histogram with " << points << " points.\n";
    double avg = 0, last_avg = 0;
    for(int i = 0; i < points; i++) {
        last_avg = avg;
        avg = 0;
        for(int j = 0; j < bin_w; j++) {
            avg += b_hist.at<int>(i*bin_w+j);
            if (i==128) {
                //cout << "Adding to avg: " << b_hist.at<int>(i*bin_w+j) << endl;
            }
        }
        avg /= bin_w;
        pointArray[i] = avg;
        //cout << "Point " << i  << ": " << avg << endl;
        //line(histImage,
        //     cv::Point(i, hist_h - (cvRound(last_avg)+1)),
        //     cv::Point(i+1, hist_h - (cvRound(avg)+1)),
        //     cv::Scalar(250, 255, 0), 1, 8, 0);

    }


    cv::Mat average(1, points, CV_64F, pointArray);
    normalize(average, average, 0, hist_h, cv::NORM_MINMAX, CV_64F);

    //for (int value = 0; value < points; value++) {
    //    cout << value << " -> " << average.at<double>(0, value) << endl;
    //}


    for(int i = 1; i < points; i++) {
        //cout << i << " -> " << cvRound(average.at<double>(0, i)) << endl;
        line(histImage,
             cv::Point(i-1, hist_h - (cvRound(average.at<double>(0, i-1))+1)),
             cv::Point(i, hist_h - (cvRound(average.at<double>(0, i))+1)),
             cv::Scalar(250, 255, 0), 1, 8, 0);
        if (i == points/2) {
            line(histImage,
                 cv::Point(i, 0),
                 cv::Point(i, hist_h),
                 cv::Scalar(75, 66, 244), 1, 8, 0);
        }

    }


    int num_digits = 0;
    int temp_max = max;
    do {
        num_digits += 1;
        temp_max /= 10;
    } while (temp_max);


    if (num_digits < 6) num_digits = 6;

    int top = (int) (0.07 * histImage.rows);
    int bottom = (int) (0.07 * histImage.rows);
    int left = (int) (0.019 * num_digits * histImage.cols);
    int right = (int) (0.1 * histImage.cols);
    cv::Mat with_border;
    with_border = histImage;


    cv::copyMakeBorder(histImage, with_border, top, bottom, left, right, cv::BORDER_CONSTANT,
                       cv::Scalar(0, 0, 0));


    putText(with_border, to_string(max), cv::Point(7*num_digits, 20), cv::FONT_HERSHEY_DUPLEX, 0.4, cv::Scalar(255, 255, 255), 1, 8);
    putText(with_border, to_string(-32767), cv::Point(7 * num_digits, with_border.rows - 17), cv::FONT_HERSHEY_DUPLEX, 0.4, cv::Scalar(255, 255, 255), 1, 8);
    putText(with_border, to_string(32767), cv::Point(with_border.cols - 63, with_border.rows - 17), cv::FONT_HERSHEY_DUPLEX, 0.4, cv::Scalar(255, 255, 255), 1, 8);


    cv::imshow(window_name, with_border);


}

double AudioEntropy::calcEntropy(map<short,int>& sndmap) {
    map<short, int>::iterator it;

    double total = 0;

    for (it = sndmap.begin(); it != sndmap.end(); ++it) {
        total += sndmap[it->first];
    }

    //12.82
    double p;
    double alpha = 0.0;
    double entropy = 0;
    for (it = sndmap.begin(); it != sndmap.end(); ++it) {
        p = (sndmap[it->first]+alpha)/(total*1.0+alpha*65536);
        //cout << p*log2(p) <<endl;
        entropy += p*log2(p);
    }
    return -entropy;

}

double AudioEntropy::calcEntropy(map<int,int>& sndmap) {
    map<int, int>::iterator it;

    double total = 0;

    for (it = sndmap.begin(); it != sndmap.end(); ++it) {
        total += sndmap[it->first];
    }

    //12.82
    double p;
    double alpha = 0.0;
    double entropy = 0;
    for (it = sndmap.begin(); it != sndmap.end(); ++it) {
        p = (sndmap[it->first]+alpha)/(total*1.0+alpha*65536);
        //cout << p*log2(p) <<endl;
        entropy += p*log2(p);
    }
    return -entropy;

}



map<short, int> AudioEntropy::mapFromVector(vector<short>& vec) {
    map<short, int> tmpmap;
    vector<short>::iterator it;

    for(it = vec.begin(); it != vec.end(); it++) {
        if (tmpmap.count(*it) == 0) tmpmap[*it] = 0;
        tmpmap[*it] = tmpmap[*it] + 1;
    }
    return tmpmap;
};


map<short, int> AudioEntropy::reducedMapFromIntVector(vector<int>& vec) {
    map<short, int> tmpmap;
    vector<int>::iterator it;
    short casted;
    for(it = vec.begin(); it != vec.end(); it++) {
        if (*it < SHRT_MIN || *it > SHRT_MAX) continue;
        casted = (short) *it;
        if (tmpmap.count(casted) == 0) tmpmap[casted] = 0;
        tmpmap[casted] = tmpmap[casted] + 1;
    }
    return tmpmap;
};

map<int, int> AudioEntropy::mapFromIntVector(vector<int>& vec) {
    map<int, int> tmpmap;
    vector<int>::iterator it;

    for(it = vec.begin(); it != vec.end(); it++) {
        if (tmpmap.count(*it) == 0) tmpmap[*it] = 0;
        tmpmap[*it] = tmpmap[*it] + 1;
    }
    return tmpmap;
};