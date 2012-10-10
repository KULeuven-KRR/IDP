/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#pragma once

#include <sstream>
#include <string>
#include "utils/PrintStacktrace.hpp"
#include "IdpException.hpp"

class UnsatException: public Exception {
public:
	UnsatException(){

	}
	std::string getMessage() const {
		std::stringstream ss;
		ss << "Unsat found during grounding.";
		return ss.str();
	}
};
