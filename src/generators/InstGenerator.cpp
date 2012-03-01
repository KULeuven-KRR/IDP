/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "common.hpp"
#include "InstGenerator.hpp"
#include "GlobalData.hpp"

void InstChecker::put(std::ostream& stream) {
	stream << "generate: " << typeid(*this).name();
}

// Can also be used for resets
// SETS the instance to the FIRST value if it exists
/*void InstGenerator::begin() {
	end = false;
	reset();
	if (not isAtEnd()) {
		next();
	}
}*/

/*void InstGenerator::operator++() {
	CHECKTERMINATION
	Assert(not isAtEnd());
	next();
}
*/

void InstGenerator::setVarsAgain(){
	std::cerr <<typeid(*this).name() <<"\n";
	Assert(false);
}
