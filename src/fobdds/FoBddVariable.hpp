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

#pragma once

#include "fobdds/FoBddTerm.hpp"
#include "vocabulary/vocabulary.hpp"

class FOBDDManager;
class Variable;

class FOBDDVariable: public FOBDDTerm {
private:
	friend class FOBDDManager;

	Variable* _variable;

	FOBDDVariable(uint id, Variable* var)
			: 	FOBDDTerm(id),
				_variable(var) {
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

struct CompareBDDVars {
	bool operator()(const FOBDDVariable* lhs, const FOBDDVariable* rhs) const;
};

typedef std::set<const FOBDDVariable*, CompareBDDVars> fobddvarset;
