#include "utility.h"
#include "lqp.h"
#include "pqp.h"
#include<algorithm>
#include<stack>

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

	postfixLQP(head);

	printLQP(head);
	
	generatePQP(head, schema_manager, mem);
}

void postfixLQP(Node *N){
	N->param = postFixfy(N->param);
	for (int i = 0; i < N->children.size(); i++){
		postfixLQP(N->children[i]);
	}
}

vector<string> postFixfy(vector<string> infix){
	// seperate ()
	for (int i = 0; i < infix.size(); i++){
		// skip single char
		if (infix[i].size() == 1) continue;
		// left (
		if (infix[i][0] == '('){
			infix[i] = infix[i].substr(1);
			infix.insert(infix.begin() + i, "(");
		}
		// right )
		if (infix[i][infix[i].size()-1] == ')'){
			infix[i] = infix[i].substr(0, infix[i].size()-1);
			infix.insert(infix.begin() + i + 1, ")");
		}
	}
	stack<string> stk;
	vector<string> postfix;
	for (int i = 0; i < infix.size(); i++){
		// is operands
		if (opPreced(infix[i]) == 0){
			postfix.push_back(infix[i]);
		}
		// is operator
		else{
			if (stk.empty() || infix[i] == "("){
				stk.push(infix[i]);
			}
			else if (infix[i] == ")"){
				while (stk.top() != "("){
					postfix.push_back(stk.top());
					stk.pop();
				}
				stk.pop();// pop (
			}
			else if (opPreced(stk.top()) < opPreced(infix[i])){
				stk.push(infix[i]);
			}
			else if (opPreced(stk.top()) >= opPreced(infix[i])){
				while (!stk.empty() && opPreced(stk.top()) >= opPreced(infix[i])){
					postfix.push_back(stk.top());
					stk.pop();
				}
				stk.push(infix[i]);
			}
		}
	}
	while (!stk.empty()){
		postfix.push_back(stk.top());
		stk.pop();
	}
	print(postfix);
	return postfix;
}

// return the precedence of the operator/operation
// rule: 
// 1. incoming operand with higher precedence than top of stack => push incoming operand
// 2. incoming operand with lower/equal precedence than top of stack => pop top and push incoming operand
int opPreced(string x){
	if (x == "(" || x == ")")
		return -1;
	else if (x == "<" || x == ">" || x == "=" || x == "+" || x == "-" || x == "*")
		return 3;
	else if (x == "AND")
		return 2;
	else if (x == "OR")
		return 1;
	else 
		return 0;
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
			string dest = "LEAF";
			vector<string> clause;

			for (int i = 0; i < params.size(); i++){
				if (params[i] != "AND"){
					// collect one clause
					clause.push_back(params[i]);

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
				// end of one clause, push to dest
				else{
					if (select_opt.count(dest) != 0) select_opt[dest].push_back("AND");
					select_opt[dest].insert(select_opt[dest].end(), clause.begin(), clause.end());
					clause.clear();
					dest = "LEAF";
				}
			}	
			// push the last one to dest
			if (select_opt.count(dest) != 0) select_opt[dest].push_back("AND");
			select_opt[dest].insert(select_opt[dest].end(), clause.begin(), clause.end());
		}
		// has OR, push all to product
		else{
			vector<string> clause;
			for (int i = 0; i < params.size(); i++){
				clause.push_back(params[i]);
			}	
			select_opt["PRODUCT"].insert(select_opt["PRODUCT"].end(), clause.begin(), clause.end());
		}
		// clear this select node
		N->param.clear();

		/*
		// output current select_opt array
		for (map<string, vector<string> >::iterator iit = select_opt.begin(); iit != select_opt.end(); ++iit){
		cout<<iit->first<<": ";
		for (int i = 0; i < iit->second.size(); i++){
		cout<<iit->second[i]<<" ";
		}
		cout<<endl;
		}
		*/
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
				cout<<level[i]->param[j];
				if (j != level[i]->param.size()-1) cout<<" | ";
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



