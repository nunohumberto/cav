#include <iostream>
#include <string>
#include <math.h>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>

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
        void calculateModel();
        void printContextStatistics();
        void printContextStatistics(string);
        void printModel();
        void printEntropy(double, double);
        string generateText(int);
        void dumpModel();
        void loadModel();
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

void FCM::printContextStatistics() {
    if (!(dataset.size() >= ORDER)) {
        cerr << "Not enough data collected.\n";
        return;
    }
    string context = dataset.substr(dataset.length()-ORDER);
    cout << "Actual context: '" << context << "'\n";
    printContextStatistics(context);

}

void FCM::printContextStatistics(string context) {
    if (setmap.count(context) == 0) {
        cout << "Context: '" << context << "' not present in the model.\n";// Exiting.\n";
        //exit(1);
        return;
    }

    cout << "Given the context: '" << context << "', the following occurrences were registered:\n";
    for (map<char, int>::iterator inner = setmap[context].begin(); inner != setmap[context].end(); ++inner) {
        cout << "'" << inner->first << "': " << setmap[context][inner->first] << endl;
    }

    //TODO - print the map inside this entry
}


void FCM::printModel() {
    map<string, map<char,int> >::iterator it;
    cout << "Now printing model information:\n";
    cout << "The following contexts were encountered: ";

    for (it = setmap.begin(); it != setmap.end(); ++it) {
        cout << '\'' << it->first << "\' ";
    }

    cout << endl;



    for (it = setmap.begin(); it != setmap.end(); ++it) {
        cout << "Information for context " << '\'' << it->first << "\':\n";
        for (map<char, int>::iterator inner = setmap[it->first].begin(); inner != setmap[it->first].end(); ++inner) {
            cout << "\t'" << inner->first << "' -> " << setmap[it->first][inner->first] << "\n";
        }
    }

}

