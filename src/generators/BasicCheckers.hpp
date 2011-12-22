/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef BASICCHECKERS_HPP_
#define BASICCHECKERS_HPP_

#include "generators/InstGenerator.hpp"

class FalseInstChecker : public InstChecker {
public:
	bool check() { return false; }
	FalseInstChecker* clone() const{
		return new FalseInstChecker(*this);
	}
};

class TrueInstChecker : public InstChecker {
public:
	bool check() { return true; }
	TrueInstChecker* clone() const{
		return new TrueInstChecker(*this);
	}
};

#endif /* BASICCHECKERS_HPP_ */
