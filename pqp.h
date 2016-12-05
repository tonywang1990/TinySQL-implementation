
extern queue<int> free_blocks;

void generateDPQP(Node * head, SchemaManager& schema_manager, MainMemory& mem);
void generatePQP(Node * head, SchemaManager& schema_manager, MainMemory& mem);

void reverse_postorder_traverse(Node * N, SchemaManager& schema_manager, MainMemory& mem);
void loadRelationPtr(Node * N, SchemaManager& schema_manager);

Relation * unaryReadWrite(Node* N, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType, int level);
Relation * binaryReadWrite(Node* left_node, Node* right_node, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType, int level);
