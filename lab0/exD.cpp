#include <iostream>
#include <string>
#include <limits>
#include <map>
#define NUMBER_OF_BINS 3
using namespace std;

int main(int argc, char* argv[]) {

	/* Check input */ 
	if (argc != 2) {
		cout << "No data input.\n";
		return 1;
	}

	/* Parse input */ 
	string dataset(argv[1]);


	/* Fill the <character>:<occurence count> hashmap */
	map<char, int> setmap;
	for(int i = 0; i < dataset.size(); i++) {
		setmap[dataset[i]]++;
	}


	int graph_height = numeric_limits<int>::min();
	int bin_max[NUMBER_OF_BINS], bin_min[NUMBER_OF_BINS];
	int minimum_char_value = numeric_limits<int>::max();
	int maximum_char_value = numeric_limits<int>::min();


	/* Find out the maximum and minimum decimal values of the input string */
	for(map<char,int>::iterator it = setmap.begin(); it != setmap.end(); ++it) {
		if (int(it->first) > maximum_char_value) maximum_char_value = int(it->first);
		if (int(it->first) < minimum_char_value) minimum_char_value = int(it->first);
	}


	cout << "Number of bins: " << NUMBER_OF_BINS << "\nMinimum: " << minimum_char_value << "\nMaximum: " << maximum_char_value << "\n\n";	


	/* Calculate how many characters each bin should have */
	int bin_range = (maximum_char_value - minimum_char_value + 1) / NUMBER_OF_BINS;

	/* If an even split is impossible, calculate the remainder */
	int remain = (maximum_char_value - minimum_char_value + 1) % NUMBER_OF_BINS;

	if (bin_range == 0) { // It means there are more bins than different characters in the input string
		for(int i = 0; i < NUMBER_OF_BINS; i++) {
			bin_min[i] = bin_max[i] = minimum_char_value + i; // In this case, each bin will only hold a specific character
		}
	}
	else {		// It means there are more different characters than bins
		for(int i = 0; i < NUMBER_OF_BINS; i++) {
			/* In this case, each bin will hold the same number of different characters */
			bin_min[i] = minimum_char_value + bin_range * i;
			bin_max[i] = minimum_char_value + bin_range * (i+1) - 1;
		}
		bin_max[NUMBER_OF_BINS - 1] += remain; // And the last bin will hold the remainder
	}


	/* The following code handles the printing and display of the results */

	for(int i = 0; i < NUMBER_OF_BINS; i++) {
		cout << "Bin " << i << " minimum: " << bin_min[i];
		cout << "\nBin " << i << " maximum: " << bin_max[i];
		cout << endl;

	}

	cout << endl;


	cout << "Input data: \033[1;34m" << dataset << "\033[0m\n";

	for(int i = 0; i < NUMBER_OF_BINS; i++) {
			cout << "These will belong in bin " << i << ": ";
			cout << "\033[1;33m";
			for(int j = bin_min[i]; j <= bin_max[i]; j++) {
				cout << char(j);
			}
			cout << "\033[0m";
			cout << endl;
	}

	int bin_amounts[NUMBER_OF_BINS] = {0};

	/* Iterate through every different character and decide which bin they belong to */
	for(map<char,int>::iterator it = setmap.begin(); it != setmap.end(); ++it) {
			for(int i = 0; i < NUMBER_OF_BINS; i++) {
				if(int(it->first) >= bin_min[i] && int(it->first) <= bin_max[i]) {
					bin_amounts[i] += setmap[it->first];
					break;
				}
			}
	}

	for(int i = 0; i < NUMBER_OF_BINS; i++) {
		if (bin_amounts[i] > graph_height) graph_height = bin_amounts[i];
	}



	cout << "\n\n\033[1;32m";
	for(int i = graph_height; i > 0; i--) {
		cout << " ";
		for(int j = 0; j < NUMBER_OF_BINS; j++) {
			if (bin_amounts[j] >= i) {
				cout << "* ";
			}
			else cout << "  ";
		}
		cout << "\n";
	}
	
	cout << "\033[0m\n";
 	cout << "\033[1;36m ";

	for(int i = 0; i < NUMBER_OF_BINS; i++) {
		cout << i << " ";
	}

	cout << "\n\n";
	cout << "\033[0m\n";


	return 0;

}
