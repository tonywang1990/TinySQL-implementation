/* Global utility functions */

#include<string>
#include<fstream>
#include<sstream>
#include<vector>
#include<ctype.h>

using namespace std;

string strip(string &str){
	string newstr;
	for (int i = 0; i < str.size(); i++){
		if (isalpha(str[i]) || isdigit(str[i]))
			newstr.push_back(str[i]);
	}
	return newstr;
}

template <class T>
void print(vector<T> V){
	for (int i = 0; i < V.size(); i++){
		cout<<V[i]<<endl;
	}
}

