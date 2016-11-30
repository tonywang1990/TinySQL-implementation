#include<iostream>
#include<iterator>
#include<cstdlib>
#include<ctime>
#include<string>
#include "Block.h"
#include "Config.h"
#include "Disk.h"
#include "Field.h"
#include "MainMemory.h"
#include "Relation.h"
#include "Schema.h"
#include "SchemaManager.h"
#include "Tuple.h"

#include<string>
#include<fstream>
#include<sstream>
#include<vector>
#include<ctype.h>
#include "utility.h"



using namespace std;

// An example procedure of appending a tuple to the end of a relation
// using memory block "memory_block_index" as output buffer
void appendTupleToRelation(Relation* relation_ptr, MainMemory& mem, int memory_block_index, Tuple& tuple) {
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
    cout << "Read the last block of the relation into memory block 5:" << endl;
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
}



int main(){
  //=======================Initialization=========================
  cout << "=======================Initialization=========================" << endl;
  
  // Initialize the memory, disk and the schema manager
  MainMemory mem;
  Disk disk;
  cout << "The memory contains " << mem.getMemorySize() << " blocks" << endl;
  cout << mem << endl << endl;
  SchemaManager schema_manager(&mem,&disk);

  disk.resetDiskIOs();
  disk.resetDiskTimer();

  // Another way to time
  clock_t start_time;
  start_time=clock();

  //=======================Read Input=========================
  string line;
  vector<string> words;
  ifstream input("test.txt");
  if (input.is_open()){
	  // for each command line
	  while(getline(input, line)){
		  istringstream iss(line);
		  string word;
		  // extract each word into vector words
		  while (iss >> word){
			  words.push_back(word);
		  }
		  if (words[0] == "CREATE"){
			  vector<string> field_names;
			  vector<enum FIELD_TYPE> field_types;

			  for (int i = 3; i < words.size(); i=i+2){
				  field_names.push_back(strip(words[i]));
				  if (strip(words[i+1]) == "INT"){
					  field_types.push_back(INT);
				  }
				  else{
					  field_types.push_back(STR20);
				  }
			  }

			  Schema schema(field_names,field_types); 

			  string relation_name = words[2];
			  Relation* relation_ptr=schema_manager.createRelation(relation_name,schema);

		  }
		  else if (words[0] == "INSERT"){

		  }
		  else if (words[0] == "DELETE"){

		  }
		  else if (words[0] == "DROP"){

		  }
		  else if (words[0] == "SELECT"){

		  }
		  else{
			  cout<<"SQL command not recognized!"<<endl;
			  cout<<line<<endl;
			  abort();
		  }
		  words.clear();
	  }
  }
  return 0;
}


