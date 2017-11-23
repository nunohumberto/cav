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
        void calculateResiduals(vector<short> &input, vector<int> *output);
        void calculateResiduals(vector<int> &input, vector<int> *output);
        int findBestM(vector<int>& input, int blocksize, long *bestsize);
        int blockSumComparison(vector<int> residuals[], vector<short>& orig, int blocksize, int blockindex);
        int blockSumComparison(vector<int> residuals[], vector<int>& orig, int blocksize, int blockindex);
        int blockSumComparison(vector<int> residuals[], int blocksize, int blockindex);
        map<int, int> residualStats(vector<int> *residuals, vector<short> orig, int blocksize);
        map<int, int> residualStats(vector<int> *residuals, int blocksize);
        map<int, int> residualStats(vector<int> *residuals, vector<int> orig, int blocksize);
        vector<int> residualComparison(vector<int> *residuals, int blocksize, vector<int> &values);
        vector<int> residualComparison(vector<int> *residuals, vector<short> orig, int blocksize, vector<int> &values);
        vector<int> residualComparison(vector<int> *residuals, vector<int> &orig, int blocksize, vector<int> &values);
        vector<int> sliceVector(vector<int>& input, long start, long end);
        vector<int> findBestPartitionNumber(vector<int>& input, int bs, long *estimatedsize_out, int *factor_out);
};


#endif //ASSIGNMENT2_COMMON_H
