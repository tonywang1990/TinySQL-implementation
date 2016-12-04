#include "utility.h"
#include "algorithm.h"
#include<algorithm>
using namespace std;

// the heap block class:
HeapBlock::HeapBlock(Relation * relation,int start, int mm_ind):
  relation_ptr(relation),
  dStart(start),
  dEnd(0),
  block_ptr(NULL),
  mm_index(mm_ind),
  tuple_cnt(0)
{
  int dBlocks = relation_ptr->getNumOfBlocks();
  dEnd = dStart + 10 < dBlocks ? dStart + 10 : dBlocks; // set end of the disk block range
  assert(relation_ptr);

  max_tuple_cnt = relation->getSchema().getTuplesPerBlock();
  
}

bool HeapBlock::popOneOut(){
  if(tuple_cnt == max_tuple_cnt) return true;
  tuple_cnt++;
  if(tuple_cnt == max_tuple_cnt) return true;
  else return false;
}

vector<Tuple> HeapBlock::getTuples(MainMemory & mem)
{
  if(dStart < dEnd){
    relation_ptr->getBlock(dStart++, mm_index);
    block_ptr = mem.getBlock(mm_index);
    vector<Tuple> tuples = block_ptr->getTuples();
    tuple_cnt = 0;

    assert(block_ptr);
    return tuples;
  }
  else 
    return vector<Tuple>();
}

// wrapper for heap manager:
HeapManager::HeapManager(Relation * r, MainMemory &mm, priority_queue<pair<int, pair<Tuple, int> >, vector<pair<int, pair<Tuple, int> > >, pqCompare> h, const vector<int>& diskHeadPtrs, int field):
relation_ptr(r),
heap(h),
sublists(diskHeadPtrs.size()),
sField(field)	  
{
  _init(diskHeadPtrs, mm);
}

void HeapManager::_init(const vector<int>& diskHeadPtrs, MainMemory& mem)
{
  resetFreeBlocks();
  
  assert(diskHeadPtrs.size() == sublists);
  // 1. create a vector of heap blocks:
  for(int i = 0; i < sublists; ++i){
    assert(!free_blocks.empty());
    int memory_block_index = free_blocks.front();
    free_blocks.pop();
    heap_blocks.push_back(HeapBlock(relation_ptr, diskHeadPtrs[i], memory_block_index));
  }
  
  // 2. for each heap blocks, read one blocks from disk through mem into the heap
  for(int i = 0; i < heap_blocks.size(); ++i){
    HeapBlock & heap_block = heap_blocks[i];
    if(!heap_block.isExhausted()){
      vector<Tuple> tuples = heap_block.getTuples(mem);
      // put those tuples into the heap 
      putTuplesToHeap(tuples, i);
    }
  }
}

// put the tuples into the heap:
void HeapManager::putTuplesToHeap(const vector<Tuple>& tuples, int hp_block_index)
{
  for(int i = 0; i < tuples.size(); ++i){
    // index of the heap block - tuple - selected sort field:
    heap.push(make_pair(hp_block_index, make_pair(tuples[i], sField)));
  }
}

Tuple HeapManager::top(){
  return heap.top().second.first;
}

void HeapManager::pop(MainMemory & mem){
  pair<int, pair<Tuple, int> > p = heap.top();
  heap.pop();
  Tuple & tuple = p.second.first;
  int hp_block_index = p.first;
  
  assert(hp_block_index >= 0 && hp_block_index < heap_blocks.size());

  HeapBlock & heap_block = heap_blocks[hp_block_index];
  if(heap_block.popOneOut() && !heap_block.isExhausted()){ // last one poped out from hb
    vector<Tuple> tuples = heap_block.getTuples(mem);
    putTuplesToHeap(tuples, hp_block_index);
  }
}
Algorithm::Algorithm(bool isOnePass, const vector<string>& conditions, TYPE type, int level):
m_isOnePass(isOnePass), 
m_conditions(conditions), 
m_type(type),
m_level(level)
{
}

