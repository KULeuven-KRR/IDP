/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#pragma once

#include "visitors/TheoryVisitor.hpp"
#include "theory/term.hpp"


class ContainedVariables: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	varset _vars;

public:
	template<class T>
	varset execute(const T* f) {
		f->accept(this);
		return _vars;
	}

protected:
	void visit(const VarTerm* vt) {
		_vars.insert(vt->var());
	}
};
