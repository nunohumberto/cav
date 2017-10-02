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

   for(map<char,int>::iterator it = setmap.begin(); it != setmap.end(); ++it) {
   	cout << it->first << ": " << setmap[it->first] << "\n";
   }
   return 0;
}
