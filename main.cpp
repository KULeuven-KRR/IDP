/************************************
	main.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cstdio>
#include <iostream>
#include <cstdlib>

using namespace std;

// Parser stuff
extern int yyparse();
extern FILE* yyin;
extern map<string,CLConst*>	clconsts;
extern void parsestring(const string&);

// Lua stuff
extern int idpcall(lua_State*);
extern int overloadcall(lua_State*);
extern int idpfunccall(lua_State*);
extern int idppredcall(lua_State*);
extern int overloaddiv(lua_State*);


/** Initialize data structures **/
void initialize() {
	BuiltinProcs::initialize();
	Insert::initialize();
}

/** Help message **/
void usage() {
	cout << "Usage:\n"
		 << "   gidl [options] [filename [filename [...]]]\n\n";
	cout << "Options:\n";
	cout << "    -i, --interactive    run in interactive mode\n";
	cout << "    -e \"<proc>\"          run procedure <proc> after parsing\n"
		 << "    --statistics         show statistics\n"
		 << "    --verbose            print additional information\n"
		 << "    -c <name1>=<name2>   substitute <name2> for <name1> in the input\n"
		 << "    -I                   read from stdin\n"
		 << "    -W                   suppress all warnings\n"
		 << "    -v, --version        show version number and stop\n"
		 << "    -h, --help           show this help message\n\n";
	exit(0);
}

/** Parse command line constants **/
void setclconst(string name1, string name2) {
	CLConst* c;
	if(isInt(name2)) 
		c = new IntClConst(stoi(name2));
	else if(isDouble(name2)) 
		c = new DoubleClConst(stod(name2));
	else if(name2.size() == 1) 
		c = new CharCLConst(name2[0],false);
	else if(name2.size() == 3 && name2[0] == '\'' && name2[2] == '\'') 
		c = new CharCLConst(name2[1],true); 
	else if(name2.size() >= 2 && name2[0] == '"' && name2[name2.size()-1] == '"') 
		c = new StrClConst(name2.substr(1,name2.size()-2),true);
	else c = new StrClConst(name2,false);
	clconsts[name1] = c;
}

/** Parse command line options **/
vector<string> read_options(int argc, char* argv[]) {
	vector<string> inputfiles;
	argc--; argv++;
	while(argc) {
		string str(argv[0]);
		argc--; argv++;
		if(str == "-e" || str == "--execute")  		{ _cloptions._exec = string(argv[0]); 
														argc--; argv++;						}
#ifdef USEINTERACTIVE
		else if(str == "-i" || str == "--interactive")	{ _cloptions._interactive = true;	}
#endif
		else if(str == "--statistics")				{ _cloptions._statistics = true;		}
		else if(str == "--verbose")					{ _cloptions._verbose = true;			}
		else if(str == "-c")						{ str = argv[0];
													  if(argc && (str.find('=') != string::npos)) {
														  int p = str.find('=');
														  string name1 = str.substr(0,p);
														  string name2 = str.substr(p+1,str.size());
														  setclconst(name1,name2); 
													  }
													  else Error::constsetexp();
													  argc--; argv++;
													}
		else if(str == "-I")						{ _cloptions._readfromstdin = true;	}
		else if(str == "-W")						{ for(unsigned int n = 0; n < _cloptions._warning.size(); ++n)
														  _cloptions._warning[n] = false;
													}
		else if(str == "-v" || str == "--version")	{ cout << "GidL 2.0.1\n"; exit(0);	}
		else if(str == "-h" || str == "--help")		{ usage(); exit(0);					}
		else if(str[0] == '-')						{ Error::unknoption(str);			}
		else										{ inputfiles.push_back(str);		}
	}
	return inputfiles;
}

/** Parse input files **/
void parsefile(const string& str) {
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	yyin = fopen(str.c_str(),"r");
	if(yyin) {
		Insert::currfile(str);
		yyparse();
		fclose(yyin);
		// TODO: de 'using' vocabularia van de global namespace uitvegen
		// en er globale variabelen van maken...?
	}
	else Error::unknfile(str);
}

