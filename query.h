void Insert(vector<string> &words, string &line, SchemaManager &schema_manager, MainMemory &mem);
Relation* Select(vector<string> &words, SchemaManager &schema_manager, MainMemory &mem);
void Delete(vector<string> &words, SchemaManager &schema_manager, MainMemory &mem);
void preProcess(const vector<string> &tables, vector<string> &words, SchemaManager &schema_manager);


