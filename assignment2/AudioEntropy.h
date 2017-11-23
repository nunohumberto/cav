//
// Created by Nuno Humberto on 19/11/2017.
//

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>

using namespace std;

#ifndef ASSIGNMENT2_AUDIOENTROPY_H
#define ASSIGNMENT2_AUDIOENTROPY_H


class AudioEntropy {
private:
    double delta;
public:
    void drawHistogram(map<short,int>&, string);
    double calcEntropy(map<short,int>&);
    double calcEntropy(map<int,int>&);
    double calcEntropy(int* arr);
    short quantize(short input);
    void setupQuantizer(int divisions);

    map<short, int> mapFromVector(vector<short>&);
    map<short, int> reducedMapFromIntVector(vector<int>&);
    map<int, int> mapFromIntVector(vector<int>&);
    void arrFromIntVector(vector<int>&, int* arr);

};


#endif //ASSIGNMENT2_AUDIOENTROPY_H
