#include "utility.h"
#include "query.h"
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
		
	}
}

void Delete(vector<string> &words, string &line, SchemaManager &schema_manager, MainMemory &mem){
	
	Relation* relation_ptr = schema_manager.getRelation(words[2]);
	vector<string>::iterator it = find(words.begin(), words.end(), "SELECT");
	// no WHERE, delete everything
	if (it == words.end()){
		relation_ptr->deleteBlocks(0);	
	}
	// with WHERE clause
	else{

	}

}

void Select(vector<string> &words, string &line, SchemaManager &schema_manager, MainMemory &mem){
	vector<string> select_list, from_list, where_list;
	string order_list;
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
			while (words[i] != "ORDER" && i < words.size()){
				where_list.push_back(words[i]);
				i++;
			}
		}
		if (words[i] == "ORDER"){
			has_orderby = true;
			i = i + 2; // skip ORDER BY
			order_list = words[i];
			i++;
		}
	}

	print(select_list);
	print(from_list);
	print(where_list);
	cout<<order_list<<endl;



}

