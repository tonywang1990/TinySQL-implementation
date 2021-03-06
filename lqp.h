class Node;
Relation* generateDLQP(vector<string> where_list, string relation_name, SchemaManager & schema_manager, MainMemory & mem);
Relation* generateLQP(bool has_distinct, vector<string> select_list, vector<string> from_list, vector<string> where_list, vector<string> order_list, SchemaManager &schema_manager, MainMemory &mem);
void printLQP(Node* head);
void optimizeLQP(Node *head);
void preorder_traverse(Node *N, map<string, vector<string> > &select_opt);
void postfixLQP(Node *N);
vector<string> postFixfy(vector<string> infix);
int opPreced(string x);