void parse(const vector<string>& inputfiles) {
	// Parse standard input file
	stringstream ss;
	ss <<DATADIR <<"/std/idp_intern.idp";
	yyin = fopen(ss.str().c_str(),"r");
	if(yyin==NULL) Error::unknfile(ss.str());
	else {
		yyparse();
		fclose(yyin);

		// Parse files of the user
		for(unsigned int n = 0; n < inputfiles.size(); ++n) {
			parsefile(inputfiles[n]);
		}
		if(_cloptions._readfromstdin) {
			yyin = stdin;
			Insert::currfile(0);
			yyparse();
		}
	}
}

/** Communication with lua **/

void fillmetatable(lua_State* L, bool index, bool newindex, const string& type) {

	if(index) {
		lua_getglobal(L,"idp_intern_index");
		lua_setfield(L,-2,"__index");
	}
	if(newindex) {
		lua_getglobal(L,"idp_intern_newindex");
		lua_setfield(L,-2,"__newindex");
	}

	lua_getglobal(L,"idp_intern_delete");
	lua_setfield(L,-2,"__gc");

	lua_pushstring(L,type.c_str());
	lua_setfield(L,-2,"idptype");
}

void createmetatables(lua_State* L) {

	luaL_newmetatable(L,"overloaded");
	fillmetatable(L,true,true,"overloaded");
	lua_getglobal(L,"idp_intern_overloadcall");
	lua_setfield(L,-2,"__call");
	lua_getglobal(L,"idp_intern_overloaddiv");
	lua_setfield(L,-2,"__div");
	lua_pop(L,1);

	luaL_newmetatable (L,"theory");
	fillmetatable(L,false,false,"theory");
	lua_pop(L,1);

	luaL_newmetatable (L,"structure");
	fillmetatable(L,true,true,"structure");
	lua_pop(L,1);

	luaL_newmetatable (L,"namespace");
	fillmetatable(L,true,false,"namespace");
	lua_pop(L,1);

	luaL_newmetatable (L,"vocabulary");
	fillmetatable(L,true,false,"vocabulary");
	lua_pop(L,1);

	luaL_newmetatable (L,"options");
	fillmetatable(L,true,true,"options");
	lua_pop(L,1);

	luaL_newmetatable(L,"sort");
	fillmetatable(L,false,false,"sort");
	lua_pop(L,1);

	luaL_newmetatable(L,"predicate_symbol");
	fillmetatable(L,true,false,"predicate_symbol");
	lua_getglobal(L,"idp_intern_div");
	lua_setfield(L,-2,"__div");
	lua_pop(L,1);

	luaL_newmetatable(L,"function_symbol");
	fillmetatable(L,true,false,"function_symbol");
	lua_getglobal(L,"idp_intern_div");
	lua_setfield(L,-2,"__div");
	lua_pop(L,1);

	luaL_newmetatable(L,"function_interpretation");
	fillmetatable(L,true,true,"function_interpretation");
	lua_getglobal(L,"idp_intern_funccall");
	lua_setfield(L,-2,"__call");
	lua_pop(L,1);

	luaL_newmetatable(L,"predicate_interpretation");
	fillmetatable(L,true,true,"predicate_interpretation");
	lua_pop(L,1);

	luaL_newmetatable(L,"predicate_table");
	fillmetatable(L,true,true,"predicate_table");
	lua_getglobal(L,"idp_intern_len");
	lua_setfield(L,-2,"__len");
	lua_getglobal(L,"idp_intern_predcall");
	lua_setfield(L,-2,"__call");
	lua_pop(L,1);

	luaL_newmetatable(L,"tuple");
	fillmetatable(L,true,true,"tuple");
	lua_getglobal(L,"idp_intern_len");
	lua_setfield(L,-2,"__len");
	lua_pop(L,1);
	
}


