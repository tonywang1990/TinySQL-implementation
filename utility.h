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

// list of node types
enum TYPE{SELECT, PROJECT, PRODUCT, JOIN, THETA, DISTINCT, SORT, LEAF}; 

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
		for(int i = 0; i < l.getNumOfFields(); i++){
			union Field f1 = l.getField(i);
			union Field f2 = r.getField(i);

			if(l.getSchema().getFieldType(i) != r.getSchema().getFieldType(i))
				return false;
			if(l.getSchema().getFieldType(i) == 0){
				//ut<<"f1: "<<f1.integer<<"\tf2: "<<f2.integer<<endl;
				if(f1.integer == f2.integer)  continue;
				else return f1.integer < f2.integer;
			}
			else{
				if (*(f1.str) == *(f2.str)) continue;
				else return *(f1.str) < *(f2.str);
			}
		}
		return true;
	}
};


class Eval{
	private:
		set<Tuple, myCompare> m_set;
		vector<string> m_conditions;
		TYPE m_type;

	public:
		Eval(const vector<string>& conditions, TYPE type);
		Tuple evalUnary(const Tuple & tuple, bool& isPassed);
		Tuple evalBinary(const Tuple & lt, const Tuple & rt, bool& isPassed);

		bool evalSelect(const Tuple & tuple);
		bool evalDistinct(const Tuple & tuple);
		Tuple evalProject(const Tuple & tuple);

		Tuple doJoin(const Tuple & lt, const Tuple & rt);
		bool evalTheta(const Tuple & tuple);


		bool evalCond(const Tuple &left, const Tuple &right, bool isUnary);
		int opType(string x);
		string evalField(string name, const vector<Tuple> &tuples);
		string findVal(string name, const Tuple &T);
};


#endif
