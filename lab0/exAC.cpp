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
	map<char, int> setmap;
	for(int i = 0; i < dataset.size(); i++) {
		setmap[dataset[i]]++;
	}


	int max = 0;

	for(map<char,int>::iterator it = setmap.begin(); it != setmap.end(); ++it) {
		if (setmap[it->first] > max) max = setmap[it->first];
	}

	cout << "\n\n\033[1;32m";
	for(int i = max; i > 0; i--) {
		cout << " ";
		for(map<char,int>::iterator it = setmap.begin(); it != setmap.end(); ++it) {
			if (setmap[it->first] >= i) {
				cout << "* ";
			}
			else cout << "  ";
		}
		cout << "\n";
	}
	
	cout << "\033[0m\n";
 	cout << "\033[1;36m ";

	for(map<char,int>::iterator it = setmap.begin(); it != setmap.end(); ++it) {
		cout << it->first << " ";
	}

	cout << "\n\n";
	cout << "\033[0m\n";


	return 0;

}
