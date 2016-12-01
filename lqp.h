enum TYPE {SELECT, PROJECT, PRODUCT, JOIN, THETA, DISTINCT, SORT, LEAF};


class Node;

void generateLQP(bool has_distinct, vector<string> select_list, vector<string> from_list, vector<string> where_list, vector<string> order_list, SchemaManager &schema_manager, MainMemory &mem);
void printLQP(Node* head);
void initMapT();
void optimizeLQP(Node *head);
void preorder_traverse(Node *N, map<string, vector<string> > &select_opt);
