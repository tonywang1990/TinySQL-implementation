/* Algorithm class for physical query plan*/
#ifndef ALGORITHM
#define ALGORITHM

#include "utility.h"

using namespace std;

// wrapper for algorithm:
class Algorithm{
	private:
		bool m_isOnePass;
		set<Tuple, myCompare> m_set;
		vector<string> m_conditions;
		TYPE m_type;
		int m_level;

	public:
		Algorithm(bool isOnePass, const vector<string>& condition, TYPE type, int level);
		Relation * RunUnary(Relation * relation_ptr, MainMemory & mem, SchemaManager & schema_mgr, bool is_leaf);

		// get new schema for projection
		Schema getNewSchema(Relation * relation_ptr, bool is_leaf);

		void Select(Relation * oldR, Relation * newR, MainMemory& mem);
		void Project(Relation * oldR, Relation * newR, MainMemory& mem, vector<int> indices);

		void Distinct(Relation * oldPtr, Relation * newPtr, MainMemory & mem);

		void Sort(Relation * oldPtr, Relation * newPtr, MainMemory & mem);

		Relation * RunBinary(Relation * left, Relation * right, MainMemory & mem, SchemaManager & schema_mgr, bool left_is_leaf, bool right_is_leaf);

		map<string, int> findJoinField();
		set<string> findDupField(vector<Relation*> relations);
		Schema getJoinSchema(Relation *left, Relation *right, bool left_is_leaf, bool right_is_leaf, vector<vector<int> > &mapping);
		void join1Pass(Relation *left, Relation *right, vector<int> left_map, vector<int> right_map, Relation *join, MainMemory& mem);
};
#endif
