#include <string> // NOTE apparently necessary in older bison versions
#include "parser.hh"
#include <cstdio>
#include <sstream>
#include <iostream>

extern FILE* yyin;
extern int yyparse();
extern std::stringstream results;

using namespace std;

int main(int, char**){
	yyin = stdin;
	auto result = yyparse();
	cout <<"[   SUMMARY   ]\n";
	cout <<results.str();
	return result;
}
