#include <iostream>
#include <string>
#include <map>
using namespace std;

int main() {
   string dataset = "aaab1c94v2416b4v4abc";
   map<char, int> setmap;
   for(int i = 0; i < dataset.size(); i++) {
   	setmap[dataset[i]]++;
   }

   for(map<char,int>::iterator it = setmap.begin(); it != setmap.end(); ++it) {
   	cout << it->first << ": " << setmap[it->first] << "\n";
   }
   return 0;
}
