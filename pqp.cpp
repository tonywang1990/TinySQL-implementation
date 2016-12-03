/*****************************************
 * Physical Query Plan
 * Take in the optimized LQP to execute 
 * it using the already written algorithm
 ******************************************/

#include "utility.h"
#include "pqp.h"
#include "lqp.h"
#include<algorithm>


// run a reverse-order travseral for the right most lqp
void generatePQP(Node * head, SchemaManager& schema_manager, MainMemory& mem){
  if(!head){
    cout<<"The given physical query plan is empty!"<<endl;
    return;
  }
  reverse_postorder_traverse(head, schema_manager, mem);
}


void reverse_postorder_traverse(Node * N, SchemaManager& schema_manager, MainMemory& mem){
  assert(N);

  for(int i = N->children.size()-1; i >= 0; --i){
    reverse_postorder_traverse(N->children[i], schema_manager, mem);
  }

  // handle from the parents of the leaf nodes
  if(N->type == LEAF){
    // assign the relation ptr to the leaf node:
    loadRelationPtr(N, schema_manager);
    return;
  }

  if(N->isChildrenLoaded()){
    if(N->children.size() == 1){ // unary operation
      // @param1: relation ptr, @param2: conditions for reading the table
      N->view = unaryReadWrite(N->children[0]->view, N->param, mem, schema_manager, N->type);
    }
    else if(N->children.size() == 2){ // binary operation
      N->view = binaryReadWrite(N->children[0]->view, N->children[1]->view, N->param, mem, schema_manager, N->type);
    }
    else{
      cerr<<"Only binary/unary operation supported! But you have "<<N->children.size()<<"operands! "<<endl;
      exit(EXIT_FAILURE);
    }
  }
  else{
    cerr<<"You want to do the reverse_post_order but the children's relation ptr is not set!!"<<endl;
    exit(EXIT_FAILURE);
  }

}

// given the table name, load the relation ptr via schema manager
void loadRelationPtr(Node * N, SchemaManager& schema_mgr){
  assert(N && N->type == LEAF);
    // the leaf node only has one param which is the relation name!
  assert(N->param.size() == 1);

  string relation_name = N->param[0];
  if(N->view != NULL){
    cerr<<"The child has been loaded with the relation "<<relation_name<<endl;
    assert(!(N->view));
  }

  // do not need to do exception handle here since the library has that already.
  N->view = schema_mgr.getRelation(relation_name);
  assert(N->view);
}


Relation * unaryReadWrite(Relation * relation_ptr, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType){
  assert(relation_ptr);

  if(conditions.empty() && T[opType] != "DISTINCT"){
    return relation_ptr;
  }

  int dBlocks = relation_ptr->getNumOfBlocks();
  if(dBlocks == 0){
    cerr<<"Warning, the relation "<<relation_ptr->getRelationName()<<" is empty!"<<endl;
    return relation_ptr;
  }

  // create a new table for the output:
  string new_relation_name = relation_ptr->getRelationName() + T[opType];// + "," + catVectorToString(relation_ptr->getSchema().getFieldNames(), '.');

  Relation * newRelation = schema_mgr.createRelation(new_relation_name, relation_ptr->getSchema());
  vector<Tuple> ret;

  // unary operation for project/selection/duplicate elimination
  // read the tuples by blocks
  int size = dBlocks-1;

  assert(!free_blocks.empty());
  // get a available mem block
  int memory_block_index = free_blocks.front();
  free_blocks.pop();

  Eval evaluate = Eval(conditions, opType);

  Block * block_ptr;// mem block
  while(size >= 0){
    // read the relatioin block by block
    relation_ptr->getBlock(size, memory_block_index);
    block_ptr = mem.getBlock(memory_block_index);
    assert(block_ptr);

    vector<Tuple> tuples = block_ptr->getTuples();
    if(tuples.empty()){
      cerr<<"Warning: No tuples in the current mem block!"<<endl;
    }
    // pick up the desired ones
    for(int i = 0; i < tuples.size(); ++i){
      bool isPassed = false;
      Tuple tmp = evaluate.evalUnary(tuples[i], isPassed);
      if(isPassed)  ret.push_back(tmp);
    }
    size--;
  }
  free_blocks.push(memory_block_index);

  // write them back together as a whole:
  for(int i = 0; i < ret.size(); ++i){
    appendTupleToRelation(newRelation, mem, ret[i]);
  }

  
  return newRelation;
}


Relation * binaryReadWrite(Relation * left, Relation * right, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType){
  return NULL;
}
