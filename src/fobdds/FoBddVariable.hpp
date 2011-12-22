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

class FOBDDManager;
class Variable;

class FOBDDVariable: public FOBDDArgument {
private:
	friend class FOBDDManager;

	Variable* _variable;

	FOBDDVariable(Variable* var) :
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
	const FOBDDArgument* acceptchange(FOBDDVisitor*) const;
};

#endif /* FOBDDVARIABLE_HPP_ */