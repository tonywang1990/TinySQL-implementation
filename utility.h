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
#include<cmath>
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
#include <queue>
#include<ctype.h>
#include<cassert>
#include<queue>
#include<utility>


using namespace std;

// list of node types
enum TYPE{SELECT, PROJECT, PRODUCT, JOIN, THETA, DISTINCT, SORT, LEAF, HEAD}; 

// Node class
class Node{
public:
	TYPE type;
	vector<string> param;
	Relation* view;
	vector<Node*> children;
	int level;
        Node(TYPE t, vector<string> p, int l): type(t), param(p), view(NULL), level(l){
	}
        Node(TYPE t): type(t), view(NULL), level(0){
	}
	bool isChildrenLoaded(){
	  for(int i = 0; i < children.size(); ++i){
	    assert(children[i]);
	    if(!(children[i]->view))  return false;
	  }
	  return true;
	}
};


// free blocks in memory
extern queue<int> free_blocks;
// map of type and its name
extern map<TYPE, string> T;


// global utiltity functions
void initMapT();

string strip(string &str);

string to_string(int i);

vector<int> getNeededFields(const Schema & old, const vector<string>& conditions);

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



// wrapper for the operation evaluation 
struct myCompare{
	bool operator()(const Tuple& l, const Tuple& r){
		if(l.getNumOfFields() != r.getNumOfFields()) return false;
		string s1, s2;
		for(int i = 0; i < l.getNumOfFields(); i++){
			union Field f1 = l.getField(i);
			union Field f2 = r.getField(i);

			if(l.getSchema().getFieldType(i) != r.getSchema().getFieldType(i)){
				cerr<<"Fatal error! tuples with different schema!!"<<endl;
				exit(EXIT_FAILURE);
			}
			if(l.getSchema().getFieldType(i) == 0){
			  s1 += to_string(f1.integer);
			  s2 += to_string(f2.integer);
			}
			else{
			  s1 += *f1.str;
			  s2 += *f2.str;
			}
		}
		return s1 < s2;
		//		return true;
	}
};


class Eval{
	private:
		vector<string> m_conditions;
		bool evalCond(const Tuple &left, const Tuple &right, bool isUnary);
		int opType(string x);
		string evalField(string name, const vector<Tuple> &tuples);
		string findVal(string name, const Tuple &T);
	public:
		Eval(const vector<string>& conditions);
		bool evalUnary(const Tuple & tuple);
		bool evalBinary(const Tuple & lt, const Tuple & rt);
};


#endif
