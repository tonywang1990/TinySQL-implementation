#include "utility.h"

queue<int> free_blocks;
map<TYPE, string> T;

void initMapT(){
	// initialize T map
	T[SELECT] = "SELECT";
	T[PROJECT] = "PROJECT";
	T[PRODUCT] = "PRODUCT";
	T[JOIN] = "JOIN";
	T[THETA] = "THETA";
	T[DISTINCT] = "DISTINCT";
	T[SORT] = "SORT";
	T[LEAF] = "LEAF";
}

string strip(string &str){
	string newstr;
	for (int i = 0; i < str.size(); i++){
		if (isalpha(str[i]) || isdigit(str[i]))
			newstr.push_back(str[i]);
	}
	return newstr;
}

string to_string(int i){
  stringstream ss;
  ss<<i;
  return ss.str();
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

vector<int> getNeededFields(const Schema & old, const vector<string>& conditions){
  vector<int> indices;
  for(int i = 0; i < conditions.size(); ++i){
    int ind = old.getFieldOffset(conditions[i]);
    vector<string> tmp = splitBy(conditions[i], ".");
    if(ind == -1 && tmp.size() == 2){  
      ind = old.getFieldOffset(tmp[1]);
    }
    if(ind == -1){
      cerr<<"Cannot find the field name: "<<conditions[i]<<"!!"<<endl;
      exit(EXIT_FAILURE);
    }
    indices.push_back(i);
  }
  return indices;
}

// AN example procedure of appending a tuple to the end of a relation
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


Eval::Eval(const vector<string> & conditions, TYPE type): m_conditions(conditions), m_type(type){
  if(T.find(type) == T.end()){
    cerr<<"Unsupported operation type: "<<type<<endl;
    exit(EXIT_FAILURE);
  }
}

Tuple Eval::evalUnary(const Tuple & tuple, bool& isPassed){
  Tuple ret = tuple;
  if(T[m_type] == "SELECT"){
    isPassed = evalSelect(tuple);
  }
  else if(T[m_type] == "DISTINCT"){
    isPassed = evalDistinct(tuple);
  }
  else if(T[m_type] == "PROJECT"){
    // different here! a new tuple is generated
    ret = evalProject(tuple);
    isPassed = true;
  }
  else{
    cerr<<"Unsupported unary operation: "<<T[m_type]<<endl;
    exit(EXIT_FAILURE);
  }
  return ret;
}


//TODO: fill in the actual operations!
bool Eval::evalSelect(const Tuple & tuple){
  return true;
}

bool Eval::evalDistinct(const Tuple & tuple){
  if(m_set.find(tuple) == m_set.end()){
    m_set.insert(tuple);
    return true;
  }
  return false;
}

Tuple Eval::evalProject(const Tuple & tuple){
  Tuple ret = Tuple(tuple);
  return ret;
}

Tuple Eval::evalBinary(const Tuple & lt, const Tuple & rt, bool& isPassed){
  Tuple ret(lt);
  if(T[m_type] == "JOIN"){
    ret = doJoin(lt, rt);
    isPassed = true;
  }
  else if(T[m_type] == "THETA"){
    ret = doJoin(lt, rt);
    isPassed = evalTheta(ret);
  }
  else{
    cout<<"Unsupported binary operation: "<<T[m_type]<<endl;
  }
  return ret;
}

// TODO: Do the join operation
Tuple Eval::doJoin(const Tuple & lt, const Tuple & rt){
  return Tuple(lt);
}

bool Eval::evalTheta(const Tuple& tuple){
  return true;
}


// DUMMY
bool Eval::eval(const Tuple & lt, const Tuple & rt, bool isUnary){
  return true;
}

