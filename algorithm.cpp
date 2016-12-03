#include "utility.h"
#include "algorithm.h"

using namespace std;

Algorithm::Algorithm(bool isOnePass, const vector<string>& conditions, TYPE type, int level):
m_isOnePass(isOnePass), 
m_conditions(conditions), 
m_type(type),
m_level(level)
{
}

Relation * Algorithm::RunUnary(Relation * relation_ptr, MainMemory & mem, SchemaManager & schema_mgr)
{
  assert(relation_ptr);
  if(T[m_type] == "PROJECT" && m_conditions.size() == 1 && m_conditions[0] == "*"){
    return relation_ptr;
  }

  // create a new table for the output:
  string new_relation_name = relation_ptr->getRelationName() + T[m_type] + to_string(m_level);
  Relation * newRelation = schema_mgr.createRelation(new_relation_name, getNewSchema(relation_ptr));

  if(T[m_type] == "SELECT"){
    Select(relation_ptr, newRelation, mem);
  }
  else if(T[m_type] == "PROJECT"){
    vector<int> indices = getNeededFields(relation_ptr->getSchema(), m_conditions);
    Project(relation_ptr, newRelation, mem, indices);
  }
  else if(T[m_type] == "DISTINCT"){
    //Distinct(relation_ptr, newRelation, mem);
    newRelation = relation_ptr;
  }
  else if(T[m_type] == "SORT"){
    Sort(relation_ptr, newRelation, mem);
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
  
  int size =  oldR->getNumOfBlocks()-1;
  Eval evaluate = Eval(m_conditions);
  Block * block_ptr = NULL;
  while(size >= 0){
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
    size--;
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
  
  int size =  oldR->getNumOfBlocks()-1;
  Block * block_ptr = NULL;

  while(size >= 0){
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
    size--;
  }
  free_blocks.push(memory_block_index);

}


void Algorithm::Distinct(Relation * oldPtr, Relation * newPtr, MainMemory & mem){
  set<Tuple, myCompare> m_set;
  
  newPtr = oldPtr;
  return;
}

void Algorithm::Sort(Relation * oldPtr, Relation * newPtr, MainMemory & mem){
  return;
}


Relation * Algorithm::RunBinary(Relation * left, Relation * right, MainMemory & mem, SchemaManager & schema_mgr){
  return NULL;
}
