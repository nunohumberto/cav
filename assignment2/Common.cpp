//
// Created by Nuno Humberto on 19/11/2017.
//

#include "Common.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"



using namespace std;




void Common::calculateResidues(vector<short>& input, vector<int> output[]) {
    int lastValues[4] = {0};
    int actualValues[5] = {0};
    int counter = 10;
    vector<short>::iterator it;
    for(it = input.begin(); it != input.end(); it++) {
        actualValues[0] = (int) *it;
        actualValues[1] = actualValues[0] - lastValues[0];
        actualValues[2] = actualValues[1] - lastValues[1];
        actualValues[3] = actualValues[2] - lastValues[2];
        actualValues[4] = actualValues[3] - lastValues[3];

        output[0].push_back(actualValues[1]);
        output[1].push_back(actualValues[2]);
        output[2].push_back(actualValues[3]);
        output[3].push_back(actualValues[4]);

        lastValues[0] = actualValues[0];
        lastValues[1] = actualValues[1];
        lastValues[2] = actualValues[2];
        lastValues[3] = actualValues[3];
    }
    //for(int i = 0; i < 10; i++) if(counter-- > 0) cout << output[0].at(i) << " ";
    //cout << endl;
}


void Common::calculateResidues(vector<int>& input, vector<int> output[]) {
    int lastValues[4] = {0};
    int actualValues[5] = {0};
    vector<int>::iterator it;
    for(it = input.begin(); it != input.end(); it++) {
        actualValues[0] = (int) *it;
        actualValues[1] = actualValues[0] - lastValues[0];
        actualValues[2] = actualValues[1] - lastValues[1];
        actualValues[3] = actualValues[2] - lastValues[2];
        actualValues[4] = actualValues[3] - lastValues[3];

        output[0].push_back(actualValues[1]);
        output[1].push_back(actualValues[2]);
        output[2].push_back(actualValues[3]);
        output[3].push_back(actualValues[4]);

        lastValues[0] = actualValues[0];
        lastValues[1] = actualValues[1];
        lastValues[2] = actualValues[2];
        lastValues[3] = actualValues[3];
    }
    //for(int i = 0; i < 10; i++) if(counter-- > 0) cout << output[0].at(i) << " ";
    //cout << endl;
}


int Common::findBestM(vector<int>& input, int blocksize, long *bestsize) {
    vector<int>::iterator it;
    int target;
    long totalnobits = -1;
    long last_total = LONG_MAX;
    int exponent = 2;
    int q, m = -1, last_m;
    int predictor_overhead = 2;
    int nobits;

    while(totalnobits <= last_total) {
        if(totalnobits < last_total && totalnobits != -1) last_total = totalnobits;
        totalnobits = 0;
        last_m = m;
        m = (int) pow(2, exponent);
        nobits = (int) log2(m);
        int i = 0;
        for (it = input.begin(); it != input.end(); it++, i++) {
            if (*it < 0) target = -(1 + *it * 2);
            else target = 2 * *it;

            q = target / m;
            totalnobits += (q + 1);
            totalnobits += nobits;
            if(i % blocksize == 0) totalnobits += predictor_overhead;
        }

        //cout << "Expected number of bytes (M=" << m << ") " << totalnobits/8 << endl;
        exponent++;
    }

    //cout << "Found best value for M: " << last_m << endl;
    *bestsize = last_total;
    return last_m;
}


int Common::blockSumComparison(vector<int> residues[], vector<short>& orig, int blocksize, int blockindex) {
    long long tmpsum = 0;
    long long lowest_sum;
    int lowest_index;
    long maxsize = orig.size();
    for(int i = blocksize*blockindex; (i < blocksize*(blockindex+1)) && (i < maxsize); i++) {
        tmpsum += abs(orig.at(i));
    }
    lowest_sum = tmpsum;
    lowest_index = 0;

    for(int j = 0; j < 3; j++) {
        tmpsum = 0;
        for(int i = blocksize*blockindex; (i < blocksize*(blockindex+1)) && (i < maxsize); i++) {
            tmpsum += abs(residues[j].at(i));
        }
        if (tmpsum < lowest_sum) {
            lowest_sum = tmpsum;
            lowest_index = j+1;
        }
    }

    return lowest_index;

}



int Common::blockSumComparison(vector<int> residues[], vector<int>& orig, int blocksize, int blockindex) {
    long long tmpsum = 0;
    long long lowest_sum;
    int lowest_index;
    long maxsize = orig.size();
    for(int i = blocksize*blockindex; (i < blocksize*(blockindex+1)) && (i < maxsize); i++) {
        tmpsum += abs(orig.at(i));
    }
    lowest_sum = tmpsum;
    lowest_index = 0;

    for(int j = 0; j < 3; j++) {
        tmpsum = 0;
        for(int i = blocksize*blockindex; (i < blocksize*(blockindex+1)) && (i < maxsize); i++) {
            tmpsum += abs(residues[j].at(i));
        }
        if (tmpsum < lowest_sum) {
            lowest_sum = tmpsum;
            lowest_index = j+1;
        }
    }

    return lowest_index;

}



int Common::blockSumComparison(vector<int> residues[], int blocksize, int blockindex) {
    long long tmpsum = 0;
    long long lowest_sum;
    int lowest_index;
    long maxsize = residues[0].size();
    for(int i = blocksize*blockindex; (i < blocksize*(blockindex+1)) && (i < maxsize); i++) {
        tmpsum += abs(residues[0].at(i));
    }
    lowest_sum = tmpsum;
    lowest_index = 0;

    for(int j = 1; j < 4; j++) {
        tmpsum = 0;
        for(int i = blocksize*blockindex; (i < blocksize*(blockindex+1)) && (i < maxsize); i++) {
            tmpsum += abs(residues[j].at(i));
        }
        if (tmpsum < lowest_sum) {
            lowest_sum = tmpsum;
            lowest_index = j;
        }
    }

    return lowest_index;

}


