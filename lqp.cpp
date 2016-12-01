#include "utility.h"
#include "lqp.h"

enum TYPE {SELECT, PROJECT, PRODUCT, JOIN, THETA, DISTINCT, SORT, LEAF};

map<TYPE, string> T;

class Node{
public:
	TYPE type;
	vector<string> param;
	Relation* view;
	vector<Node*> children;
	Node(TYPE t, vector<string> p){
		type = t;
		param = p;
		view = NULL;
	}
	Node(TYPE t){
		type = t;
		view = NULL;
	}
};

void generateLQP(bool has_distinct, vector<string> select_list, vector<string> from_list, vector<string> where_list, vector<string> order_list, SchemaManager &schema_manager, MainMemory &mem){

	initMapT();

	bool has_where = !where_list.empty();
	bool has_order = !order_list.empty();
	//construct lqp
	// project 
	Node *head = new Node(PROJECT, select_list);
	Node *N = head;
	Node *child;
	// sort 
	if (has_order){
		child = new Node(SORT, order_list);
		N->children.push_back(child);
		N = child;
	}
	// eliminate dup
	if (has_distinct){
		child = new Node(DISTINCT);
		N->children.push_back(child);
		N = child;
	}
	// select
	child = new Node(SELECT, where_list);
	N->children.push_back(child);
	N = child;
	// product
	int Size = from_list.size();
	int idx = 0;
	while (Size > 1){
		child = new Node(PRODUCT);
		N->children.push_back(child);
		//Node *product = N;
		N = child;
		child = new Node(LEAF, vector<string> (1, from_list[idx]));
		N->children.push_back(child);
		Size--; idx++;
	}
	// last child
	child = new Node(LEAF, vector<string> (1, from_list[idx]));
	N->children.push_back(child);

	printLQP(head);

	optimizeLQP(head);

}

// apply optimization logic plan
void optimizeLQP(Node *head){

}

void printLQP(Node* head){
	cout<<"*****Start of LQP tree ******"<<endl;
	queue<Node*> Q;
	Q.push(head);
	while (!Q.empty()){
		vector<Node*> level;
		while (!Q.empty()){
			level.push_back(Q.front());
			Q.pop();
		}
		for (int i = 0; i < level.size(); i++){
			cout<<T[level[i]->type]<<level[i]->children.size()<<" (";
			for (int j = 0; j < level[i]->param.size(); j++){
				cout<<level[i]->param[j]<<" ";
			}
			cout<<") ";
			for (int k = 0; k < level[i]->children.size(); k++){
				Q.push(level[i]->children[k]);
			}
		}
		cout<<endl;
	}
	cout<<"*****End of LQP tree ******"<<endl;
}

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
}

