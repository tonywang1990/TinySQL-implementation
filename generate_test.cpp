// For generating some other test cases for report

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unordered_set>

using namespace std;

void append_3rd_table(ofstream& f_out){
	int i = 0;
	ifstream f_in("research.txt");
	while(f_in.is_open() && i++ < 10){
		string line;
		f_in>>line;
		string name;
		string field;
		istringstream iss(line);
		getline(iss, name, '\t');
		getline(iss, name, '\t');
		getline(iss, field, '\t');
		string output = "INSERT INTO research (name, area) VALUES (\""+ name + "\", " + "\" " + field + "\")";
		f_out<<output<<endl;  
	}
	f_out<<"# Test cross product first"<<endl;
	f_out<<"SELECT * FROM research, basic"<<endl;
	f_out<<"# Test select and project"<<endl;
	f_out<<"SELECT pid, professor.name FROM professor"<<endl;
	f_out<<"# Test select from where"<<endl;
	f_out<<"SELECT professor.name score FROM professor WHERE score > 50"<<endl;
	f_out<<"# Test natural join"<<endl;
	f_out<<"SELECT professor.name score area FROM professor, research WHERE professor.name = research.name"<<endl;
	f_out<<"# Test theta join might break the code"<<endl;
	f_out<<"#SELECT professor.name score website FROM professor, research, academic WHERE professor.name = research.name AND score > 50 OR professor.name = \"Robert Balog\""<<endl;
	f_out<<"# Test theta join"<<endl;
	f_out<<"SELECT professor.name score website FROM professor, research, academic WHERE professor.name = research.name AND score > 50"<<endl;
	f_out<<"# Test sorting"<<endl;
	f_out<<"SELECT professor.name score, website FROM professor, academic WHERE score > 50 ORDER BY score"<<endl;
	f_out<<"# Add distinct case manually!"<<endl;
	f_out<<"DROP TABLE research"<<endl;
	
}

int main(int argc, char ** argv){

	ifstream  f_in_p("basic.txt");
	ifstream  f_in_a("academic.txt");
	string line_p;
	string line_a;
	stringstream out;
	unordered_set<int> hash;
	hash.insert(20); 
	hash.insert(50); 
	hash.insert(100); 
	hash.insert(200); 
	hash.insert(400); 
	if(f_in_p.is_open() && f_in_a.is_open()){
		int cnt_line = 0;
		while(getline(f_in_p, line_p) && getline(f_in_a, line_a)){
			istringstream iss_p(line_p);
			istringstream iss_a(line_a);
			
			int field = 0;
			string value_p = "VALUES (";
			string value_a = "VALUES A";
			string token_p, token_a;
			while(getline(iss_p, token_p, '\t') && getline(iss_a, token_a, '\t') && field < 3){
				if(field == 0){
					value_p += token_p + ", ";
					value_a += token_a + ", ";
				}
				else if(field == 1){
					value_p +=  "\"" + token_p + "\", ";
					value_a +=  "\"" + token_a + "\", ";
				}
				else if(field  == 2){
					value_p += to_string(rand()%100) + ")";
					value_a += token_a.empty() ? "\"dummy\", " : "\"" + token_a + "\", ";
				}
				field++;
			}
			string output_p = "INSERT INTO professor (pid, name, score) " + value_a;
			string output_a = "INSERT INTO academic (pid, name, website) " + value_p;
			cout<<output_p<<endl;
			cout<<output_a<<endl;
			out<<output_p<<endl;
			out<<output_a<<endl;
			if(hash.find(++cnt_line) != hash.end()){
				string filename = "my_test_"+to_string(cnt_line)+ ".txt";
				ofstream f_out(filename.c_str());
				f_out<<"CREATE TABLE professor (pid INT, name STR20, score INT)"<<endl;
				f_out<<"CREATE TABLE academic (pid INT, name STR20, website STR20)"<<endl;
				f_out<<"CREATE TABLE research (name STR20, area STR20)"<<endl;
				f_out<<out.str()<<endl;
				append_3rd_table(f_out);
				f_out.close();
			}
		}
	}
}
