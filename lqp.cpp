#include "utility.h"
#include "lqp.h"
#include "pqp.h"
#include<algorithm>
#include<stack>

// do this only for delete!
Relation* generateDLQP(vector<string> where_list, string relation_name, SchemaManager & schema_manager, MainMemory & mem){
  Node * head = new Node(DELETE, where_list, 0);
  Node * node = new Node(LEAF, vector<string>(1, relation_name), 1);
  assert(head && node);
  head->children.push_back(node);

  // change conditions to post fix expression
  postfixLQP(head);
  
  generateDPQP(head, schema_manager, mem);
  
  assert(head->view);
  return head->view;

}

Relation* generateLQP(bool has_distinct, vector<string> select_list, vector<string> from_list, vector<string> where_list, vector<string> order_list, SchemaManager &schema_manager, MainMemory &mem){

	initMapT();
	
	int level = 0;
	bool has_where = !where_list.empty();
	bool has_order = !order_list.empty();
	//construct lqp
	Node *dummy = new Node(HEAD, vector<string> (), -1);
	Node *N = dummy, *child;
	// eliminate dup
	if (has_distinct){
		child = new Node(DISTINCT);
		child->level = level++;
		N->children.push_back(child);
		N = child;
	}
	// project 

	child = new Node(PROJECT, select_list, level++);
	child->level = level++;
	N->children.push_back(child);
	N = child;

	// sort 
	if (has_order){
		child = new Node(SORT, order_list, level++);
		N->children.push_back(child);
		N = child;
	}

	// select
	child = new Node(SELECT, where_list, level++);
	N->children.push_back(child);
	N = child;
	// product
	int Size = from_list.size();
	int idx = 0;
	while (Size > 1){
		child = new Node(PRODUCT);
		child->level = level++;
		N->children.push_back(child);
		//Node *product = N;
		N = child;
		child = new Node(LEAF, vector<string> (1, from_list[idx]), level++);
		N->children.push_back(child);
		Size--; idx++;
	}
	// last child
	child = new Node(LEAF, vector<string> (1, from_list[idx]), level++);
	N->children.push_back(child);

	assert(dummy && dummy->children.size() == 1);
	Node* head = dummy->children[0];
	assert(head);

	//cout<<"LQP tree:"<<endl;
	printLQP(head);

	//cout<<"after optimize tree:"<<endl;
	optimizeLQP(head);
	printLQP(head);

	//cout<<"after postfix:"<<endl;
	postfixLQP(head);
	printLQP(head);

	generatePQP(head, schema_manager, mem);
	return head->view;
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
			i--;
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
	// if both distinct and sort exist, set the order of distinct to be the same as sort!
	if (N->type == SORT){
		select_opt["DISTINCT"].push_back(N->param[0]);	
	}
	else if (N->type == SELECT){
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
		// decide which condition to use for this PRODUCT node
		if (select_opt.find("PRODUCT") != select_opt.end()){
			vector<string> to_pro = select_opt["PRODUCT"];
			// has OR
			if (find(to_pro.begin(), to_pro.end(), "OR") != to_pro.end()){
				N->param.insert(N->param.end(), to_pro.begin(), to_pro.end());
				select_opt["PRODUCT"].clear();
			}
			// only has AND
			else{
				vector<string> remain;
				int start_pos = 0;
				int i = 0;
				while (i < to_pro.size()){
					vector<string> clause;
					bool match_relation = false;
					// colect a clause
					while (i < to_pro.size() && to_pro[i] != "AND"){
						clause.push_back(to_pro[i]);
						string relation_name = splitBy(to_pro[i], ".")[0];
						// check if clause has the children's relation name 
						for (int j = 0; j < N->children.size(); j++){
							if (N->children[j]->type == LEAF){
								if (N->children[j]->param[0] == relation_name){
									match_relation = true;
								}
							}
						}
						i++;
					}
					// if this condition matches the children column name, leave it here
					if (match_relation){
						if (N->param.size() != 0) N->param.push_back("AND");
						N->param.insert(N->param.end(), clause.begin(), clause.end());
					}
					// otherwise, leave it in remain(select_opt)
					else{
						if (remain.size() != 0) remain.push_back("AND");
						remain.insert(remain.end(), clause.begin(), clause.end());
					}
					i++;
				}
				select_opt["PRODUCT"] = remain;
			}
		}
	}
	else if (N->type == LEAF){
		string relation_name = N->param[0];
		if (select_opt.find(relation_name) != select_opt.end()){
			// make a copy of N as L, set L as N's child
			Node *L = new Node(N->type, N->param, N->level+1);
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

	// handle exceptions 1: no product node
	if (N->type == SELECT && !select_opt["PRODUCT"].empty()){
		N->param.insert(N->param.end(), select_opt["PRODUCT"].begin(), select_opt["PRODUCT"].end());
	}
	// handle exceptions 2: both distinct and sort exist, set the order of distinct to be the same as sort!
	if (N->type == DISTINCT && !select_opt["DISTINCT"].empty()){
		N->param.insert(N->param.end(), select_opt["DISTINCT"].begin(), select_opt["DISTINCT"].end());
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
			cout<<T[level[i]->type]<<level[i]->children.size()<<" [ ";
			for (int j = 0; j < level[i]->param.size(); j++){
				cout<<level[i]->param[j];
				if (j != level[i]->param.size()-1) cout<<" ";
			}
			cout<<" ] ";
			for (int k = 0; k < level[i]->children.size(); k++){
				Q.push(level[i]->children[k]);
			}
		}
		cout<<endl;
	}
	cout<<"*****End of LQP tree ******"<<endl;
}



