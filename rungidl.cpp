/************************************
	main.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <map>
#include "clconst.hpp"
#include "common.hpp"
#include "error.hpp"
#include "luaconnection.hpp"
#include "interactive.hpp"
#include "rungidl.hpp"
#include "insert.hpp"
using namespace std;

// seed
int global_seed;	//!< seed used for bdd estimators

// Parser stuff
extern map<string,CLConst*>	clconsts;
extern void parsestring(const string&);
extern void parsefile(const string&);
extern void parsestdin();

std::ostream& operator<<(std::ostream& stream, Status status){
	stream <<(status==Status::FAIL?"failed":"success");
	return stream;
}

/**
 * Print help message and stop
 **/
void usage() {
	cout << "Usage:\n"
		 << "   gidl [options] [filename [filename [...]]]\n\n";
	cout << "Options:\n";
	cout << "    -i, --interactive    run in interactive mode\n";
	cout << "    -e \"<proc>\"          run procedure <proc> after parsing\n"
		 << "    -c <name1>=<name2>   substitute <name2> for <name1> in the input\n"
		 << "    --seed=N             use N as seed for the random generator\n"
		 << "    -I                   read from stdin\n"
		 << "    -v, --version        show version number and stop\n"
		 << "    -h, --help           show this help message\n\n";
	exit(0);
}

/** 
 * Parse command line constants 
 **/
void setclconst(string name1, string name2) {
	CLConst* c;
	if(isInt(name2)) 
		c = new IntClConst(toInt(name2));
	else if(isDouble(name2)) 
		c = new DoubleClConst(toDouble(name2));
	else if(name2.size() == 1) 
		c = new CharCLConst(name2[0],false);
	else if(name2.size() == 3 && name2[0] == '\'' && name2[2] == '\'') 
		c = new CharCLConst(name2[1],true); 
	else if(name2.size() >= 2 && name2[0] == '"' && name2[name2.size()-1] == '"') 
		c = new StrClConst(name2.substr(1,name2.size()-2),true);
	else c = new StrClConst(name2,false);
	clconsts[name1] = c;
}

struct CLOptions {
	string	_exec;
#ifdef USEINTERACTIVE
	bool	_interactive;
#endif
	bool	_readfromstdin;
	CLOptions()
		: _exec("")
#ifdef USEINTERACTIVE
		, _interactive(false)
#endif
		, _readfromstdin(false) { }
};

/** 
 * Parse command line options 
 **/
vector<string> read_options(int argc, char* argv[], CLOptions& cloptions) {
	vector<string> inputfiles;
	argc--; argv++;
	while(argc) {
		string str(argv[0]);
		argc--; argv++;
		if(str == "-e" || str == "--execute")			{ cloptions._exec = string(argv[0]); 
														  argc--; argv++;					}
#ifdef USEINTERACTIVE
		else if(str == "-i" || str == "--interactive")	{ cloptions._interactive = true;	}
#endif
		else if(str == "-c")							{ str = argv[0];
														  if(argc && (str.find('=') != string::npos)) {
															  int p = str.find('=');
															  string name1 = str.substr(0,p);
															  string name2 = str.substr(p+1,str.size());
															  setclconst(name1,name2); 
														  }
														  else Error::constsetexp();
														  argc--; argv++;
														}
		else if(str.substr(0,7) == "--seed=")			{ global_seed = toInt(str.substr(7,str.size()));	}
		else if(str == "-I")							{ cloptions._readfromstdin = true;	}
		else if(str == "-v" || str == "--version")		{ cout << "GidL 2.0.1\n"; exit(0);	}
		else if(str == "-h" || str == "--help")			{ usage(); exit(0);					}
		else if(str[0] == '-')							{ Error::unknoption(str);			}
		else											{ inputfiles.push_back(str);		}
	}
	return inputfiles;
}

/**
 * Parse all input files
 */
void parse(const vector<string>& inputfiles) {
	for(unsigned int n = 0; n < inputfiles.size(); ++n) {
		parsefile(inputfiles[n]);
	}
}

/** 
 * Execute a procecure 
 **/
const DomainElement* executeproc(const string& proc) {
	if(proc != "") {
		return Insert::exec(proc);
	}
	return NULL;
}


#ifdef USEINTERACTIVE
/** 
 * Interactive mode 
 **/
void interactive() {
	string help1 = "help", help2 = "help()", exit1 = "exit", exit2 = "exit()";

	cout << "Running GidL in interactive mode.\n"
		 << "  Type 'exit' to quit.\n"
		 << "  Type 'help' for help\n\n";

	idp_rl_start();
	while(true) {
		char* userline = rl_gets();
		if(userline!=NULL) {
			string command(userline);
			if(command==exit1 || command==exit2) {
				free(userline);
				idp_rl_end();
				return;
			}
			if(command==help1){
				command = help2;
			}
			parsestring("##intern##{"+command+'}');
		}
		else cout << "\n";
	}
}
#endif

Insert insert;

// TODO merge with main method
void run(const std::string& inputfileurl){
	run({inputfileurl});
}

Status run(const std::vector<std::string>& inputfileurls){
	insert = Insert();
	LuaConnection::makeLuaConnection();

	parse(inputfileurls);

	Status result = Status::FAIL;
	if(Error::nr_of_errors() == 0) {
		stringstream ss;
		ss <<getLibraryName() <<".main()";
		auto value = executeproc(ss.str());
		if(value!=NULL && value->type()==DomainElementType::DET_INT && value->value()._int == 1){
			result = Status::SUCCESS;
		}
	}

	if(Error::nr_of_errors()>0){
		result = Status::FAIL;
	}

	LuaConnection::closeLuaConnection();

	return result;
}

int run(int argc, char* argv[]) {
	insert = Insert();
	LuaConnection::makeLuaConnection();

	// Parse idp input
	CLOptions cloptions;
	vector<string> inputfiles = read_options(argc,argv,cloptions);
	parse(inputfiles);
	if(cloptions._readfromstdin) parsestdin();

	// Run
	if(not Error::nr_of_errors()) {
		executeproc(cloptions._exec);
#ifdef USEINTERACTIVE
		if(cloptions._interactive){
			interactive();
		} else
#endif
		if(cloptions._exec == ""){
			stringstream ss;
			ss <<getLibraryName() <<".main()";
			executeproc(ss.str());
		}
	}

	LuaConnection::closeLuaConnection();

	return Error::nr_of_errors();
}