Relation * Algorithm::runUnary(Relation * relation_ptr, MainMemory & mem, SchemaManager & schema_mgr)
{
  assert(relation_ptr);
  if(T[m_type] == "PROJECT" && m_conditions.size() == 1 && m_conditions[0] == "*"){
    return relation_ptr;
  }

  // create a new table for the output:
  string new_relation_name = relation_ptr->getRelationName() + T[m_type] + to_string(m_level);
  Schema schema = getNewSchema(relation_ptr);
  Relation * newRelation = schema_mgr.createRelation(new_relation_name, schema);

  if(T[m_type] == "SELECT"){
    Select(relation_ptr, newRelation, mem);
  }
  else if(T[m_type] == "PROJECT"){
    vector<int> indices = getNeededFields(relation_ptr->getSchema(), m_conditions);
    Project(relation_ptr, newRelation, mem, indices);
  }
  else if(T[m_type] == "DISTINCT"){
    if(m_isOnePass)
      distinctOnePass(relation_ptr, newRelation, mem);
    else
      newRelation = relation_ptr; // need to do later
  }
  else if(T[m_type] == "SORT"){
    if(m_isOnePass)
      sortOnePass(relation_ptr, newRelation, mem);
    else
      sortTwoPass(relation_ptr, newRelation, mem, schema_mgr);
  }
  else{
    cerr<<"Unsupport unary operation! "<<m_type<<endl;
    exit(EXIT_FAILURE);
  }
  return newRelation;
}

// get the new schema for projection
Schema Algorithm::getNewSchema(Relation * relation_ptr){
  if(T[m_type] != "PROJECT")  return relation_ptr->getSchema();

  // get the needed schema:
  Schema old = relation_ptr->getSchema();
  vector<int> indices = getNeededFields(old, m_conditions);

  vector<string> fieldNames = relation_ptr->getSchema().getFieldNames();
  vector<enum FIELD_TYPE> fieldTypes = relation_ptr->getSchema().getFieldTypes();
  assert(fieldNames.size() == fieldTypes.size());

  vector<string> myFieldNames;
  vector<enum FIELD_TYPE> myFieldTypes;
  assert(myFieldNames.size() == myFieldTypes.size());
  for(int i = 0; i < indices.size(); ++i){
    myFieldNames.push_back(fieldNames[indices[i]]);
    myFieldTypes.push_back(fieldTypes[indices[i]]);
    
  }
  
  if(myFieldNames.empty()){
    cerr<<"Warning: no available projection attributes!"<<endl;
  }
  Schema mySchema = Schema(myFieldNames, myFieldTypes);
  return mySchema;

}


void Algorithm::Select(Relation * oldR, Relation * newR, MainMemory& mem){
  assert(!free_blocks.empty());
  // get a available mem block
  int memory_block_index = free_blocks.front();
  free_blocks.pop();
  
  int dBlocks = oldR->getNumOfBlocks();
  int size = 0;

  Eval evaluate = Eval(m_conditions);
  Block * block_ptr = NULL;
  while(size < dBlocks){
    // read the relatioin block by block
    oldR->getBlock(size, memory_block_index);
    block_ptr = mem.getBlock(memory_block_index);
    assert(block_ptr);

    vector<Tuple> tuples = block_ptr->getTuples();
    if(tuples.empty()){
      cerr<<"Warning: No tuples in the current mem block!"<<endl;
    }
    // pick up the desired ones
    for(int i = 0; i < tuples.size(); ++i){
      if(evaluate.evalUnary(tuples[i]))  appendTupleToRelation(newR, mem, tuples[i]);
    }
    size++;
  }
  free_blocks.push(memory_block_index);

}

