/*****************************************
 * Physical Query Plan
 * Take in the optimized LQP to execute 
 * it using the already written algorithm
 ******************************************/

#include "utility.h"
#include "pqp.h"
#include "lqp.h"
#include "algorithm.h"
#include<algorithm>

// do this only for delete! fixed tree structure!!!
void generateDPQP(Node * head, SchemaManager& schema_manager, MainMemory& mem){
        if(!head){
		cout<<"The given physical query plan is empty!"<<endl;
		return;
	}
	
	assert(head->children.size() == 1);
	// load the child
	loadRelationPtr(head->children[0], schema_manager);
	// simple do the job!
	head->view = unaryReadWrite(head->children[0], head->param, mem, schema_manager, head->type, head->level);
	
	return;
  
}

// run a reverse-order travseral for the right most lqp
void generatePQP(Node * head, SchemaManager& schema_manager, MainMemory& mem){
	if(!head){
		cout<<"The given physical query plan is empty!"<<endl;
		return;
	}
	reverse_postorder_traverse(head, schema_manager, mem);
	//cout<<*(head->view)<<endl;
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
			N->view = unaryReadWrite(N->children[0], N->param, mem, schema_manager, N->type, N->level);
		}
		else if(N->children.size() == 2){ // binary operation
			N->view = binaryReadWrite(N->children[0], N->children[1], N->param, mem, schema_manager, N->type, N->level);
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
	//cout<<T[N->type]<<endl;

	//cout<<*(N->view)<<endl;

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


Relation * unaryReadWrite(Node *N, const vector<string>& conditions, MainMemory& mem,  SchemaManager& schema_mgr, TYPE opType, int level){
	Relation* relation_ptr = N->view;
	assert(relation_ptr);

	if(conditions.empty() && T[opType] != "DISTINCT"){
		return relation_ptr;
	}

	int dBlocks = relation_ptr->getNumOfBlocks();
	if(dBlocks == 0){
		cerr<<"Warning, when doing the unary operation,  the relation "<<relation_ptr->getRelationName()<<" is empty!"<<endl;
		return relation_ptr;
	}
	bool isOnePass = (T[opType] == "SELECT" || T[opType] == "PROJECT" || T[opType] == "DELETE" || dBlocks < mem.getMemorySize()) ? true : false;

	Algorithm alg(isOnePass, conditions, opType, level);
	Relation * newRelation = alg.runUnary(relation_ptr, mem, schema_mgr, N->type == LEAF);
	assert(newRelation);
	return newRelation;
}


Relation * binaryReadWrite(Node* left_node, Node* right_node, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType, int level){
	Relation* left = left_node->view, *right = right_node->view;
	assert(left && right);

	int dBlocks_left = left->getNumOfBlocks();
	int dBlocks_right = right->getNumOfBlocks();

	if(dBlocks_left == 0 || dBlocks_right == 0){
		// either one of them is empty, handle here
		if(dBlocks_left == 0)
			cerr<<"Warning, when doing the binary operation, the relation "<<left->getRelationName()<<" is empty!"<<endl;
		if(dBlocks_right == 0)
			cerr<<"Warning, when doing the binary operation, the relation "<<right->getRelationName()<<" is empty!"<<endl;
		return dBlocks_left == 0 ? left : right; // always return the empty one
	}

	// TODO: for natural nature, this might not be true
	bool isOnePass = (T[opType] == "PRODUCT") ? true : false;

	Algorithm alg(isOnePass, conditions, opType, level);
	Relation * newRelation = alg.runBinary(left, right, mem, schema_mgr, left_node->type == LEAF, right_node->type == LEAF);
	assert(newRelation);
	return newRelation;
}
