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
void generatePQP(Node * head){
	if(!head){
		cout<<"The given physical query plan is empty!"<<endl;
		return;
	}
	reverse_postorder_traverse(head);
}


void reverse_postorder_traverse(Node * N){
	for(int i = N->children.size()-1; i >= 0; --i){
		reverse_postorder_traverse(N->children[i]);
	}

	// handle from the parents of the leaf nodes
	if(N->type == LEAF)
		return;
	for(int = N->children.size()-1; i >= 0; --i){
		if(N->children[i]->type == LEAF){

		}
	}
}

