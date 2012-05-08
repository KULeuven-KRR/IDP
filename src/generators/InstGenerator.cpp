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

template<>
std::string toString(const Pattern& type){
	std::stringstream output;
	switch (type) {
	case Pattern::INPUT:
		output << "in";
		break;
	case Pattern::OUTPUT:
		output << "out";
		break;
	}
	return output.str();
}

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
	std::stringstream ss;
	ss << "Resetting variables for " << (typeid(*this).name()) << "\n";
	throw notyetimplemented(ss.str());
}
