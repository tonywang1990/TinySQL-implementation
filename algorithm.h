/* Algorithm class for physical query plan*/
#ifndef ALGORITHM
#define ALGORITHM

#include "utility.h"

using namespace std;

  class pqCompare{   
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

// wrapper for heap manager for the two pass sorted based alg:
class HeapBlock{
 private:
  Relation * relation_ptr;
  int dStart;  // disk block range
  int dEnd;
  Block * block_ptr;
  int mm_index;
  int tuple_cnt;
  int max_tuple_cnt; // the number of the tuples in the disk block

  // bring one disk from mem 

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
  priority_queue<pair<int, pair<Tuple, int> >, vector<pair<int, pair<Tuple, int> > >, pqCompare> heap;
  int sublists;
  int sField; // the selected ordered field
  vector<HeapBlock> heap_blocks;
  void _init(const vector<int>& diskHeadPtrs, MainMemory& mem);
  void putTuplesToHeap(const vector<Tuple>& tuples, int ind);

 public:
  HeapManager(Relation * r, MainMemory & mm, priority_queue<pair<int, pair<Tuple, int> >, vector<pair<int, pair<Tuple, int> > >, pqCompare> h, const vector<int>& diskHeadPtrs, int field);
  Tuple top();
  void pop(MainMemory & mem);
  int size(){return heap.size();}
  bool empty(){return heap.empty();}
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
		void distinctTwoPass(Relation * oldR, Relation * newR, MainMemory & mem);

		// for sorting:
		int getNeededOrder(Relation * relation_ptr);
		int sortByChunkWB(Relation * oldR, Relation * newR, MainMemory & mem, int start);
		void sortOnePass(Relation * oldR, Relation * newR, MainMemory & mem);
		void sortTwoPass(Relation * oldR, Relation *& newR, MainMemory & mem, SchemaManager & schema_mgr);

		Relation * runBinary(Relation * left, Relation * right, MainMemory & mem, SchemaManager & schema_mgr, bool left_is_leaf, bool right_is_leaf);

		map<string, int> findJoinField();
		set<string> findDupField(vector<Relation*> relations);
		Schema getJoinSchema(Relation *left, Relation *right, bool left_is_leaf, bool right_is_leaf, vector<vector<int> > &mapping);
		void join1Pass(Relation *left, Relation *right, vector<int> left_map, vector<int> right_map, Relation *join, MainMemory& mem);
};
#endif
