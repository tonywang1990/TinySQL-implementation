#include "utility.h"
#include<stack>
#include<sstream>

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
};

string strip(string &str){
	string newstr;
	for (int i = 0; i < str.size(); i++){
		if (isalpha(str[i]) || isdigit(str[i]))
			newstr.push_back(str[i]);
	}
	return newstr;
};

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
  Tuple ret(tuple);
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

// evaluate the value of a postfix clause
bool eval(vector<string> postfix, Tuple &left, Tuple &right){
	stack<string> stk;
	const string True = "_#true#_", False = "_#false#_";
	for (int i = 0; i < postfix.size(); i++){
		int type = opType(postfix[i]);
		// is operand
		if (type == 0){
			string val = findValBinary(postfix[i], left, right);
			stk.push(val);
		}
		// is operator
		else{
			string op2 = stk.top();
			stk.pop();
			string op1 = stk.top();
			stk.pop();
			// < >
			if (type == 4){
				int num1 = atoi(op1.c_str());
				int num2 = atoi(op2.c_str());
				bool ans = false;
				if (postfix[i] == "<"){
					ans = num1 < num2;
				}
				else{
					ans = num1 > num2;
				}
				stk.push(ans == true ? True : False);
			}
			// int
			if (type == 3){
				int num1 = atoi(op1.c_str());
				int num2 = atoi(op2.c_str());
				int ans = 0;
				if (postfix[i] == "+"){
					ans = num1 + num2;
				}
				else if (postfix[i] == "-"){
					ans = num1 - num2;
				}
				else{
					ans = num1 * num2;
				}
				stringstream ss;
				ss << ans;
				string answer = ss.str(); 
				stk.push(string(answer));
			}
			// bool
			else if (type == 2){
				bool b1 = op1 == True;
				bool b2 = op2 == True;
				if (postfix[i] == "AND"){
					stk.push(b1 && b2 == 0 ? False : True);
				}
				else{
					stk.push(b1 || b2 == 0 ? False : True);
				}
			}
			// string
			else{
				stk.push(op1 == op2 ? True : False);
			}
		}
	}
	return stk.top() == True;
}

// find operator type
int opType(string x){
	// int -> bool
	if (x == "<" || x == ">"){
		return 4;
	}
	// int -> int
	if (x == "+" || x == "-" || x == "*")
		return 3;
	// bool -> bool
	else if (x == "AND" || x == "OR")
		return 2;
	// string -> bool
	else if (x == "=")
		return 1;
	else 
		return 0;
}

string findValBinary(string name, Tuple &left, Tuple &right){
	const string null = "_#NULL#_";
	string val = null;

	// first pass, use postfix directly
	if (val == null) val = findVal(name, left);
	if (val == null) val = findVal(name, right);

	// second pass, use only the field name
	assert(splitBy(name, ".").size() == 2);
	string field_name = splitBy(name, ".")[1];
	if (val == null) val = findVal(field_name, left);
	if (val == null) val = findVal(field_name, right);

	// assert val is found
	assert(val != null);
	return val;
}

string findVal(string name, Tuple &T){
	int size = T.getNumOfFields();
	for (int i = 0; i < size; i++){
		if (T.getSchema().fieldNameExists(name)){
			union Field F = T.getField(name);
			if (T.getSchema().getFieldType(name) == INT){
				stringstream ss;
				ss << F.integer;
				return ss.str();
			}
			else{
				return *(F.str);
			}
		}
	}
	return "_#NULL#_";
}

