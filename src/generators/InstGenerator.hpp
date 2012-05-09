/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef INSTGENERATOR_HPP_
#define INSTGENERATOR_HPP_

#include <typeinfo>
#include <sstream>
#include <iostream>
#include "GlobalData.hpp"


enum class Pattern {
	INPUT, OUTPUT
};

template<>
std::string toString(const Pattern& type);

class InstChecker {
private:
	bool generatesInfiniteDomain;

public:
	InstChecker(): generatesInfiniteDomain(false){}
	virtual ~InstChecker() {}

	// FIXME Checker should only be created if there are no output variables
	virtual bool check() = 0;

	// NOTE: should be a deep clone
	virtual InstChecker* clone() const = 0; // FIXME need to reimplemnt some as a deep clone!

	virtual void put(std::ostream& stream) const;

	void notifyIsInfiniteGenerator(){
		generatesInfiniteDomain = true;
	}

	bool isInfiniteGenerator() const {
		return generatesInfiniteDomain;
	}
};

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

	virtual void setVarsAgain();

	// NOTE: important to always call new XGen(*this); to guarantee that all parent variables are set correctly!
	virtual InstGenerator* clone() const = 0;

	bool isInitialized() const { return initdone; }
};

#endif /* INSTGENERATOR_HPP_ */
