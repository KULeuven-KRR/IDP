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

#ifndef BASICCHECKERS_HPP_
#define BASICCHECKERS_HPP_

#include "InstGenerator.hpp"

class FalseInstChecker: public InstChecker {
public:
	bool check() {
		return false;
	}
	FalseInstChecker* clone() const {
		return new FalseInstChecker(*this);
	}
};

class TrueInstChecker: public InstChecker {
public:
	bool check() {
		return true;
	}
	TrueInstChecker* clone() const {
		return new TrueInstChecker(*this);
	}
};

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

	virtual void internalSetVarsAgain(){

	}

	void put(std::ostream& stream) const {
		stream << "false";
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

	virtual void internalSetVarsAgain(){

	}

	void put(std::ostream& stream) const {
		stream << "true";
	}
};

#endif /* BASICCHECKERS_HPP_ */
