#include <string> // TODO apparently necessary in older bison versions
#include "parser.hh"
#include <cstdio>

extern FILE* yyin;
extern int yyparse();

using namespace std;

int main(int argc, char** argv){
	yyin = stdin;
	return yyparse();
}