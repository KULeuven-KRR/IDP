/************************************
	main.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "insert.hpp"
#include "builtin.hpp"
#include "namespace.hpp"
#include "parse.h"
#include "error.hpp"
#include "options.hpp"
#include "clconst.hpp"
#include <cstdio>
#include <iostream>
#include <cstdlib>

// Parser stuff
extern int yyparse();
extern FILE* yyin;
extern map<string,CLConst*>	clconsts;
extern void parsestring(const string&);

extern void help_execute();

/** Initialize data structures **/
void initialize() {
	Insert::initialize();
}

/** Help message **/
void usage() {
	cout << "Usage:\n"
		 << "   gidl [options] [filename [filename [...]]]\n\n";
	cout << "Options:\n"
		 << "    -i, --interactive	  run in interactive mode\n"
		 << "    --statistics:        show statistics\n"
		 << "    --verbose:           print additional information\n"
		 << "    -c <name1>=<name2>:  substitute <name2> for <name1> in the input\n"
		 << "    -I:                  read from stdin\n"
		 << "    -W:                  suppress all warnings\n"
		 << "    -v, --version:       show version number and stop\n"
		 << "    -h, --help:          show this help message\n"
		 << "    --help-execute:      show all methods available in the execute block\n\n";
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
		if(str == "-i" || str == "--interactive")	{ _cloptions._interactive = true;		}
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
		else if(str == "-W")						{ for(unsigned int n = 0; n < _cloptions._warning.size(); ++n) {
														  _cloptions._warning[n] = false;
													  }
													}
		else if(str == "-v" || str == "--version")	{ cout << "GidL 2.0.1\n"; exit(0);	}
		else if(str == "-h" || str == "--help")		{ usage(); exit(0);					}
		else if(str == "--help-execute")			{ help_execute(); exit(0);			}
		else if(str[0] == '-')						{ Error::unknoption(str);			}
		else										{ inputfiles.push_back(str);		}
	}
	return inputfiles;
}

/** Parse input files **/
void parse(const vector<string>& inputfiles) {
	for(unsigned int n = 0; n < inputfiles.size(); ++n) {
		yylloc.first_line = 1;
		yylloc.first_column = 1;
		yyin = fopen(inputfiles[n].c_str(),"r");
		if(yyin) {
			Insert::currfile(inputfiles[n]);
			yyparse();	
			fclose(yyin);
			// TODO: de 'using' vocabularia van de global namespace uitvegen
		}
		else Error::unknfile(inputfiles[n]);
	}
	if(_cloptions._readfromstdin) {
		yyin = stdin;
		Insert::currfile(0);
		yyparse();
	}
}

/** Interactive mode **/
void interactive() {
	cout << "Running GidL in interactive mode.\n"
		 << "  Type 'help' for help.\n"
		 << "  Type 'exit' to quit.\n\n";

	while(true) {
		cout << "> ";
		string str;
		getline(cin,str);
		if(str == "exit") return;
		else if(str == "help") {
			help_execute();
		}
		else {
			str = "#execute{ " + str + "}";
			parsestring(str);
		}
	}
}

/** Delete all data **/
void cleanup() {
	Insert::cleanup();
	for(map<string,CLConst*>::iterator it = clconsts.begin(); it != clconsts.end(); ++it) delete(it->second);
}

/** Main **/
int main(int argc, char* argv[]) {
	initialize();
	vector<string> inputfiles = read_options(argc,argv);
	parse(inputfiles);
	if(_cloptions._interactive) interactive();
	cleanup();
	return Error::nr_of_errors();
}
