#include "utility.h"
#include "algorithm.h"
#include<stack>
#include<set>

using namespace std;

Algorithm::Algorithm(bool isOnePass, const vector<string>& conditions, TYPE type, int level):
	m_isOnePass(isOnePass), 
	m_conditions(conditions), 
	m_type(type),
	m_level(level)
{
}

Relation * Algorithm::RunUnary(Relation * relation_ptr, MainMemory & mem, SchemaManager & schema_mgr, bool is_leaf)
{
	assert(relation_ptr);
	if(T[m_type] == "PROJECT" && m_conditions.size() == 1 && m_conditions[0] == "*"){
		return relation_ptr;
	}

	// create a new table for the output:
	string new_relation_name = relation_ptr->getRelationName() + T[m_type] + to_string(m_level);
	Relation * newRelation = schema_mgr.createRelation(new_relation_name, getNewSchema(relation_ptr, is_leaf));

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
		for(int i = 0; i < tuples.size(); ++i){
			Tuple t = tuples[i];
			if (evaluate.evalUnary(t) == false) continue;
			Tuple tmp = newR->createTuple();
			for(int j = 0; j < tmp.getSchema().getNumOfFields(); ++j){
				if(tmp.getSchema().getFieldType(j) == 0)
					tmp.setField(j, t.getField(j).integer);
				else
					tmp.setField(j, *(t.getField(j).str));
			}

			appendTupleToRelation(newR, mem, tmp);
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

map<string, bool> Algorithm::findJoinField(){
	stack<string> stk;
	map<string, bool> fields;
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
					fields[op1] = true;
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
Schema Algorithm::getJoinSchema(Relation *left, Relation *right, bool left_is_leaf, bool right_is_leaf, vector<vector<int> > &mapping, vector<string> &join_fields){
	vector<pair<Relation*, bool> > relations;
	relations.push_back(make_pair(left, left_is_leaf));
	relations.push_back(make_pair(right, right_is_leaf));

	map<string, bool> join_field = findJoinField();
	for (map<string,bool>::iterator it = join_field.begin(); it != join_field.end(); ++it){
		join_fields.push_back(it->first);
	}
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
				if (join_field[field] == true){
					new_names[idx] = field;
					new_types[idx] = types[i];
					// remove 
					join_field[field] = false;
					//dup_field.erase(names[i]);
					
					// mapping i -> idx: names[i] is now new_names[idx]
					mapping[r][i] = idx++;
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

Relation * Algorithm::RunBinary(Relation * left, Relation * right, MainMemory & mem, SchemaManager & schema_mgr, bool left_is_leaf, bool right_is_leaf){
	int num_blocks = free_blocks.size();
	// get a available mem block
	assert(num_blocks >= 2);
	int left_size =  left->getNumOfBlocks();
	int right_size =  right->getNumOfBlocks();
	if (left_size > right_size) 
		return RunBinary(right, left, mem, schema_mgr, right_is_leaf, left_is_leaf);
	assert(left_size <= right_size);

	vector<vector<int> > idx_map(2);
	vector<string> joint_fields;
	Schema join_schema = getJoinSchema(left, right, left_is_leaf, right_is_leaf, idx_map, joint_fields);
	cout<<join_schema<<endl;
	print(idx_map[0]);
	print(idx_map[1]);

	string new_relation_name = left->getRelationName() + "_" + left->getRelationName();
	Relation * join_relation = schema_mgr.createRelation(new_relation_name, join_schema);

	if (left_size <= num_blocks-2){ // one for right, one for output
		join1Pass(left, right, idx_map[0], idx_map[1], join_relation, mem);
	}
	else{
		cout<<"use 2Pass algorithm!"<<endl;
		abort();
	}


	return join_relation;
}

void Algorithm::join1Pass(Relation *left, Relation *right, vector<int> left_map, vector<int> right_map, Relation *join, MainMemory& mem){
	int num_blocks = free_blocks.size();
	int left_size =  left->getNumOfBlocks();
	int right_size =  right->getNumOfBlocks();
	assert(left_size <= right_size && left_size < num_blocks);
	
	Eval evaluate = Eval(m_conditions);

	int p_left = free_blocks.front();
	for (int i = 0; i < left_size; i++){
		left->getBlock(i, p_left + i);
		free_blocks.pop();
	}

	int p_right = free_blocks.front();
	free_blocks.pop();
	for (int j = 0; j < right_size; j++){
		right->getBlock(j, p_right);
		Block *right_ptr = mem.getBlock(p_right);
		vector<Tuple> right_tuples = right_ptr->getTuples();
		for(int r = 0; r < right_tuples.size(); ++r){
			Tuple right_tuple = right_tuples[r];

			for (int i = p_left; i < p_left + left_size; i++){
				Block *left_ptr = mem.getBlock(i);
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
						// collision: check is valid natural join tuple
						if (is_written[right_map[ri]] == true){
							if (join_tuple.getSchema().getFieldType(right_map[ri]) == INT){
								if (join_tuple.getField(right_map[ri]).integer != right_tuple.getField(ri).integer){
									valid_tuple = false;
									break;
								}
							}	
							else{
								if (join_tuple.getField(right_map[ri]).str!= right_tuple.getField(ri).str){
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

	for (int i = 0; i < left_size; i++){
		free_blocks.push(p_left + i);
	}
	free_blocks.push(p_right);
}
