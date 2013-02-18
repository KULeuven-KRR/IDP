/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "common.hpp"
#include "InstGenerator.hpp"

std::ostream& operator<<(std::ostream& output, const Pattern& type){
	switch (type) {
	case Pattern::INPUT:
		output << "in";
		break;
	case Pattern::OUTPUT:
		output << "out";
		break;
	}
	return output;
}

void InstChecker::put(std::ostream& stream) const {
	stream << "generate: " << typeid(*this).name();
}
PRINTTOSTREAMIMPL(Pattern)

void InstGenerator::internalSetVarsAgain(){
	std::stringstream ss;
	ss << "Resetting variables for " << (typeid(*this).name()) << "\n";
	throw notyetimplemented(ss.str());
}
