#include <iostream>
#include <algorithm>
#include <string>
#include <math.h>
#include <map>
using namespace std;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "No data input.\n";
        return 1;
    }

    string dataset = "OCRO HLI RGWR NMIELWIS EU LL NBNESEBYA TH EEI ALHENHTTPA OOBTTVA NAH BRL";
    string temp(dataset);
    int size = dataset.size();
    map<char, int> counts;

    for(int i = 0; i < dataset.size(); i++) counts[dataset[i]]++;



    double entropy = 0;
    for(map<char,int>::iterator it = counts.begin(); it != counts.end(); it++) {

        entropy += (counts[it->first]/(double)size) *  (log2(counts[it->first]/(double)size))  ;
        cout << it->first << " -> " << counts[it->first] << " -> " <<  counts[it->first]/(double)size << endl;
    }

    cout << "Entropy: " << -entropy;

    return 0;

}

