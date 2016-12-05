#include "utility.h"
#include "query.h"
#include "lqp.h"

#include<algorithm>
#include<cstdio>

extern queue<int> free_blocks;

void Insert(vector<string> &words, string &line, SchemaManager &schema_manager, MainMemory &mem){
	Relation* relation_ptr = schema_manager.getRelation(words[2]);

	vector<string>::iterator it = find(words.begin(), words.end(), "SELECT");
	// no select
	if (it == words.end()){
		// get insert vals
		vector<string> content = splitBy(line, "()");
		vector<string> fields = splitBy(content[1], ", ");
		vector<string> vals = splitBy(content[3], ", ");
		//preProcess(vector<string>(1, words[2]), fields, schema_manager);
		preProcess(vector<string>(1, words[2]), vals, schema_manager);

		assert(fields.size() == vals.size());

		Tuple tuple = relation_ptr->createTuple();

		for (int i = 0; i < vals.size(); i++){
			if (tuple.getSchema().getFieldType(fields[i]) == INT){
				tuple.setField(fields[i], atoi(vals[i].c_str()));
			}
			else{
				tuple.setField(fields[i], vals[i]);
			}
		}
		appendTupleToRelation(relation_ptr, mem, tuple);
	}
	// with SELECT
	else{
		vector<string> SFW(it, words.end());	
		Relation* new_relation = Select(SFW, schema_manager, mem);
		assert(new_relation);

		vector<string> new_field_names = nakedFieldNames(new_relation);
		vector<string> field_names = nakedFieldNames(relation_ptr);

		// mapping: index of new_field_names to field_names 
		vector<int> mapping(new_field_names.size(), -1);
		for (int i = 0; i < new_field_names.size(); i++){
			for (int j = 0; j < field_names.size(); j++){
				if (new_field_names[i] == field_names[j]){
					mapping[i] = j;
					break;
				}
			}
		}

		int new_field_size = new_relation->getSchema().getNumOfFields();

		// warning: new_relation and relation_ptr might be the same!
		// get all tuples from the new_relation in one run
		vector<Tuple> new_tuples;
		for (int i = 0; i < new_relation->getNumOfBlocks(); i++){

			assert(!free_blocks.empty());
			int memory_block_index = free_blocks.front();
			free_blocks.pop();

			// read the relation block by block
			new_relation->getBlock(i, memory_block_index);
			Block* block_ptr = mem.getBlock(memory_block_index);
			assert(block_ptr);
			vector<Tuple> block_tuples = block_ptr->getTuples();
			new_tuples.insert(new_tuples.end(), block_tuples.begin(), block_tuples.end());
			if(new_tuples.empty()){
				cerr<<"Warning: Insert from SFW, No tuples in the current mem block!"<<endl;
			}
			free_blocks.push(memory_block_index);
		}

		for (int j = 0; j < new_tuples.size(); j++){
			Tuple tuple = relation_ptr->createTuple();
			for (int k = 0; k < new_field_size; k++){
				if (mapping[k] != -1){
					int idx = mapping[k];
					assert(idx < relation_ptr->getSchema().getNumOfFields() && idx >= 0);
					if (tuple.getSchema().getFieldType(idx) == INT){
						int val = new_tuples[j].getField(k).integer;
						tuple.setField(field_names[idx], val);
					}
					else{
						string *str = new_tuples[j].getField(k).str;
						tuple.setField(field_names[idx], *str);
					}
				}
			}
			appendTupleToRelation(relation_ptr, mem, tuple);
		}
		cout<<*relation_ptr<<endl;
	}
}

void Delete(vector<string> &words, SchemaManager &schema_manager, MainMemory &mem){

	Relation* relation_ptr = schema_manager.getRelation(words[2]);
	vector<string>::iterator it = find(words.begin(), words.end(), "SELECT");
	// no WHERE, delete everything
	if (it == words.end()){
		relation_ptr->deleteBlocks(0);	
	}
	// with WHERE clause
	else{
		vector<string> where_list(it, words.end());
		preProcess(vector<string> (1, words[2]), where_list, schema_manager);

	}

}

Relation* Select(vector<string> &words, SchemaManager &schema_manager, MainMemory &mem){
	vector<string> select_list, from_list, where_list, order_list;
	bool has_distinct = false, has_where = false, has_orderby = false;
	int i = 1;
	if (words[i] == "DISTINCT"){
		has_distinct = true;
		i++;
	}
	while (i < words.size() && words[i] != "FROM"){
		// drop comma
		select_list.push_back(splitBy(words[i], ",")[0]);
		i++;
	}
	i++; // skip FROM
	while ( i < words.size() && words[i] != "WHERE" && words[i] != "ORDER"){
		from_list.push_back(splitBy(words[i], ",")[0]);
		i++;
	}
	if (i < words.size()){
		if (words[i] == "WHERE"){
			has_where = true;
			i++; // skip WHERE
			while (i < words.size() && words[i] != "ORDER"){
				where_list.push_back(words[i]);
				i++;
			}
		}
		if (i < words.size() && words[i] == "ORDER"){
			has_orderby = true;
			i = i + 2; // skip ORDER BY
			order_list.push_back(words[i]);
			i++;
		}
	}

	// add table name to each column name
	preProcess(from_list, select_list, schema_manager);
	preProcess(from_list, where_list, schema_manager);
	preProcess(from_list, order_list, schema_manager);
	/*
	   print(select_list);
	   print(from_list);
	   print(where_list);
	   print(order_list);
	   */
	return generateLQP(has_distinct, select_list, from_list, where_list, order_list, schema_manager, mem);

}

void preProcess(const vector<string> &tables, vector<string> &words, SchemaManager &schema_manager){
	for (int i = 0; i < words.size(); i++){
		bool is_column = false;
		// has no "."
		if (words[i].find('.') == string::npos){
			for (int j = 0; j < tables.size(); j++){
				if (schema_manager.getSchema(tables[j]).fieldNameExists(words[i])){
					words[i] = tables[j] + "." + words[i];
					is_column = true;
					break;
				}
				// term or value
				if (!is_column){
					string legal_word;
					for (int k = 0; k < words[i].size(); k++){
						if (words[i][k] != '"'){
							legal_word.push_back(words[i][k]);
						}
					}
					words[i] = legal_word;
				}
			}
		}
	}
}

