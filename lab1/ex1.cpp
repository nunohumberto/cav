#include <iostream>
#include <string>
#include <math.h>
#include <map>
#include <algorithm>

#define CONTEXT_SIZE 1
#define RESULT_SIZE 20
#define ORDER 1

using namespace std;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "No data input.\n";
        return 1;
    }

    string dataset(argv[1]);
    map<string, map<char,int> > setmap;

    for(int i = 0; i < dataset.size()-ORDER; i++) {
        string s(1, dataset[i]);
        //string s = "a";
        if(setmap.count(s) == 0){
            setmap[s] = map<char,int>();
            setmap[s][dataset[i+ORDER]] = 0;
        }
        setmap[s][dataset[i+ORDER]]++;
        //cout << "Added the key to " << s << endl;
    }



    double state_sum;
    double entropy, state_entropy, prob;
    int total = dataset.size() - CONTEXT_SIZE;/*
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
    }*/

    string result;

    string c;

    //get first chars in string
    int maxtimes=0;

    map<string, map<char,int> >::iterator it;
    for(it = setmap.begin(); it != setmap.end(); ++it) {
        int num = 0;
        for(map<char,int>::iterator inner = setmap[it->first].begin(); inner != setmap[it->first].end(); ++inner) {
            num += setmap[it->first][inner->first];
        }

        if( num > maxtimes){
            maxtimes = num;
            c = it->first;
        }
    }

    result = result + c;

    // write the rest of generated text with given context
    char b; //char to be added

    while(result.length() < RESULT_SIZE){

        map <char, double> textPercent; //percentage of char in generated text
        bool wfound = false; //context found

        for(int i = 0; i < dataset.size()-ORDER; i++) {
            if(textPercent.count(dataset[i]) == 0){
                size_t n = std::count(result.begin(), result.end(), dataset[i]);
                textPercent[dataset[i]] = n/result.size();
            }
        }

        string w = result.substr(result.length()-ORDER,result.length()); //context

        cout << "Context " << w << endl;

        int maxbnum = 0;

        for (map<char, int>::iterator inner = setmap[w].begin();
             inner != setmap[w].end(); ++inner) {
            int bnum = setmap[w][inner->first];
            if (bnum > maxbnum && bnum/dataset.size() >= textPercent[inner->first]) {
                maxbnum = bnum;
                b = inner->first;
                wfound = true;
            }
        }

        if(!wfound){ //if context not found
            int maxnum = 0;
            int i;
            for(i = 0; i < dataset.length(); ++i){
                size_t ndataset = std::count(dataset.begin(), dataset.end(), dataset[i]);
                double datasetpercent = ndataset/dataset.length();
                if(datasetpercent <= textPercent[dataset[i]] && maxnum < ndataset){
                    maxnum = ndataset;
                    b = dataset[i];
                }
            }
        }

        cout << "Added " << b << endl;
        result = result + b;

        cout << "Result " << result << endl;
    }

    cout << result << endl;

    return 0;

}
