/* Global utility functions and objects, also defines general inlcude libs*/
#ifndef UTILITY
#define UTILITY

#include<string>
#include<fstream>
#include<sstream>
#include<iostream>
#include<iterator>
#include<cstdlib>
#include<ctime>
#include "Block.h"
#include "Config.h"
#include "Disk.h"
#include "Field.h"
#include "MainMemory.h"
#include "Relation.h"
#include "Schema.h"
#include "SchemaManager.h"
#include "Tuple.h"

#include<vector>
#include<ctype.h>
#include<cassert>
#include<queue>


using namespace std;

extern queue<int> free_blocks;

string strip(string &str);

template <class T>
void print(vector<T> V){
	for (int i = 0; i < V.size(); i++){
		cout<<V[i]<<" ";
	}
	cout<<endl;
};

vector<string> splitBy(string str, string delimiter);

void resetFreeBlocks();

void appendTupleToRelation(Relation* relation_ptr, MainMemory& mem, Tuple& tuple);

#endif
