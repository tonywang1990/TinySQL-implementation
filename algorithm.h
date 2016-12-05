/* Algorithm class for physical query plan*/
#ifndef ALGORITHM
#define ALGORITHM

#include "utility.h"

using namespace std;

// compare for single column based sorting (support two-pass sorting alg)
class pqCompareSingle{   
  public: 
    bool operator()(const pair<int, pair<Tuple, int> >& l, const pair<int, pair<Tuple, int> >& r){
      int ind = l.second.second;
      union Field f1 = l.second.first.getField(ind);
      union Field f2 = r.second.first.getField(ind);
      if(l.second.first.getSchema().getFieldType(ind) == 0)
	return f1.integer > f2.integer;
      else 
	return *f1.str > *f2.str;
    }
};

class pqCompareMultiple{
 public:
    bool operator()(const pair<int, pair<Tuple, string> >& l, const pair<int, pair<Tuple, string> >& r){
      return l.second.second > r.second.second;
    }
};

// wrapper for heap block for the two-pass alg:
// heap block takes care of reading disk blocks from disk to mem and bring out the tuples
class HeapBlock{
 private:
  Relation * relation_ptr;
  int dStart;  // disk block range
  int dEnd;
  Block * block_ptr;
  int mm_index;
  int tuple_cnt;
  int max_tuple_cnt; // the number of the tuples in the disk block

 public:
  HeapBlock(Relation * relation, int start, int mm_ind);
  bool popOneOut();
  bool isExhausted(){return dStart == dEnd;}
  vector<Tuple> getTuples(MainMemory & mem); 
};


// wrapper for taking care of the heap
class HeapManager{
 private:
  Relation * relation_ptr;
  priority_queue<pair<int, pair<Tuple, int> >, vector<pair<int, pair<Tuple, int> > >, pqCompareSingle> heapSg; // for single column based
  priority_queue<pair<int, pair<Tuple, string> >, vector<pair<int, pair<Tuple, string> > >, pqCompareMultiple> heapMp; // for multiple column based

  int sublists;
  int sField; // the index of selected single column, -1 means multiple columns selected
  int isMultiple;
  vector<int> sFields; // indices of selected single/multiple columns

  vector<HeapBlock> heap_blocks;
  void _init(const vector<int>& diskHeadPtrs, MainMemory& mem);

  void putTuplesToHeap(const vector<Tuple>& tuples, int ind);
  Tuple top();
  void pop(MainMemory& mem);

 public:
  HeapManager(Relation * r, MainMemory & mm, const vector<int>& diskHeadPtrs, vector<int> fields);

  vector<Tuple> popTopMins(MainMemory & mem); // return all the min tuples (including the same ones!)
  int size();
  bool empty();
};



// wrapper for algorithm:
class Algorithm{
	private:
		bool m_isOnePass;
		set<Tuple, myCompare> m_set;
		vector<string> m_conditions;
		TYPE m_type;
		int m_level;
		// for sorting only:
		struct sortCompareInt{
			bool operator()(const pair<int, Tuple>& p1,const pair<int, Tuple>& p2){
				return p1.first < p2.first;
			}
		}mySortCompareInt;
		struct sortCompareStr{
			bool operator()(const pair<string, Tuple>& p1,const pair<string, Tuple>& p2){
				return p1.first < p2.first;
			}
		}mySortCompareStr;

	public:
		Algorithm(bool isOnePass, const vector<string>& condition, TYPE type, int level);
		Relation * runUnary(Relation * relation_ptr, MainMemory & mem, SchemaManager & schema_mgr, bool is_leaf);

		
		// get new schema for projection
		Schema getNewSchema(Relation * relation_ptr, bool is_leaf);

		void Select(Relation * oldR, Relation * newR, MainMemory& mem);
		void Project(Relation * oldR, Relation * newR, MainMemory& mem, vector<int> indices);

		// for duplicate elimination:
		void distinctOnePass(Relation * oldR, Relation * newR, MainMemory & mem);
		void distinctTwoPass(Relation * oldR, Relation *& newR, MainMemory & mem, SchemaManager & schema_mgr);

		// for sorting:
		int getNeededOrder(Relation * relation_ptr);
		int sortByChunkWB(Relation * oldR, Relation * newR, MainMemory & mem, int start, vector<int> indices);
		void sortOnePass(Relation * oldR, Relation * newR, MainMemory & mem);
		void sortTwoPass(Relation * oldR, Relation *& newR, MainMemory & mem, SchemaManager & schema_mgr);

		// for join
		Relation * runBinary(Relation * left, Relation * right, MainMemory & mem, SchemaManager & schema_mgr, bool left_is_leaf, bool right_is_leaf);

		void join1Pass(Relation *left, Relation *right, vector<int> left_map, vector<int> right_map, Relation *join, MainMemory& mem);
		// only for natrual join
		void join2Pass(Relation *old_left, Relation *old_right, vector<int> left_map, vector<int> right_map, Relation *join, MainMemory& mem, SchemaManager & schema_mgr);

		// helper functions for binary operations
		map<string, int> findJoinField(); 
		set<string> findDupField(vector<Relation*> relations);
		Schema getJoinSchema(Relation *left, Relation *right, bool left_is_leaf, bool right_is_leaf, vector<vector<int> > &mapping, bool &is_natural);
		vector<int> subSortForJoin(Relation* oldR, Relation* &newR, MainMemory &mem, SchemaManager & schema_mgr, vector<int> indices);
};
#endif