map<int, int> Common::residueStats(vector<int> residues[], vector<short> orig, int blocksize) {
    map<int, int> tmpmap;
    int winner;
    int total_blocks = (int) orig.size()/blocksize;
    cout << "Total blocks: " << total_blocks << endl;
    for(int i = 0; i <= total_blocks; i++) {

        winner = blockSumComparison(residues, orig, blocksize, i);
        if (tmpmap.count(winner) == 0) tmpmap[winner] = 0;
        tmpmap[winner] = tmpmap[winner] + 1;
        cout << "Block " << i << " done.\n";
    }

    return tmpmap;
};

map<int, int> Common::residueStats(vector<int> residues[], int blocksize) {
    map<int, int> tmpmap;
    int winner;
    int total_blocks = (int) residues[0].size()/blocksize;
    cout << "Total blocks: " << total_blocks << endl;
    for(int i = 0; i <= total_blocks; i++) {

        winner = blockSumComparison(residues, blocksize, i);
        if (tmpmap.count(winner) == 0) tmpmap[winner] = 0;
        tmpmap[winner] = tmpmap[winner] + 1;
        cout << "Block " << i << " done.\n";
    }

    return tmpmap;
};

map<int, int> Common::residueStats(vector<int> residues[], vector<int> orig, int blocksize) {
    map<int, int> tmpmap;
    int winner;
    int total_blocks = (int) orig.size()/blocksize;
    cout << "Total blocks: " << total_blocks << endl;
    for(int i = 0; i <= total_blocks; i++) {

        winner = blockSumComparison(residues, orig, blocksize, i);
        if (tmpmap.count(winner) == 0) tmpmap[winner] = 0;
        tmpmap[winner] = tmpmap[winner] + 1;
        cout << "Block " << i << " done.\n";
    }

    return tmpmap;
};

vector<int> Common::residueComparison(vector<int> residues[], int blocksize, vector<int>& values) {
    vector<int> lowest_indexes;
    long maxsize = residues[0].size();
    int winner, res;
    //cout << "Comparing residues: ";
    for(int i = 0; i <= maxsize/blocksize; i++) {
        winner = blockSumComparison(residues, blocksize, i);
        lowest_indexes.push_back(winner);
        for(int j = i*blocksize; (j < ((i+1)*blocksize)) && (j < maxsize); j++) {
            res = residues[winner].at(j);
            //if (i == 51) cout << res << " ";
            values.push_back(res);
        }
    }
    //cout << endl;
    return lowest_indexes;
};


vector<int> Common::residueComparison(vector<int> residues[], vector<short> orig, int blocksize, vector<int>& values) {
    vector<int> lowest_indexes;
    long maxsize = orig.size();
    int winner;
    for(int i = 0; i <= orig.size()/blocksize; i++) {
        winner = blockSumComparison(residues, orig, blocksize, i);
        lowest_indexes.push_back(winner);
        for(int j = i*blocksize; (j < ((i+1)*blocksize)) && (j < maxsize); j++) {
            values.push_back(winner == 0 ? orig.at(j) : residues[winner - 1].at(j));
        }
    }
    return lowest_indexes;
};

vector<int> Common::residueComparison(vector<int> residues[], vector<int>& orig, int blocksize, vector<int>& values) {
    vector<int> lowest_indexes;
    long maxsize = orig.size();
    int winner;
    for(int i = 0; i <= orig.size()/blocksize; i++) {
        cout << "Processing block " << i << "/" << orig.size()/blocksize << endl;
        winner = blockSumComparison(residues, orig, blocksize, i);
        lowest_indexes.push_back(winner);
        for(int j = i*blocksize; (j < ((i+1)*blocksize)) && (j < maxsize); j++) {
            values.push_back(winner == 0 ? orig.at(j) : residues[winner - 1].at(j));
        }
    }
    return lowest_indexes;
};

vector<int> Common::sliceVector(vector<int>& input, long start, long end) {
    vector<int> output;
    int in;
    while(start <= end) {
        in = input.at(start++);
        output.push_back(in);
    }
    return output;
}

vector<int> Common::findBestPartitionNumber(vector<int>& input, int bs, long *estimatedsize_out, int *factor_out) {
    long last_size = LONG_MAX;
    long actual_size = -1;
    int factor = 0;
    long partition_size;
    vector<int> partition;
    vector<int> tmp_output;
    vector<int> best_output;
    int number_of_partitions;
    long max_slice, best_m_size;
    int best_m;;
    while(actual_size < last_size) {
        if(actual_size < last_size && actual_size != -1) {
            best_output = tmp_output;
            last_size = actual_size;
        }

        tmp_output.clear();
        actual_size = 0;
        factor++;
        partition_size = (bs*factor);
        number_of_partitions = (int) ceil((input.size()*1.0)/partition_size);


        for (int i = 0; i < number_of_partitions; i++) {
            max_slice = (i + 1) * partition_size;
            if (max_slice >= input.size())
                partition = sliceVector(input, i * partition_size, input.size() - 1);
            else partition = sliceVector(input, i * partition_size, max_slice - 1);
            //cout << "Slice with index: " << i * partition_size << " to " << max_slice - 1 << endl;
            best_m_size = 0;
            best_m = findBestM(partition, bs, &best_m_size);
            tmp_output.push_back(best_m);
            actual_size += best_m_size;
            //totalsamples += partition.size();
        }
        actual_size += (number_of_partitions * 8);
        //cerr << "Attempting " << factor << ": " << actual_size/8 << endl;
    }
    *factor_out = --factor;
    *estimatedsize_out = last_size;
    return best_output;

}