void FCM::calculateModel() {
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

void FCM::dumpModel() {

    ofstream output;
    string outfilename("model.txt");
    output.open (outfilename);

    map<string, map<char,int> >::iterator it;



    for (it = setmap.begin(); it != setmap.end(); ++it) {
        output << it->first << "\t";
        for (map<char, int>::iterator inner = setmap[it->first].begin(); inner != setmap[it->first].end(); ++inner) {
            output << inner->first << "=" << setmap[it->first][inner->first] << ";";
        }
        output << "\n";
    }

    output.close();

}

void FCM::loadModel() {
    string buffer;
    ifstream input ("model.txt");
    map<string, map<char,int> > tempMap;

    if (input.is_open())
    {
        while (getline(input, buffer))
        {
            istringstream ss(buffer);
            std::string slice;
            std::string slice2;
            vector<string> vec1;
            while(std::getline(ss, slice, '\t')) {
                vec1.push_back(slice);
            }
            istringstream ss1(vec1[1]);
            while(std::getline(ss1, slice, ';')) {
                istringstream ss3(slice);
                vector<string> vec2;
                while(std::getline(ss3, slice2, '=')) {
                    vec2.push_back(slice2);
                }
                if(tempMap.count(vec1[0]) == 0) tempMap[vec1[0]] = map<char,int>();
                tempMap[vec1[0]][vec2[0][0]] = stoi(vec2[1]);
            }
        }
        input.close();
    }

    setmap = tempMap;

}

void FCM::printEntropy(double alpha, double alpha2) {

    double entropy = 0;

    double global_sum = 0;

    map<string, map<char,int> >::iterator it;

    for (it = setmap.begin(); it != setmap.end(); ++it) {
        for (map<char, int>::iterator inner = setmap[it->first].begin(); inner != setmap[it->first].end(); ++inner) {
            global_sum += setmap[it->first][inner->first];
            global_sum += alpha;
        }
    }

    for (it = setmap.begin(); it != setmap.end(); ++it) {
        double sum = 0, sum_no_alpha = 0;
        for (map<char, int>::iterator inner = setmap[it->first].begin(); inner != setmap[it->first].end(); ++inner) {
            sum += setmap[it->first][inner->first];
            sum += alpha2;
            sum_no_alpha += setmap[it->first][inner->first];
        }

        double PSi = (sum_no_alpha+alpha)/global_sum;

        double state_entropy = 0;

        for (map<char, int>::iterator inner = setmap[it->first].begin(); inner != setmap[it->first].end(); ++inner) {
            double pbw = (setmap[it->first][inner->first]+alpha2)/sum;
            state_entropy += (- pbw * log2(pbw));
        }

        entropy += (PSi * state_entropy);

    }


    double Po = 0;
    map <char, int> unique;
    for(int i = 0; i< dataset.size() ; i++) {
        if(unique.count(dataset[i])) unique[i] = 0;
        unique[dataset[i]]++;
    }

    double powpow = pow(unique.size(), ORDER);

    double no = powpow - setmap.size();
    Po = (no * alpha)/ (global_sum+alpha*powpow);


    entropy += (Po * log2(powpow));


    cout << "Entropy: " << entropy << endl;

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


        // filling the result setmap
        for(int i = 0; i < result.size()-ORDER; i++) {
            string s(result, i, ORDER);
            //string s = "a";
            if(result_setmap.count(s) == 0){
                result_setmap[s] = map<char,int>();
                result_setmap[s][result[i+ORDER]] = 0;
            }
            result_setmap[s][result[i+ORDER]]++;
        }

        // updating textPercent
        for(int i = 0; i < dataset.size()-ORDER; i++) {
            if(textPercent.count(dataset[i]) == 0){
                long n = std::count(result.begin(), result.end(), dataset[i]);
                textPercent[dataset[i]] = ((double) n)/result.size();
                //cout << "\t" << dataset[i] << " -> " << textPercent[dataset[i]] << endl;
            }
        }


        int maxbnum = 0;

        // producing the next char
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

        // if context not found
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
    return result + "\n";


}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        cout << "Usage: input prog <file name> <model order> <desired generated text length>\n";
        return 1;
    }

    string buffer;
    string dataset;
    ifstream input (argv[1]);
    if (input.is_open())
    {
        while (getline(input, buffer))
        {
            buffer.erase(std::remove(buffer.begin(), buffer.end(), ','), buffer.end());
            buffer.erase(std::remove(buffer.begin(), buffer.end(), '\t'), buffer.end());
            buffer.erase(std::remove(buffer.begin(), buffer.end(), ';'), buffer.end());
            transform(buffer.begin(), buffer.end(), buffer.begin(), ::tolower);
            dataset = dataset + buffer;
        }
        input.close();
    }

    int order = std::stoi(argv[2]);
    int len = std::stoi(argv[3]);

    if (dataset.size() <= order) {
        cout << "Input text too short for the selected order.\n";
        exit(1);
    }



    FCM fcm(order);


    cout << "Now adding input to dataset.\n";
    fcm.addToDataset(dataset);

    cout << "Now generating the model.\n";
    fcm.calculateModel();

    cout << "Now dumping and reloading model to/from file.\n";
    fcm.dumpModel();
    fcm.loadModel();

    fcm.printModel();



    cout << "Now printing actual context statistics.\n";
    fcm.printContextStatistics();

    string context_test_value;
    cout << "Input an example context to obtain statistics (order " << order << "): ";
    cin >> context_test_value;
    fcm.printContextStatistics(context_test_value);

    double alpha, alpha2;
    cout << "Input an alpha to calculate the global entropy for each context: ";
    cin >> alpha;
    cout << "Input an alpha to calculate the probability of each character in a context: ";
    cin >> alpha2;

    fcm.printEntropy(alpha, alpha2);

    ofstream output;
    string outfilename("output.txt");
    output.open (outfilename);
    output << fcm.generateText(len);
    output.close();

    cout << "Generated text printed to " << outfilename << endl;

    return 0;

}
