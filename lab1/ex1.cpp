#include <iostream>
#include <string>
#include <math.h>
#include <map>
#include <algorithm>
#include <fstream>

#define CONTEXT_SIZE 1
//#define RESULT_SIZE 100

using namespace std;

class FCM {
        string dataset;
        int ORDER;
        map<string, map<char,int> > setmap;
    public:
        FCM(int);
        void addToDataset(char);
        void addToDataset(string);
        void fillInternalMap();
        string generateText(int);
};

FCM::FCM(int order) {
    ORDER = order;
}

void FCM::addToDataset(char c) {
    dataset = dataset + c;
};

void FCM::addToDataset(string c) {
    dataset = dataset + c;
};

void FCM::fillInternalMap() {
    for(int i = 0; i < dataset.size()-ORDER; i++) {
        string s(dataset, i, ORDER);
        //string s = "a";
        if(setmap.count(s) == 0){
            setmap[s] = map<char,int>();
            setmap[s][dataset[i+ORDER]] = 0;
        }
        setmap[s][dataset[i+ORDER]]++;
    }
}

string FCM::generateText(int length) {

    string result;

    string c;

    //get first chars in string
    int maxtimes=0;

    map<string, map<char,int> >::iterator it;
    for (it = setmap.begin(); it != setmap.end(); ++it) {
        //cerr << "Trying " << it->first << endl;
        int num = 0;
        for (map<char, int>::iterator inner = setmap[it->first].begin();
             inner != setmap[it->first].end(); ++inner) {
            num += setmap[it->first][inner->first];
        }

        if (num > maxtimes) {
            maxtimes = num;
            c = it->first;
        }
    }

    result = result + c;

    //cerr << "Preliminary result: " << result << "\n";


    // write the rest of generated text with given context
    char b; //char to be added

    while(result.length() < length){

        map <char, double> textPercent; //percentage of char in generated text
        map<string, map<char,int> > result_setmap;
        bool wfound = false; //context found


        string w = result.substr(result.length()-ORDER,result.length()); //context
        //cout << "Context: '" << w << '\'' << endl;



        for(int i = 0; i < result.size()-ORDER; i++) {
            string s(result, i, ORDER);
            //string s = "a";
            if(result_setmap.count(s) == 0){
                result_setmap[s] = map<char,int>();
                result_setmap[s][result[i+ORDER]] = 0;
            }
            result_setmap[s][result[i+ORDER]]++;
        }


        for(int i = 0; i < dataset.size()-ORDER; i++) {
            if(textPercent.count(dataset[i]) == 0){
                long n = std::count(result.begin(), result.end(), dataset[i]);
                textPercent[dataset[i]] = ((double) n)/result.size();
                //cout << "\t" << dataset[i] << " -> " << textPercent[dataset[i]] << endl;
            }
        }





        int maxbnum = 0;

        for (map<char, int>::iterator inner = setmap[w].begin(); inner != setmap[w].end(); ++inner) {
            int bnum = setmap[w][inner->first];
            double percent_in_result = -1;
            bool only_one_possibility = false;
            if (result_setmap.count(w) > 0 && result_setmap[w].count(inner->first) > 0) {
                percent_in_result = ((double) result_setmap[w][inner->first]) / result.size();
            }
            if (setmap.count(w) > 0 && setmap[w].size() == 1) {
                only_one_possibility = true;
            }
            //cout << "Checking for '" << inner->first << "'\n";
            if (bnum > maxbnum  && ( only_one_possibility || percent_in_result == -1 || ((double) bnum)/dataset.size() >= percent_in_result )) {
                //cout << "Since bnum (" << bnum << ") > maxbnum(" << maxbnum << ") <-- Condition 1 OK!\n";
                //cout << "Since the prob (" << ((double) bnum)/dataset.size() << ") >= textpercent(" << textPercent[inner->first] << ") <-- Condition 2 OK!\n";
                //cout << "Since the prob (" << ((double) bnum)/dataset.size() << ") >= textpercent(" << percent_in_result << ") OR only one possibility: " << only_one_possibility << " <-- Condition 2 OK!\n";

                maxbnum = bnum;
                b = inner->first;
                wfound = true;
            } else {
                //cout << "Since bnum (" << bnum << ") > maxbnum(" << maxbnum << ") <-- Condition 1 may not be OK!\n";
                //cout << "Since the prob (" << ((double) bnum)/dataset.size() << ") >= textpercent(" << textPercent[inner->first] << ") <-- Condition 2 may not be OK!\n";
                //cout << "Since the prob (" << ((double) bnum)/dataset.size() << ") >= textpercent(" << percent_in_result << ") OR only one possibility: " << only_one_possibility << " <-- Condition 2 may not be OK!\n";

            }
        }

        //cout << "wfound = " << wfound << '\n';
        if(!wfound){ //if context not found
            long maxnum = 0;
            int i;
            for(i = 0; i < dataset.length(); ++i){
                long ndataset = std::count(dataset.begin(), dataset.end(), dataset[i]);

                double datasetpercent = ((double) ndataset)/dataset.length();

                //cout << "New candidate: " << dataset[i] << " with ndataset " << ndataset << " and percent " << datasetpercent << ", textpercent " << textPercent[dataset[i]]<< endl;
                if(datasetpercent >= textPercent[dataset[i]] && maxnum < ndataset){
                    maxnum = ndataset;
                    b = dataset[i];
                }
            }
        }

        //cout << "Added " << b << endl;
        result = result + b;

        //cout << "Result: '" << result << '\'' << endl;
    }
    return result;


}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        cout << "Usage: prog <file name> <model order> <desired generated text length>\n";
        return 1;
    }

    string buffer;
    string dataset;
    ifstream input (argv[1]);
    if (input.is_open())
    {
        while (getline(input, buffer))
        {
            dataset = dataset + buffer;
        }
        input.close();
    }

    int order = stoi(argv[2]);
    int len = stoi(argv[3]);


    FCM fcm(order);

    fcm.addToDataset(dataset);
    fcm.fillInternalMap();

    ofstream output;
    string outfilename("output.txt");
    output.open (outfilename);
    output << fcm.generateText(len);
    output.close();

    cout << "Generated text printed to " << outfilename << endl;

    return 0;

}
