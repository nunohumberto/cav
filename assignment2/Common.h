//
// Created by Nuno Humberto on 19/11/2017.
//

#ifndef ASSIGNMENT2_COMMON_H
#define ASSIGNMENT2_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>

using namespace std;

class Common {
    public:
        void calculateResidues(vector<short>& input, vector<int> output[]);
        void calculateResidues(vector<int>& input, vector<int> output[]);
        int findBestM(vector<int>& input, int blocksize, long *bestsize);
        int blockSumComparison(vector<int> residues[], vector<short>& orig, int blocksize, int blockindex);
        int blockSumComparison(vector<int> residues[], vector<int>& orig, int blocksize, int blockindex);
        int blockSumComparison(vector<int> residues[], int blocksize, int blockindex);
        map<int, int> residueStats(vector<int> residues[], vector<short> orig, int blocksize);
        map<int, int> residueStats(vector<int> residues[], int blocksize);
        map<int, int> residueStats(vector<int> residues[], vector<int> orig, int blocksize);
        vector<int> residueComparison(vector<int> residues[], int blocksize, vector<int>& values);
        vector<int> residueComparison(vector<int> residues[], vector<short> orig, int blocksize, vector<int>& values);
        vector<int> residueComparison(vector<int> residues[], vector<int>& orig, int blocksize, vector<int>& values);
        vector<int> sliceVector(vector<int>& input, long start, long end);
        vector<int> findBestPartitionNumber(vector<int>& input, int bs, long *estimatedsize_out, int *factor_out);
};


#endif //ASSIGNMENT2_COMMON_H
