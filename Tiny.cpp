#include "utility.h"
#include "Tiny.h"
#include "query.h"


using namespace std;

extern queue<int> free_blocks;

int main(int argc, char ** argv){
	// Initialize the memory, disk and the schema manager
	MainMemory mem;
	Disk disk;
	//cout << "The memory contains " << mem.getMemorySize() << " blocks" << endl;
	SchemaManager schema_manager(&mem,&disk);
	disk.resetDiskIOs();
	disk.resetDiskTimer();
	resetFreeBlocks();
	clock_t start_time;
	start_time=clock();

	//=======================Read Input=========================
	ifstream input;
	bool interactive_mode = false;
	assert(argv && argc >= 1);

	cout<<endl;
	cout<<"                   ";
	cout<<"============================================="<<endl<<endl;
	cout<<"                   ";
	cout<<"Welcome to TinySQL Database Management System"<<endl<<endl;
	cout<<"                   ";
	cout<<"Develped by Jimmy Jin & Tony Wang, 12/6/2016"<<endl<<endl;
	cout<<"                   ";
	cout<<"============================================="<<endl<<endl;


	if (argc == 1){
		cout<<endl<<"Entering Interactive Mode: Type query and ENTER to execute; Type EXIT to exit the program."<<endl<<endl;
		interactive_mode = true;
	}
	else if (argc == 2){
		string filename = argv[1];
		input.open(filename.c_str());
		if (!input.is_open()){
			cout<<"Cannot Open file: "<<filename<<endl;
			return 0;
		}
	}
	else{
		cout<<"To use TinySQL to read input file, type:   ";
		cout<<"./Tiny <filename>"<<endl; 
		cout<<"To use TinySQL in Interactive Mode, type:  ";
		cout<<"./Tiny"<<endl;
		return 0;
	}
	unsigned long int ios = 0;
	double time = 0;
	string line;
	vector<string> words;

	// for each command line
	while(1){
		if (interactive_mode){
			cout<<">>";
			char console_input[1000];
			cin.getline(console_input, sizeof(console_input));
			line = string(console_input);
			if (line == "EXIT") break;
			cout<<endl;
		}
		else{
			if (!getline(input, line)) break;
			cout<<line<<endl;	
		}
		if(line[0] == '#')	continue;
		if(line.size() == 0)	continue;
		// extract each word into vector words
		words = splitBy(line," ");

		// prepare memory
		resetFreeBlocks();

		if (words[0] == "CREATE"){
			Create(words, schema_manager, mem);
		}
		else if (words[0] == "DROP"){
			string relation_name = words[2];
			schema_manager.deleteRelation(relation_name);
		}
		else if (words[0] == "INSERT"){
			Insert(words, line, schema_manager, mem);
			//	cout<< *(schema_manager.getRelation(relation_name))<<endl;
		}
		else if (words[0] == "DELETE"){
			Delete(words, schema_manager, mem);
		}
		else if (words[0] == "SELECT"){
			Select(words, schema_manager, mem);
		}
		else{
			cout<<"Not a valid Tiny-SQL command!"<<endl<<endl;
			continue;
			//abort();
		}
		words.clear();
		cout << "Elapse time = " << disk.getDiskTimer() - time<< " ms" << endl;
		cout << "Disk I/Os = " << disk.getDiskIOs() - ios<< endl<<endl;
		time = disk.getDiskTimer();
		ios = disk.getDiskIOs();
	}
	if (!interactive_mode)	input.close();
	cout << "==== End of inputs ==== "<<endl;
	cout << "Total elapse time = " << ((double)(clock()-start_time)/CLOCKS_PER_SEC*1000) << " ms" << endl;
	cout << "Total Disk elapse time = " << disk.getDiskTimer() << " ms" << endl;
	cout << "Total Disk I/Os = " << disk.getDiskIOs() << endl;
	return 0;
}


