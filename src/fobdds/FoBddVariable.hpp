/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef FOBDDVARIABLE_HPP_
#define FOBDDVARIABLE_HPP_

#include "fobdds/FoBddTerm.hpp"
#include "vocabulary/vocabulary.hpp"

class FOBDDManager;
class Variable;

class FOBDDVariable: public FOBDDTerm {
private:
	friend class FOBDDManager;

	Variable* _variable;

	FOBDDVariable(Variable* var)
			: _variable(var) {
	}

public:
	bool containsDeBruijnIndex(unsigned int) const {
		return false;
	}

	Variable* variable() const {
		return _variable;
	}
	Sort* sort() const;

	void accept(FOBDDVisitor*) const;
	const FOBDDTerm* acceptchange(FOBDDVisitor*) const;
	virtual std::ostream& put(std::ostream& output) const;

};

//This struct is needed in order to always quantify !x y in the same order.
struct CompareBDDVars {
	bool operator()(const FOBDDVariable* v1, const FOBDDVariable* v2) const {
		if (v1->sort() < v2->sort()) {
			return true;
		}
		if (v1->sort() > v2->sort()) {
			return false;
		}
		if (v1->variable()->name() < v2->variable()->name()) {
			return true;
		}
		if (v1->variable()->name() > v2->variable()->name()) {
			return false;
		}
		return v1 < v2;
	}
};

#endif /* FOBDDVARIABLE_HPP_ */
