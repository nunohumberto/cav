//
// Created by Nuno Humberto on 19/11/2017.
//

#ifndef ASSIGNMENT2_FAKEAUDIOCODEC_H
#define ASSIGNMENT2_FAKEAUDIOCODEC_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;

class FakeAudioCodec {
public:
    vector<int> fakeDecodeGolombFromFile(ifstream& input, long total_samples, int bs, int factor);
    void fakeEncodeGolombToFile(vector<int>& input, vector<int>& predictors, vector<int>& m_vector, ofstream& outfile, int bs, int factor);
    int decodeSingleResidue(int input, int winner, int last_vals[], bool debug);
    int fakeDecodeToWav(ifstream& input, vector<short>& outLEFT, vector<int>& outDELTA);
    vector<short> lossyRecover(vector<short>& in);
    vector<int> lossyRecover(vector<int>& in);
    void fakeReadHeader(ifstream& infile, long *samples, int *bs, int *channels, int *fact);
    void fakeWriteHeader(ofstream& outfile, long samples, int bs, int channels, int fact);
    void writeWavToFile(string filename, vector<short>& left, vector<int>& delta, int chan);
};


#endif //ASSIGNMENT2_FAKEAUDIOCODEC_H
