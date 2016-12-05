#include "utility.h"
#include "algorithm.h"
#include<stack>
#include<set>
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
HeapManager::HeapManager(Relation * r, MainMemory &mm, const vector<int>& diskHeadPtrs, vector<int> fields):
	relation_ptr(r),
	sublists(diskHeadPtrs.size()),
	sFields(fields)
{

  if(sFields.size() == 1){
    isMultiple = false;
    sField = sFields[0];
  }
  else{
    assert(sFields.size() > 1);
    isMultiple = true;
    sField = -1;
  }

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
		// index of the heap block - tuple - selected sort field(s):
	        if(isMultiple){
		  string key = encodeFields(tuples[i], sFields);
	          heapMp.push(make_pair(hp_block_index, make_pair(tuples[i], key)));
                }
		else{
		  heapSg.push(make_pair(hp_block_index, make_pair(tuples[i], sField)));
		}
	}
}

Tuple HeapManager::top(){
  return isMultiple ? heapMp.top().second.first : heapSg.top().second.first; 
}

bool HeapManager::empty(){
  return isMultiple ? heapMp.empty() : heapSg.empty();
}

int HeapManager::size(){
  return isMultiple ? heapMp.size() : heapSg.size();
}

vector<Tuple> HeapManager::popTopMins(MainMemory & mem){
  if(this->empty())  return vector<Tuple>();

  vector<Tuple> tuples(1, this->top());
  this->pop(mem);

  while(!(this->empty()) && compareTuples(this->top(), tuples.back())){
      tuples.push_back(this->top());
      this->pop(mem);
  }
  return tuples;
}

