#include <string> // TODO apparently necessary in older bison versions
#include "parser.hh"
#include <cstdio>

extern FILE* yyin;
extern int yyparse();

using namespace std;

int main(int, char**){
	yyin = stdin;
	return yyparse();
}
