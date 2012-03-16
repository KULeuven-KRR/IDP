/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef INCONSISTENTSTRUCTURE_HPP_
#define INCONSISTENTSTRUCTURE_HPP_

#include "AbstractStructure.hpp"

class InconsistentStructure: public AbstractStructure {
public:
	InconsistentStructure()
			: AbstractStructure("Inconsistent Structure", ParseInfo()) {
	}
	InconsistentStructure(const std::string& name, const ParseInfo& pi)
			: AbstractStructure(name, pi) {
	}
	~InconsistentStructure() {
	}
	void inter(Predicate* p, PredInter* i); //!< set the interpretation of p to i
	void inter(Function* f, FuncInter* i); //!< set the interpretation of f to i
	void clean() {
	} //!< make three-valued interpretations that are in fact
	  //!< two-valued, two-valued.
	void materialize() {
	} //!< Convert symbolic tables containing a finite number of tuples to enumerated tables.

	SortTable* inter(Sort* s) const; // Return the domain of s.
	PredInter* inter(Predicate* p) const; // Return the interpretation of p.
	FuncInter* inter(Function* f) const; // Return the interpretation of f.
	PredInter* inter(PFSymbol* s) const; // Return the interpretation of s.

	const std::map<Predicate*, PredInter*>& getPredInters() const;
	const std::map<Function*, FuncInter*>& getFuncInters() const;

	AbstractStructure* clone() const; // take a clone of this structure

	Universe universe(const PFSymbol*) const;

	bool approxTwoValued() const;
	bool isConsistent() const;

	void makeTwoValued();
};

#endif /* INCONSISTENTSTRUCTURE_HPP_ */
