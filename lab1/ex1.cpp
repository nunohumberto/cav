#include <iostream>
#include <string>
#include <math.h>
#include <map>

#define CONTEXT_SIZE 1

using namespace std;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "No data input.\n";
        return 1;
    }

    string dataset(argv[1]);
    map<string, map<char,int> > setmap;

    for(int i = 0; i < dataset.size()-1; i++) {
        string s(1, dataset[i]);
        //string s = "a";
        if(setmap.count(s) == 0){
            setmap[s] = map<char,int>();
            setmap[s][dataset[i+1]] = 0;
        }
        setmap[s][dataset[i+1]]++;
        cout << "Added the key to " << s << endl;
    }



    double state_sum;
    double entropy, state_entropy, prob;
    int total = dataset.size() - CONTEXT_SIZE;
    for(map<string, map<char,int> >::iterator it = setmap.begin(); it != setmap.end(); ++it) {
        state_sum = 0;
        state_entropy = 0;
        for(map<char,int>::iterator inner = setmap[it->first].begin(); inner != setmap[it->first].end(); ++inner) {
            state_sum += setmap[it->first][inner->first];
        }

        for(map<char,int>::iterator inner = setmap[it->first].begin(); inner != setmap[it->first].end(); ++inner) {
            prob = setmap[it->first][inner->first]/state_sum;
            state_entropy += -prob * log2(prob);
        }

        entropy += state_entropy * (state_sum/total);
    }

    return 0;

}