void Algorithm::Project(Relation * oldR, Relation * newR, MainMemory& mem, vector<int> indices){
  assert(!free_blocks.empty());
  if(indices.empty()){
    cerr<<"Cannot find the needed fields for projection!"<<endl;
    exit(EXIT_FAILURE);
  }

  // get a available mem block
  int memory_block_index = free_blocks.front();
  free_blocks.pop();
  
  int dBlocks = oldR->getNumOfBlocks();
  int size =  0;
  Block * block_ptr = NULL;

  while(size < dBlocks){
    // read the relatioin block by block
    oldR->getBlock(size, memory_block_index);
    block_ptr = mem.getBlock(memory_block_index);
    assert(block_ptr);

    vector<Tuple> tuples = block_ptr->getTuples();
    if(tuples.empty()){
      cerr<<"Warning: No tuples in the current mem block!"<<endl;
    }

    for(int i = 0; i < tuples.size(); ++i){
      Tuple t = tuples[i];
      Tuple tmp = newR->createTuple();
      for(int j = 0; j < indices.size(); ++j){
	if(tmp.getSchema().getFieldType(j) == 0)
	  tmp.setField(j, t.getField(indices[j]).integer);
	else
	  tmp.setField(j, *(t.getField(indices[j]).str));
      }
      
      appendTupleToRelation(newR, mem, tmp);
    }
    size++;
  }
  free_blocks.push(memory_block_index);

}

// idea: the main memory is large enough, simply reading them in and do distinct
void Algorithm::distinctOnePass(Relation * oldR, Relation * newR, MainMemory & mem){
  assert(!free_blocks.empty());
  // get a available mem block
  int memory_block_index = free_blocks.front();
  free_blocks.pop();

  int size =  oldR->getNumOfBlocks()-1;

  assert(size < mem.getMemorySize()); // doing one pass here!

  Block * block_ptr = NULL;

  while(size >= 0){
    // read the relatioin block by block
    oldR->getBlock(size, memory_block_index);
    block_ptr = mem.getBlock(memory_block_index);
    assert(block_ptr);

    vector<Tuple> tuples = block_ptr->getTuples();
    if(tuples.empty()){
      cerr<<"Warning: DistinctOnPass, No tuples in the current mem block!"<<endl;
    }

    for(int i = 0; i < tuples.size(); ++i){
      Tuple t = tuples[i];
      if(m_set.find(t) == m_set.end()){
	m_set.insert(t);
	appendTupleToRelation(newR, mem, t);
      }
    }
    size--;
  }
  free_blocks.push(memory_block_index);

}
void Algorithm::distinctTwoPass(Relation * oldR, Relation * newR, MainMemory & mem){  
  newR = oldR;
  return;
}

// return the indices of the overlapped fields between the given condition and schema
int Algorithm::getNeededOrder(Relation * relation_ptr){
  // get the schema:
  Schema schema = relation_ptr->getSchema();
  // get the overlapped fields between the given order conditions and the schema fields
  vector<int> indices = getNeededFields(schema, m_conditions);
  if(indices.size() != 1){
    cerr<<"Fatal error: The relation can only be ordered by one attribute!!"<<endl;
    exit(EXIT_FAILURE);
  }

  return indices[0];
}


