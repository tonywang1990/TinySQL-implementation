/* Algorithm class for physical query plan*/
#ifndef ALGORITHM
#define ALGORITHM

#include "utility.h"

using namespace std;



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
  Relation * runUnary(Relation * relation_ptr, MainMemory & mem, SchemaManager & schema_mgr);

  // get new schema for projection
  Schema getNewSchema(Relation * relation_ptr);

  void Select(Relation * oldR, Relation * newR, MainMemory& mem);
  void Project(Relation * oldR, Relation * newR, MainMemory& mem, vector<int> indices);

  // for duplicate elimination:
  void distinctOnePass(Relation * oldR, Relation * newR, MainMemory & mem);
  void distinctTwoPass(Relation * oldR, Relation * newR, MainMemory & mem);

  // for sorting:
  int getNeededOrder(Relation * relation_ptr);
  int sortByChunkWB(Relation * oldR, Relation * newR, MainMemory & mem, int start);
  void sortOnePass(Relation * oldR, Relation * newR, MainMemory & mem);
  void sortTwoPass(Relation * oldR, Relation * newR, MainMemory & mem);

  Relation * runBinary(Relation * left, Relation * right, MainMemory & mem, SchemaManager & schema_mgr);

};
#endif