lua_State* initLua() {
		
	// Create the lua state
	lua_State* L = lua_open();
	luaL_openlibs(L);

	// Create the main communication functions
	lua_pushcfunction(L,&idpcall);
	lua_setglobal(L,"idp_intern_idpcall");
	lua_pushcfunction(L,&overloadcall);
	lua_setglobal(L,"idp_intern_overloadcall");
	lua_pushcfunction(L,&idpfunccall);
	lua_setglobal(L,"idp_intern_funccall");
	lua_pushcfunction(L,&idppredcall);
	lua_setglobal(L,"idp_intern_predcall");
	lua_pushcfunction(L,&overloaddiv);
	lua_setglobal(L,"idp_intern_overloaddiv");

	luaL_dostring(L,"idp_intern_index = function(t,k) return idp_intern_idpcall(\"index\",t,k) end");
	luaL_dostring(L,"idp_intern_len = function(t) return idp_intern_idpcall(\"len\",t) end");
	luaL_dostring(L,"idp_intern_div = function(o,a) return idp_intern_idpcall(\"aritycast\",o,a) end");

	luaL_dostring(L,"idp_intern_delete = function(obj) idp_intern_idpcall(\"delete\",obj) end");
	luaL_dostring(L,"idp_intern_newindex = function(t,k,v) idp_intern_idpcall(\"newindex\",t,k,v) end");

	// Create metatables for the different userdata
	createmetatables(L);

	// Overwrite some standard lua procedures
	stringstream ss;
	ss << DATADIR << "/std/idp_intern.lua";
	luaL_dofile(L,ss.str().c_str());

	// Make the objects in the global namespace global variables in lua 
	Namespace::global()->toLuaGlobal(L);

	return L;
}

/** Execute a procecure **/

void executeproc(lua_State* L, const string& proc) {
	if(proc != "") {
		string str = "##intern##{"+proc+'}';
		parsestring(str);
		LuaProcedure* proc = Insert::currproc();
		luaL_loadstring(L,(proc->code()).c_str());
		delete(proc);
		int res = lua_pcall(L,0,0,0);
		if(res) {
			cerr << string(lua_tostring(L,1)) << endl; 
			lua_pop(L,1);
		}
	}
}

/** Interactive mode **/

void interactive(lua_State* L) {
	cout << "Running GidL in interactive mode.\n"
		 << "  Type 'exit' to quit.\n\n";

#ifdef USEINTERACTIVE
	idp_rl_start();
	while(true) {
		char* userline = rl_gets();
		if(userline) {
			if(string(userline) == "exit") {
				free(userline);
				idp_rl_end();
				return;
			}
			else {
				string str = "##intern##{"+string(userline)+'}';
				parsestring(str);
				LuaProcedure* proc = Insert::currproc();
				luaL_loadstring(L,(proc->code()).c_str());
				delete(proc);
				int res = lua_pcall(L,0,0,0);
				if(res) {
				cerr << string(lua_tostring(L,1)) << endl;
				}
			}
		}
		else cout << "\n";
	}
#else
	cout << "> ";
	string userline = cin.getline();
	while(userline != "exit") {
		string str = "##intern##{"+userline+'}';
		parsestring(str);
		LuaProcedure* proc = Insert::currproc();
		luaL_loadstring(L,(proc->code()).c_str());
		delete(proc);
		int res = lua_pcall(L,0,0,0);
		if(res) {
			cerr << string(lua_tostring(L,1)) << endl;
		}
		userline = cin.getline();
	}
#endif
}


/** Delete all data **/
void cleanup() {
	Insert::cleanup();
	BuiltinProcs::cleanup();
	delete(DomainData::instance());
	for(map<string,CLConst*>::iterator it = clconsts.begin(); it != clconsts.end(); ++it) delete(it->second);
}

/** Main **/
int main(int argc, char* argv[]) {

	// Parse idp input
	initialize();
	vector<string> inputfiles = read_options(argc,argv);
	parse(inputfiles);

	// Run
	if(!Error::nr_of_errors()) {

		// Initialize communication with lua
		lua_State* L = initLua();

		// Execute statements
		executeproc(L,_cloptions._exec);
		if(_cloptions._interactive) interactive(L);
		else if(_cloptions._exec == "") executeproc(L,"idp_intern_main()");

		// End lua communication
		lua_close(L);
	}

	// Exit
	cleanup();
	return Error::nr_of_errors();
}
