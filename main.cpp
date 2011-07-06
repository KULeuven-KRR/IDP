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
using namespace std;

// seed
int global_seed;

// Parser stuff
extern map<string,CLConst*>	clconsts;
extern void parsestring(const string&);
extern void parsefile(const string&);
extern void parsestdin();

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

struct CLOptions {
	string	_exec;
	bool	_interactive;
	bool	_readfromstdin;
	CLOptions() : _exec(""), _interactive(false), _readfromstdin(false) { }
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
		else if(str == "-i" || str == "--interactive")	{ cloptions._interactive = true;	}
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
		else if(str.substr(0,7) == "--seed=")			{ global_seed = stoi(str.substr(7,str.size()));	}
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
void executeproc(const string& proc) {
	if(proc != "") {
		string str = "##intern##{"+proc+'}';
		parsestring(str);
	}
}

/** 
 * Interactive mode 
 **/
void interactive() {
	cout << "Running GidL in interactive mode.\n"
		 << "  Type 'exit' to quit.\n"
		 << "  Type 'help()' for help\n\n";

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
		userline = cin.getline();
	}
#endif
}

/** 
 * Main 
 **/
int main(int argc, char* argv[]) {

	// Make lua connection
	LuaConnection::makeLuaConnection();

	// Parse idp input
	CLOptions cloptions;
	vector<string> inputfiles = read_options(argc,argv,cloptions);
	parse(inputfiles);
	if(cloptions._readfromstdin) parsestdin();

	// Run
	if(!Error::nr_of_errors()) {
		// Execute statements
		executeproc(cloptions._exec);
		if(cloptions._interactive) interactive();
		else if(cloptions._exec == ""){
			stringstream ss;
			ss <<getLibraryName() <<".main()";
			executeproc(ss.str());
		}
	}

	// Close lua communication
	LuaConnection::closeLuaConnection();

	// Exit
	return Error::nr_of_errors();
}
