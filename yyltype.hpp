#include <sstream>

#define YYLTYPE YYLTYPE
typedef struct YYLTYPE { 
	int first_line; 
	int first_column; 
	int last_line; 
	int last_column; 
	std::stringstream* descr; 
} YYLTYPE; 