void HeapManager::pop(MainMemory & mem){
        int hp_block_index = -1;
	if(isMultiple){
	  pair<int, pair<Tuple, string> > p = heapMp.top();
	  heapMp.pop();
	  hp_block_index = p.first;
	}
	else{
	  pair<int, pair<Tuple, int> > p = heapSg.top();
	  heapSg.pop();
	  hp_block_index = p.first;
	}
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

Relation * Algorithm::runUnary(Relation * relation_ptr, MainMemory & mem, SchemaManager & schema_mgr, bool is_leaf)
{
	assert(relation_ptr);
	if(T[m_type] == "PROJECT" && m_conditions.size() == 1 && m_conditions[0] == "*"){
		return relation_ptr;
	}

	// create a new table for the output:
	string new_relation_name = relation_ptr->getRelationName() + T[m_type] + to_string(m_level) + to_string(_g_relation_counter++);
	Relation * newRelation = schema_mgr.createRelation(new_relation_name, getNewSchema(relation_ptr, is_leaf));
	assert(relation_ptr && newRelation);

	if(T[m_type] == "SELECT"){
	        Select(relation_ptr, newRelation, mem, false);
	}
	else if(T[m_type] == "PROJECT"){
		vector<int> indices = getNeededFields(relation_ptr->getSchema(), m_conditions);
		Project(relation_ptr, newRelation, mem, indices);
	}
	else if(T[m_type] == "DISTINCT"){
		if(m_isOnePass)
			distinctOnePass(relation_ptr, newRelation, mem);
		else
		        distinctTwoPass(relation_ptr, newRelation, mem, schema_mgr); 
		        // for both two-pass, the new relation is passed by reference!!!
	}
	else if(T[m_type] == "SORT"){
		if(m_isOnePass)
			sortOnePass(relation_ptr, newRelation, mem);
		else
			sortTwoPass(relation_ptr, newRelation, mem, schema_mgr);
	}
	else if(T[m_type] == "DELETE"){
	        Select(relation_ptr, newRelation, mem, true);
	}
	else{
		cerr<<"Unsupport unary operation! "<<m_type<<endl;
		exit(EXIT_FAILURE);
	}
	return newRelation;
}


// get the new schema for projection
Schema Algorithm::getNewSchema(Relation * relation_ptr, bool is_leaf){

	// old schema
	Schema old_schema = relation_ptr->getSchema();
	vector<string> old_names = old_schema.getFieldNames();
	vector<enum FIELD_TYPE> old_types = old_schema.getFieldTypes();

	vector<int> indices;
	if(T[m_type] == "PROJECT"){
		// apply column selection
		indices = getNeededFields(old_schema, m_conditions);
	} 
	else{
		// choose all column
		for (int i = 0; i < old_schema.getNumOfFields(); i++){
			indices.push_back(i);
		}
	}
	// rename if read raw table
	if (is_leaf){
		for(int i = 0; i < indices.size(); ++i){
			old_names[indices[i]] = relation_ptr->getRelationName() + "." + old_names[indices[i]];
		}
	}

	// new schema name/type
	vector<string> new_names;
	vector<enum FIELD_TYPE> new_types;
	for(int i = 0; i < indices.size(); ++i){
		new_names.push_back(old_names[indices[i]]);
		new_types.push_back(old_types[indices[i]]);
	}

	if(new_names.empty()){
		cerr<<"Warning: no available projection attributes!"<<endl;
	}
	Schema new_schema = Schema(new_names, new_types);
	return new_schema;

}

// @param4: will be true for delete!
void Algorithm::Select(Relation * oldR, Relation * newR, MainMemory& mem, bool isInvert){
	assert(!free_blocks.empty());
	// get a available mem block
	int memory_block_index = free_blocks.front();
	free_blocks.pop();
	int dBlocks =oldR->getNumOfBlocks(); 
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
		for(int i = 0; i < tuples.size(); ++i){
			Tuple t = tuples[i];
			if (evaluate.evalUnary(t) == isInvert) continue;
			Tuple tmp = newR->createTuple();
			for(int j = 0; j < tmp.getSchema().getNumOfFields(); ++j){
				if(tmp.getSchema().getFieldType(j) == 0)
					tmp.setField(j, t.getField(j).integer);
				else
					tmp.setField(j, *(t.getField(j).str));
			}

			appendTupleToRelation(newR, mem, tmp);
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

	int dBlocks =  oldR->getNumOfBlocks();
	int size = 0;

	assert(dBlocks <= mem.getMemorySize()); // doing one pass here!

	Block * block_ptr = NULL;

	while(size < dBlocks){
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
		size++;
	}
	free_blocks.push(memory_block_index);

}

// using sort-based two pass alg:
void Algorithm::distinctTwoPass(Relation * oldR, Relation * & newR, MainMemory & mem, SchemaManager & schema_mgr){  
	int dBlocks=  oldR->getNumOfBlocks();
	const int mSize = mem.getMemorySize();
	assert(dBlocks > mSize); // doing one pass here!
	if(sqrt(dBlocks) > double(mSize)){
	        cerr<<"Fatal Error: The memory condition: M >= sqrt(B(R)) is not satisfied!! Cannot use the two-passed algorithms!"<<endl;
		return;
	}

	vector<int> inds; // indices of all the columns of the tuple
	if(m_conditions.empty()){ 
	  // distinct order will be set if sort exist in lpq
	  // then we should use that order!
	  for(int i = 0; i < newR->getSchema().getNumOfFields(); ++i)  inds.push_back(i);
	}

	int sublists = 0;
	vector<int> subHeads;
	// first pass, make sorted sublists:
	int cnt = 0, start = 0;
	for(start = 0; start < dBlocks; start += cnt){
		subHeads.push_back(start);
		// sort the subchunk first and write back, now sort with full field!
		cnt = sortByChunkWB(oldR, newR, mem, start, inds);
		sublists++;
	}
	//cout<<"Now have you have "<<sublists<<" sublists to merge for distinct"<<endl;
	//cout<<*newR<<endl;

	// second pass, merge the sorted sublists:
	string table_name = oldR->getRelationName(); 
	Relation * newRR = schema_mgr.createRelation(table_name+"DISTINCT", newR->getSchema());
	
	// handle exception 2 in lpq that is if sort exists, stick with the same order
	// inds is expected to be empty there is some condition

	// to be careful about deleting the relations!
	if(inds.empty())  inds.push_back(getNeededOrder(newR));
	
	// @param1:relation ptr, @param2: mem, @param3: head of the sublists, 
	// @param4: the vector of multiple/single target sort column
	HeapManager heapMgr(newR, mem, subHeads, inds);

	while(!heapMgr.empty()){
	  vector<Tuple> tuples = heapMgr.popTopMins(mem); 
	  appendTupleToRelation(newRR, mem, tuples[0]);
	}
	resetFreeBlocks();

	// very ugly!
	newR = newRR;
	return;
} 

// return the indices of the overlapped fields between the given condition and schema
// ONLY for sort single column!!
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
int Algorithm::sortByChunkWB(Relation * oldR, Relation * newR, MainMemory & mem, int start, vector<int> indices){
	assert(!free_blocks.empty());
	// get a available mem block
	resetFreeBlocks();

	Block * block_ptr = NULL;
	int dBlocks = oldR->getNumOfBlocks();

	int cnt = 0; // how many disk block you bring into the main memory at the end

	// get the overlapped field between the order and the schema only for SORT!
	// else use the given indices to do sort for DISTINCT/NATURAL JOIN!
	if(indices.empty()){
	  int indField = getNeededOrder(oldR);
	  indices.push_back(indField);
	}
	
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
		  	
			if(indices.size() > 1){ // for mul columns
			        string key = encodeFields(tuples[j], indices);
			        sortListStr.push_back(make_pair(key, tuples[j]));
			}
			else if(tuples[j].getSchema().getFieldType(indices[0]) == INT){
			        union Field f = tuples[j].getField(indices[0]);
				sortListInt.push_back(make_pair(f.integer, tuples[j]));
			}
			else{
			        union Field f = tuples[j].getField(indices[0]);
				sortListStr.push_back(make_pair(*f.str, tuples[j]));
			}
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
	// give a empty vector of pre-selected columns
	int cnt = sortByChunkWB(oldR, newR, mem, 0, vector<int>());
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
		cnt = sortByChunkWB(oldR, newR, mem, start, vector<int>());
		sublists++;
	}
	//cout<<"Now have you have "<<sublists<<" sublists to merge "<<endl;
	//cout<<*newR<<endl;

	// second pass, merge the sorted sublists, note that the to-be-merged one is in the newR  
	int indField = getNeededOrder(oldR); // is supposed to ordered by this field!
	// index in the heap block vector-> tuple-> selected field
	//priority_queue<pair<int, pair<Tuple, int> >, vector<pair<int, pair<Tuple, int> > >, pqCompare> pq;
	

	string table_name = oldR->getRelationName(); 
	//schema_mgr.deleteRelation(table_name); // this might cause bugs!

	// get an new table, whose name is the same as the original one
	Relation * newRR = schema_mgr.createRelation(table_name+"SORT"+to_string(_g_relation_counter++), newR->getSchema());
	
	// @param1:relation ptr, @param2: mem, @param3: head of the sublists, 
	// @param4: the vector of multiple/single target sort column
	HeapManager heapMgr(newR, mem, subHeads, vector<int>(1, indField));

	while(!heapMgr.empty()){
		vector<Tuple> tuples = heapMgr.popTopMins(mem);
		for(int j = 0; j < tuples.size(); ++j){
		  appendTupleToRelation(newRR, mem, tuples[j]);
		}
	}
	resetFreeBlocks();

	// very ugly!
	newR = newRR;
	return;
}


map<string, int> Algorithm::findJoinField(){
	stack<string> stk;
	map<string, int> fields;
	for (int i = 0; i < m_conditions.size(); i++){
		vector<string> column = splitBy(m_conditions[i], ".");
		// column name
		if (column.size() == 2){
			stk.push(column[1]);
		}
		// operator: only "=" "AND" is allowed
		else{
			if (m_conditions[i] == "="){
				string op1 = stk.top();
				stk.pop();
				string op2 = stk.top();
				stk.pop();
				if (op1 != op2){
					fields.clear();
					return fields;
				}
				else 
					fields[op1] = -1;
			}
			else if (m_conditions[i] != "AND"){
				fields.clear();
				return fields;
			}
		}
	}
}

set<string> Algorithm::findDupField(vector<Relation*> relations){
	set<string> dups;
	set<string> names;

	for (int r = 0; r < relations.size(); r++){
		Schema sch = relations[r]->getSchema();
		for (int i = 0; i < sch.getNumOfFields(); i++){
			string field_name = sch.getFieldName(i);
			if (names.count(field_name) != 0){
				dups.insert(field_name);
			}
			else
				names.insert(field_name);
		}
	}
	return dups;
}

// get the new schema for Binary Op
Schema Algorithm::getJoinSchema(Relation *left, Relation *right, bool left_is_leaf, bool right_is_leaf, vector<vector<int> > &mapping, bool &is_natural){
	vector<pair<Relation*, bool> > relations;
	relations.push_back(make_pair(left, left_is_leaf));
	relations.push_back(make_pair(right, right_is_leaf));

	map<string, int> join_field = findJoinField();
	is_natural = !join_field.empty();
	//set<string> dup_field = findDupField(relations);

	int size = left->getSchema().getNumOfFields() + right->getSchema().getNumOfFields() - join_field.size();

	vector<string> new_names(size), names;
	vector<enum FIELD_TYPE> new_types(size), types;
	//vector<vector<int> > mapping(relations.size());

	int idx = 0;
	for (int r = 0; r < relations.size(); r++){
		Relation *rel = relations[r].first;
		bool is_leaf = relations[r].second;

		names = rel->getSchema().getFieldNames();
		types = rel->getSchema().getFieldTypes();

		if (is_leaf){
			for (int i = 0; i < names.size(); i++){
				names[i] = rel->getRelationName() + "." + names[i];
			}
		}

		mapping[r].resize(names.size());
		for (int i = 0; i < names.size(); i++){
			string field = names[i];
			if (splitBy(field, ".").size() == 2)
				field = splitBy(field, ".")[1];

			if (join_field.count(field) != 0){
				// first time seeing this field
				if (join_field[field] == -1){
					new_names[idx] = field;
					new_types[idx] = types[i];
					// save current postition
					join_field[field] = idx;

					//dup_field.erase(names[i]);
					//
					// mapping i -> idx: names[i] is now new_names[idx]
					mapping[r][i] = idx++;
				}
				// has been seen
				else{
					mapping[r][i] = join_field[field];

				}
			}
			/*
			   else if (dup_field.count(names[i]) != 0){
			   new_names.push_back(rel->getRelationName() + "." + names[i]);
			   new_types.push_back(types[i]);
			   }
			 */
			else{
				new_names[idx] = names[i];
				new_types[idx] = types[i];
				// mapping i -> idx: names[i] is now new_names[idx]
				mapping[r][i] = idx++;
			}
		}
	}
	assert(size == idx);


	if(new_names.empty()){
		cerr<<"Warning: no available projection attributes!"<<endl;
	}
	Schema new_schema = Schema(new_names, new_types);
	return new_schema;

}

Relation * Algorithm::runBinary(Relation * left, Relation * right, MainMemory & mem, SchemaManager & schema_mgr, bool left_is_leaf, bool right_is_leaf){
	int num_blocks = free_blocks.size();
	// get a available mem block
	assert(num_blocks >= 2);
	int left_size =  left->getNumOfBlocks();
	int right_size =  right->getNumOfBlocks();
	if (left_size > right_size) 
		return runBinary(right, left, mem, schema_mgr, right_is_leaf, left_is_leaf);
	assert(left_size <= right_size);

	vector<vector<int> > idx_map(2);
	bool is_natural = false;
	Schema join_schema = getJoinSchema(left, right, left_is_leaf, right_is_leaf, idx_map, is_natural);
	/*
	cout<<join_schema<<endl;
	print(idx_map[0]);
	print(idx_map[1]);
	*/

	string new_relation_name = left->getRelationName() + "_" + left->getRelationName() + to_string(_g_relation_counter++);
	Relation * join_relation = schema_mgr.createRelation(new_relation_name, join_schema);

	int cost1pass = ( left_size / num_blocks ) * right_size + left_size;
	int cost2pass = 3 * (left_size + right_size); 
	if (is_natural && cost2pass < cost1pass){
		//cout<<"Using 2 pass join!"<<endl;
		join2Pass(left, right, idx_map[0], idx_map[1], join_relation, mem, schema_mgr);
	}

	else{
		//cout<<"Using 1 pass join!"<<endl;
		join1Pass(left, right, idx_map[0], idx_map[1], join_relation, mem);
	}
	return join_relation;
}

vector<int> Algorithm::subSortForJoin(Relation* oldR, Relation* &newR, MainMemory &mem, SchemaManager &schema_mgr, vector<int> indices){

	string new_relation_name = oldR->getRelationName() + to_string(_g_relation_counter++);
	newR  = schema_mgr.createRelation(new_relation_name, getNewSchema(oldR, false));

	int dBlocks= oldR->getNumOfBlocks();
	int sublists = 0;
	vector<int> subHeads;
	// first pass, make sorted sublists:
	int cnt = 0, start = 0;
	for(int start = 0; start < dBlocks; start += cnt){
		subHeads.push_back(start);
		// sort the subchunk first and write back
		cnt = sortByChunkWB(oldR, newR, mem, start, indices);
		sublists++;
	}
	//cout<<"relation has "<<sublists<<" sublists to merge. "<<endl;
	//cout<<*newR<<endl;
	return subHeads;
}

void Algorithm::join2Pass(Relation *old_left, Relation *old_right, vector<int> left_map, vector<int> right_map, Relation *join, MainMemory& mem, SchemaManager &schema_mgr){
	int dBlocks= old_left->getNumOfBlocks() + old_right-> getNumOfBlocks();
	const int mSize = mem.getMemorySize();
	if(sqrt(dBlocks) > double(mSize)){
		cerr<<"Fatal Error: The memory condition: M >= sqrt(B(R)) is not satisfied!! Cannot use the two-passed algorithms!"<<endl;
		return;
	}

	//find joint field index in left and right relation
	vector<int> join_field_left, join_field_right;
	for (int i = 0; i < left_map.size(); i++){
		for (int j = 0; j < right_map.size(); j++){
			if (left_map[i] == right_map[j]){
				join_field_left.push_back(i);
				join_field_right.push_back(j);
			}
		}
	}

	Relation* new_left, *new_right;
	// first pass
	vector<int> leftHeads = subSortForJoin(old_left, new_left, mem, schema_mgr, join_field_left);
	vector<int> rightHeads = subSortForJoin(old_right, new_right, mem, schema_mgr, join_field_right);


	// second pass, merge the sorted sublists, note that the to-be-merged one is in the newR  
	// @param1:relation ptr, @param2: mem, @param3: head of the sublists, 
	// @param4: the vector of multiple/single target sort column
	HeapManager leftHMgr(new_left, mem, leftHeads, join_field_left);
	HeapManager rightHMgr(new_right, mem, rightHeads, join_field_right);

	while(!leftHMgr.empty() || !rightHMgr.empty()){
		vector<Tuple> left_tuples = leftHMgr.popTopMins(mem);
		vector<Tuple> right_tuples = rightHMgr.popTopMins(mem);
		for(int i = 0; i < left_tuples.size(); ++i){
			Tuple left_tuple = left_tuples[i];
			for (int j = 0; j < right_tuples.size(); ++j){
				Tuple right_tuple = right_tuples[i];
				// here we have left_tuple and right_tuple that can be natrual join
				Tuple join_tuple = join->createTuple();
				// write into join_tuple
				for (int li = 0; li < left_map.size(); li++){
					if (join_tuple.getSchema().getFieldType(left_map[li]) == INT)	
						join_tuple.setField(left_map[li], left_tuple.getField(li).integer);
					else 
						join_tuple.setField(left_map[li], *(left_tuple.getField(li).str));
				}
				for (int ri = 0; ri < right_map.size(); ri++){
					if (join_tuple.getSchema().getFieldType(right_map[ri]) == INT)	
						join_tuple.setField(right_map[ri], right_tuple.getField(ri).integer);
					else 
						join_tuple.setField(right_map[ri], *(right_tuple.getField(ri).str));
				}
				appendTupleToRelation(join, mem, join_tuple);
			}
		}
	}
}

void Algorithm::join1Pass(Relation *left, Relation *right, vector<int> left_map, vector<int> right_map, Relation *join, MainMemory& mem){
	int num_blocks = free_blocks.size();
	int left_size =  left->getNumOfBlocks();
	int p_left_size = num_blocks - 2;
	int right_size =  right->getNumOfBlocks();
	assert(left_size <= right_size);

	// chekcout free mem blocks
	vector<int> p_left;
	for (int i = 0; i < p_left_size; i++){
		p_left.push_back(free_blocks.front());
		free_blocks.pop();
	}
	int p_right = free_blocks.front();
	free_blocks.pop();

	Eval evaluate = Eval(m_conditions);
	int left_idx = 0;
	while (left_idx < left_size){
		int i = 0;
		for (; i < p_left_size && left_idx < left_size; i++, left_idx++){
			left->getBlock(left_idx, p_left[i]);
		}
		int p_left_in_use = i;

		for (int j = 0; j < right_size; j++){
			right->getBlock(j, p_right);
			Block *right_ptr = mem.getBlock(p_right);
			vector<Tuple> right_tuples = right_ptr->getTuples();
			for(int r = 0; r < right_tuples.size(); ++r){
				Tuple right_tuple = right_tuples[r];

				for (int i = 0; i < p_left_in_use; i++){
					Block *left_ptr = mem.getBlock(p_left[i]);
					vector<Tuple> left_tuples = left_ptr->getTuples();
					for(int l = 0; l < left_tuples.size(); ++l){
						Tuple left_tuple = left_tuples[l];
						Tuple join_tuple = join->createTuple();
						vector<bool> is_written(join_tuple.getNumOfFields(), false);
						bool valid_tuple = true;
						for (int li = 0; li < left_map.size(); li++){
							is_written[left_map[li]] = true;
							if (join_tuple.getSchema().getFieldType(left_map[li]) == INT)	
								join_tuple.setField(left_map[li], left_tuple.getField(li).integer);
							else 
								join_tuple.setField(left_map[li], *(left_tuple.getField(li).str));
						}
						for (int ri = 0; ri < right_map.size(); ri++){
							// fount collision: check if the value written by right is the same as left
							// if yes, it is valid natural join tuple
							// if no, it is invalid natural join tuple
							if (is_written[right_map[ri]] == true){
								if (join_tuple.getSchema().getFieldType(right_map[ri]) == INT){
									if (join_tuple.getField(right_map[ri]).integer != right_tuple.getField(ri).integer){
										valid_tuple = false;
										break;
									}
								}	
								else{
									if (*(join_tuple.getField(right_map[ri]).str) != *(right_tuple.getField(ri).str)){
										valid_tuple = false;
										break;
									} 
								}
							}
							if (join_tuple.getSchema().getFieldType(right_map[ri]) == INT)	

								join_tuple.setField(right_map[ri], right_tuple.getField(ri).integer);
							else 
								join_tuple.setField(right_map[ri], *(right_tuple.getField(ri).str));
						}
						if (valid_tuple == true && evaluate.evalUnary(join_tuple) == true) 
							appendTupleToRelation(join, mem, join_tuple);
					}
				}
			}
		}
	}

	for (int i = 0; i < p_left_size; i++){
		free_blocks.push(p_left[i]);
	}
	free_blocks.push(p_right);
}
