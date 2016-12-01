#include "utility.h"
#include "lqp.h"
#include<algorithm>

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
	printLQP(head);

}

// apply optimization to logic plan
void optimizeLQP(Node *head){
	// optimization array
	// <destination, params>
	map<string, vector<string> > select_opt;
	preorder_traverse(head, select_opt);
}

void preorder_traverse(Node *N, map<string, vector<string> > &select_opt){
	if (N->type == SELECT){
		vector<string> params = N->param;
		vector<string>::iterator it = find(params.begin(), params.end(), "OR");
		// no OR statement, safe to push down selection
		if (it == params.end()){
			string clause, dest = "LEAF";

			for (int i = 0; i < params.size(); i++){
				if (params[i] != "AND"){
					// collect one clause
					clause += params[i] + " ";

					// find destination of this clause
					vector<string> decomp = splitBy(params[i], ".()");
					// relation name is given
					if (decomp.size() == 2){
						if (dest == "LEAF"){
							// remember this relation name
							dest = decomp[0];
						}
						else{
							// if different, can only pushed down to product
							if (decomp[0] != dest){
								dest = "PRODUCT";
							}
						}
					}
				}
				else{
					select_opt[dest].push_back(clause);
					clause.clear();
					dest = "LEAF";
				}
			}	
			select_opt[dest].push_back(clause);
		}
		// has OR, push all to product
		else{
			string clause;
			for (int i = 0; i < params.size(); i++){
				clause += params[i] + " ";
			}	
			select_opt["PRODUCT"].push_back(clause);
		}
		// clear this select node
		N->param.clear();

		// output current select_opt array
		for (map<string, vector<string> >::iterator iit = select_opt.begin(); iit != select_opt.end(); ++iit){
			cout<<iit->first<<": ";
			for (int i = 0; i < iit->second.size(); i++){
				cout<<iit->second[i]<<" ";
			}
			cout<<endl;
		}
	}
	else if (N->type == PRODUCT){
		if (select_opt.find("PRODUCT") != select_opt.end()){
			for (int i = 0; i < select_opt["PRODUCT"].size(); i++){
				N->param.push_back(select_opt["PRODUCT"][i]);
			}

		}
	}
	else if (N->type == LEAF){
		string relation_name = N->param[0];
		if (select_opt.find(relation_name) != select_opt.end()){
			// make a copy of N as L, set L as N's child
			Node *L = new Node(N->type, N->param);
			N->children.push_back(L);
			// change N into a select node
			N->type = SELECT;
			N->param = select_opt[relation_name];
			N = L;
		}

	}

	for (int i = 0; i < N->children.size(); i++){
		preorder_traverse(N->children[i], select_opt);
	}
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

