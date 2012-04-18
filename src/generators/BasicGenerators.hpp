/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef BASICGENERATOR_HPP_
#define BASICGENERATOR_HPP_

#include "InstGenerator.hpp"

/**
 * Generates an empty set of instances
 */
class EmptyGenerator: public InstGenerator {
public:
	virtual void next() {
	}
	virtual void reset() {
		notifyAtEnd();
	}

	EmptyGenerator* clone() const {
		return new EmptyGenerator(*this);
	}

	virtual void setVarsAgain(){

	}
};

class FullGenerator: public InstGenerator {
private:
	bool first;
public:
	FullGenerator()
			: first(true) {
	}

	FullGenerator* clone() const {
		return new FullGenerator(*this);
	}

	virtual void next() {
		if (first) {
			first = false;
		} else {
			notifyAtEnd();
		}
	}
	virtual void reset() {
		first = true;
	}

	virtual void setVarsAgain(){

	}
};

#endif /* BASICGENERATOR_HPP_ */
