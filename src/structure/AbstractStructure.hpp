/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef ABSTRACTSTRUCTURE_HPP_
#define ABSTRACTSTRUCTURE_HPP_

#include "common.hpp"
#include "parseinfo.hpp"

#include "Universe.hpp"

class Function;
class FuncInter;
class Predicate;
class PredInter;
class PFSymbol;
class Sort;
class SortTable;
class Vocabulary;


class AbstractStructure {
protected:
	std::string _name; // The name of the structure
	ParseInfo _pi; // The place where this structure was parsed.
	Vocabulary* _vocabulary; // The vocabulary of the structure.

public:
	// NOTE: cannot call changevoc in constructor (as its virtual), so CALL IN CHILD!
	AbstractStructure(std::string name, const ParseInfo& pi)
			: _name(name), _pi(pi), _vocabulary(NULL) {
	}
	virtual ~AbstractStructure();

	// Mutators
	virtual void changeVocabulary(Vocabulary* v);

	virtual void changeInter(Predicate* p, PredInter* i) = 0; //!< CHANGE the interpretation of p to i
	virtual void changeInter(Function* f, FuncInter* i) = 0; //!< CHANGE the interpretation of f to i
	virtual void clean() = 0; //!< make three-valued interpretations that are in fact
							  //!< two-valued, two-valued.

	virtual void materialize() = 0; //!< Convert symbolic tables containing a finite number of tuples to enumerated tables.

	// Inspectors
	const std::string& name() const {
		return _name;
	}
	ParseInfo pi() const {
		return _pi;
	}
	Vocabulary* vocabulary() const {
		return _vocabulary;
	}
	virtual SortTable* inter(Sort* s) const = 0; // Return the domain of s.
	virtual PredInter* inter(Predicate* p) const = 0; // Return the interpretation of p.
	virtual FuncInter* inter(Function* f) const = 0; // Return the interpretation of f.
	virtual PredInter* inter(PFSymbol* s) const = 0; // Return the interpretation of s.
	virtual const std::map<Predicate*, PredInter*>& getPredInters() const = 0;
	virtual const std::map<Function*, FuncInter*>& getFuncInters() const = 0;

	virtual AbstractStructure* clone() const = 0; // take a clone of this structure

	virtual Universe universe(const PFSymbol*) const = 0;

	virtual bool approxTwoValued() const = 0;

	// Note: loops over all tuples of all tables, SLOW!
	virtual bool isConsistent() const = 0;

	virtual void makeTwoValued() = 0;

	void put(std::ostream& s) const;
};

#endif /* ABSTRACTSTRUCTURE_HPP_ */
