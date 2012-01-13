/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef CHECKFUNCTERMS_HPP_
#define CHECKFUNCTERMS_HPP_

#include "visitors/TheoryVisitor.hpp"

class FormulaFuncTermChecker: public TheoryVisitor {
	VISITORFRIENDS()
private:
	bool _result;
protected:
	void visit(const FuncTerm*) {
		_result = true;
	}
public:
	bool execute(Formula* f) {
		_result = false;
		f->accept(this);
		return _result;
	}
};

#endif /* CHECKFUNCTERMS_HPP_ */