// read the oldR from given disk index and bring them into the main memory 
// until the mm is full or we exhaust the oldR
// sort these tuples in the memory and write them back to the disk from the start index
// @ret: the total number of blocks read and write back to disk
int Algorithm::sortByChunkWB(Relation * oldR, Relation * newR, MainMemory & mem, int start){
  assert(!free_blocks.empty());
  // get a available mem block
  resetFreeBlocks();

  Block * block_ptr = NULL;
  int dBlocks = oldR->getNumOfBlocks();

  int cnt = 0; // how many disk block you bring into the main memory at the end

  // get the overlapped field (only have one) between the order and the schema for later key forming!
  int indField = getNeededOrder(oldR);

  // bring the disk blocks into the main memory until mm is full or we have read
  // all the disk blocks
  vector<pair<int, Tuple> > sortListInt; // field->tuple
  vector<pair<string, Tuple> > sortListStr;

  for(int i = start; i < dBlocks && cnt < mem.getMemorySize(); i++, cnt++){
      assert(!free_blocks.empty());
      int memory_block_index = free_blocks.front();
      free_blocks.pop();

      // read the relation block by block
      oldR->getBlock(i, memory_block_index);
      block_ptr = mem.getBlock(memory_block_index);
      vector<Tuple> tuples = block_ptr->getTuples();
      if(tuples.empty()){
	cerr<<"Warning: sortByChunkWB, No tuples in the current mem block!"<<endl;
      }

      // push the tuples into the list:
      for(int j = 0; j < tuples.size(); ++j){
	union Field f = tuples[j].getField(indField);

	if(tuples[j].getSchema().getFieldType(indField) == 0) // if is int
	  sortListInt.push_back(make_pair(f.integer, tuples[j]));
	else
	  sortListStr.push_back(make_pair(*f.str, tuples[j]));
      }
      
  }
  
  assert((!sortListInt.empty() && sortListStr.empty()) || (sortListInt.empty() && !sortListStr.empty()));

  // sort the tuples and bring them back to the disk
  if(!sortListInt.empty())  sort(sortListInt.begin(), sortListInt.end(), mySortCompareInt); 
  if(!sortListStr.empty())  sort(sortListStr.begin(), sortListStr.end(), mySortCompareStr);
  resetFreeBlocks();
  
  
  for(int i = 0; i < sortListInt.size(); ++i){
    // always append to the end of the new relation!
    appendTupleToRelation(newR, mem, sortListInt[i].second); 
  }
	 
  for(int i = 0; i < sortListStr.size(); ++i){
    // always append to the end of the new relation!
    appendTupleToRelation(newR, mem, sortListStr[i].second); 
  }

  return cnt;
}

void Algorithm::sortOnePass(Relation * oldR, Relation * newR, MainMemory & mem){

  int dBlocks=  oldR->getNumOfBlocks();
  assert(dBlocks <= mem.getMemorySize()); // doing one pass here!

  // read all the disk block (<= 10) into the memory, sort them,
  // and put them back to disk starts from a disk index:
  int cnt = sortByChunkWB(oldR, newR, mem, 0);
  assert(cnt == dBlocks);
  return;

}

void Algorithm::sortTwoPass(Relation * oldR, Relation *& newR, MainMemory & mem, SchemaManager& schema_mgr){
  int dBlocks=  oldR->getNumOfBlocks();
  const int mSize = mem.getMemorySize();
  assert(dBlocks > mem.getMemorySize()); // doing one pass here!
  if(sqrt(dBlocks) > double(mSize)){
    cerr<<"Fatal Error: The memory condition: M >= sqrt(B(R)) is not satisfied!! Cannot use the two-passed algorithms!"<<endl;
    return;
  }
  
  int sublists = 0;
  vector<int> subHeads;
  // first pass, make sorted sublists:
  int cnt = 0, start = 0;
  for(start = 0; start < dBlocks; start += cnt){
    subHeads.push_back(start);
    // sort the subchunk first and write back
    cnt = sortByChunkWB(oldR, newR, mem, start);
    sublists++;
  }
  cout<<"Now have you have "<<sublists<<" sublists to merge "<<endl;
  cout<<*newR<<endl;

  // second pass, merge the sorted sublists, note that the to-be-merged one is in the newR  
  int indField = getNeededOrder(oldR); // is supposed to ordered by this field!

  // index in the heap block vector-> tuple-> selected field
  priority_queue<pair<int, pair<Tuple, int> >, vector<pair<int, pair<Tuple, int> > >, pqCompare> pq;
  // this might cause bugs!

  string table_name = oldR->getRelationName(); 
  //schema_mgr.deleteRelation(table_name);
  // get an new table, whose name is the same as the original one
  Relation * newRR = schema_mgr.createRelation(table_name+"SORT", newR->getSchema());
  
  HeapManager heapMgr(newR, mem, pq, subHeads, indField);
  
  while(!heapMgr.empty()){
    Tuple t = heapMgr.top();
    heapMgr.pop(mem);
    appendTupleToRelation(newRR, mem, t);
  }
  
  resetFreeBlocks();

  // very ugly!
  newR = newRR;
  return;
}


Relation * Algorithm::runBinary(Relation * left, Relation * right, MainMemory & mem, SchemaManager & schema_mgr){
  return NULL;
}
