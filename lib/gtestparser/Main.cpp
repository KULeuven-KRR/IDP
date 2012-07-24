#include <string> // NOTE apparently necessary in older bison versions
#include "parser.hh"
#include <cstdio>
#include <sstream>
#include <iostream>

extern FILE* yyin;
extern int yyparse();
extern std::stringstream results;
extern std::stringstream ss;
extern std::stringstream error;

using namespace std;

int main(int, char**) {
	yyin = stdin;
	auto result = yyparse();
	if (ss.str() != "" || error.str()!="") {
		cerr << "ERROR ENCOUNTERED, ABORTED EARLY (possibly segfault): \n";
		if(error.str()!=""){
			cerr << error.str() << "\n";
		}
		if(ss.str()!=""){
			cerr << ss.str() << "\n";
		}
	} else {
		cout << "[   SUMMARY   ]\n";
		cout << results.str();
	}
	return result;
}
