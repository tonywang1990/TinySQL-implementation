
extern queue<int> free_blocks;

void generatePQP(Node * head, SchemaManager& schema_manager, MainMemory& mem);

void reverse_postorder_traverse(Node * N, SchemaManager& schema_manager, MainMemory& mem);
void loadRelationPtr(Node * N, SchemaManager& schema_manager);

Relation * unaryReadWrite(Relation * child, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType, int level);
Relation * binaryReadWrite(Relation * left, Relation * right, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType, int level);
