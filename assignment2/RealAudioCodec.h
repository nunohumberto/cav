//
// Created by Nuno Humberto on 19/11/2017.
//

#ifndef ASSIGNMENT2_REALAUDIOCODEC_H
#define ASSIGNMENT2_REALAUDIOCODEC_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;

class RealAudioCodec {
private:
    int buffer = 0;
    int bitsinbuffer = 0;
public:
    vector<int> decodeGolombFromFile(ifstream& input, long total_samples, int bs, int factor);
    void encodeGolombToFile(vector<int>& input, vector<int>& predictors, vector<int>& m_vector, ofstream& outfile, int bs, int factor);
    int decodeSingleResidue(int input, int winner, int last_vals[], bool debug);
    int decodeToWav(ifstream& input, vector<short>& outLEFT, vector<int>& outDELTA);
    vector<short> lossyRecover(vector<short>& in);
    vector<int> lossyRecover(vector<int>& in);
    void readHeader(ifstream& infile, long *samples, int *bs, int *channels, int *fact);
    void writeHeader(ofstream& outfile, long samples, int bs, int channels, int fact);
    void writeWavToFile(string filename, vector<short>& left, vector<int>& delta, int chan);
    void replenish(ifstream& infile);
    char readNextByte(ifstream& infile);
    int readNextPredictor(ifstream& infile);
    int readNextQ(ifstream& infile);
    int readNextR(ifstream& infile, int nobits);
    int buildOnes(int n);
    void flush(ofstream& file);
    void forceFlush(ofstream& file);
    void writeToFile(int data, int length, char type, ofstream& file);
};


#endif //ASSIGNMENT2_REALAUDIOCODEC_H
