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
	cout << "[ SUMMARY  ]\n";
	if (ss.str() != "" or error.str() != "") {
		cout << "[  ERROR   ] Encountered error which interrupted at least one batch of tests (possibly segfault): \n";
		if (error.str() != "") {
			cout << error.str() << "\n";
		}
		if (ss.str() != "") {
			cout << ss.str() << "\n";
		}
		cout << "[REMAINDER]\n";
	}
	cout << results.str();
	return result;
}
