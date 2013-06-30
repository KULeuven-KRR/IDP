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

#ifndef INSTGENERATOR_HPP_
#define INSTGENERATOR_HPP_

#include <typeinfo>
#include <sstream>
#include <iostream>
#include "GlobalData.hpp"


enum class Pattern {
	INPUT, OUTPUT
};

std::ostream& operator<<(std::ostream& stream, const Pattern& type);
PRINTTOSTREAM(Pattern)

class InstChecker {
private:
	bool generatesInfiniteDomain;

public:
	InstChecker(): generatesInfiniteDomain(false){}
	virtual ~InstChecker() {}

	// FIXME Checker should only be created if there are no output variables
	virtual bool check() = 0;

	// NOTE: should be a deep clone
	virtual InstChecker* clone() const = 0;

	virtual void put(std::ostream& stream) const;

	void notifyIsInfiniteGenerator(){
		generatesInfiniteDomain = true;
	}

	bool isInfiniteGenerator() const {
		return generatesInfiniteDomain;
	}
};

/**
 * InstGenerators are used to generate instantiations of variables, given some fixed instantiations
 * for some variables and some predicate or function to constrain the possibilities.
 *
 * One situation where generators are needed is during grounding, in conjunction with BDD's.
 *
 * E.g.: instantiating F(x,3)=y generates the possible values for x and y such that F(x,3)=y.
 * The encoding of F(x,3)=y is done by a pattern, where "true false true" would mean "output input output"
 * which in turn means "find instantiations for the first and the third (variable) given a value for the second".
 */
class InstGenerator: public InstChecker {
private:
	bool end;
	bool initdone;

protected:
	void notifyAtEnd() {
		end = true;
	}

	// Semantics: next should set the variables to a NOT-YET-SEEN instantiation.
	//		the combination of reset + next should set the variables to the first matching instantation.
	//	No assumption is made about calling reset on its own.
	//	Next will never be called when already at end.
	virtual void next() = 0;
	virtual void reset() = 0; // FIXME can probably make this static and drop all lower resets to this one

	virtual void internalSetVarsAgain();

public:
	InstGenerator():end(false),initdone(false){
	}
	virtual ~InstGenerator() {
	}

	virtual bool check() {
		begin();
		return not isAtEnd();
	}

	// Can also be used for resets
	// SETS the instance to the FIRST value if it exists
	inline void begin(){
		CHECKTERMINATION
		end = false;
		reset();
		if (not end) {
			next();
		}
		initdone = true;
	}

	/**
	 * Returns true if the last element has already been set as an instance
	 */
	inline bool isAtEnd() const {
		Assert(initdone);
		return end;
	}

	inline void operator++(){
		CHECKTERMINATION
		Assert(initdone);
		Assert(not isAtEnd());
		next();
	}

	virtual void setVarsAgain(){
		if(not initdone || isAtEnd()){
			return;
		}
		internalSetVarsAgain();
	}

	// NOTE: important to always call new XGen(*this); to guarantee that all parent variables are set correctly!
	virtual InstGenerator* clone() const = 0;

	bool isInitialized() const { return initdone; }
};

#endif /* INSTGENERATOR_HPP_ */
