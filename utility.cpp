#include "utility.h"

queue<int> free_blocks;

string strip(string &str){
	string newstr;
	for (int i = 0; i < str.size(); i++){
		if (isalpha(str[i]) || isdigit(str[i]))
			newstr.push_back(str[i]);
	}
	return newstr;
}



vector<string> splitBy(string str, string delimiter) {
	vector<string> words;
	words.clear();
	int prev = 0, pos;
	while ((pos = str.find_first_of(delimiter, prev)) != string::npos) {
		if (pos > prev) {
			words.push_back(str.substr(prev, pos-prev));
		}
		prev = pos+1;
	}
	if (prev < str.length()) {
		words.push_back(str.substr(prev, string::npos));
	}
	//printVector(words);
	return words;
};

void resetFreeBlocks(){
	while (!free_blocks.empty()){
		free_blocks.pop();
	}
	for (int i = 0; i < NUM_OF_BLOCKS_IN_MEMORY; i++){
		free_blocks.push(i);
	}
};

// An example procedure of appending a tuple to the end of a relation
// using memory block "memory_block_index" as output buffer
void appendTupleToRelation(Relation* relation_ptr, MainMemory& mem, Tuple& tuple) {
	assert(!free_blocks.empty());
	// get a available mem block
	int memory_block_index = free_blocks.front();
	free_blocks.pop();
	Block* block_ptr;
	if (relation_ptr->getNumOfBlocks()==0) {
		cout << "The relation is empty" << endl;
		cout << "Get the handle to the memory block " << memory_block_index << " and clear it" << endl;
		block_ptr=mem.getBlock(memory_block_index);
		block_ptr->clear(); //clear the block
		block_ptr->appendTuple(tuple); // append the tuple
		cout << "Write to the first block of the relation" << endl;
		relation_ptr->setBlock(relation_ptr->getNumOfBlocks(),memory_block_index);
	} else {
		cout << "Read the last block of the relation into memory block:" << endl;
		relation_ptr->getBlock(relation_ptr->getNumOfBlocks()-1,memory_block_index);
		block_ptr=mem.getBlock(memory_block_index);

		if (block_ptr->isFull()) {
			cout << "(The block is full: Clear the memory block and append the tuple)" << endl;
			block_ptr->clear(); //clear the block
			block_ptr->appendTuple(tuple); // append the tuple
			cout << "Write to a new block at the end of the relation" << endl;
			relation_ptr->setBlock(relation_ptr->getNumOfBlocks(),memory_block_index); //write back to the relation
		} else {
			cout << "(The block is not full: Append it directly)" << endl;
			block_ptr->appendTuple(tuple); // append the tuple
			cout << "Write to the last block of the relation" << endl;
			relation_ptr->setBlock(relation_ptr->getNumOfBlocks()-1,memory_block_index); //write back to the relation
		}
	}  
	free_blocks.push(memory_block_index);
};

