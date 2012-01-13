/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include <sstream>

#define YYLTYPE YYLTYPE
typedef struct YYLTYPE {
	int first_line;
	int first_column;
	int last_line;
	int last_column;
	std::stringstream* descr;
}YYLTYPE;
