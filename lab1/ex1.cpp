#include <iostream>
#include <string>
#include <map>
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
        if(setmap[s] == nullptr){
            setmap[s] = map<char,int>();
            setmap[s][dataset[i+1]] = 0;
        }
        setmap[s][dataset[i+1]]++;
    }

    /*for(map<char,int>::iterator it = setmap.begin(); it != setmap.end(); ++it) {
        if (setmap[it->first] > max) max = setmap[it->first];
    }*/

    return 0;

}
