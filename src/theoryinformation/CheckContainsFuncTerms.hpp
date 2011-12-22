/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CONTAINSFUNCTERMS_HPP_
#define CONTAINSFUNCTERMS_HPP_

#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

class CheckContainsFuncTerms: public TheoryVisitor {
	VISITORFRIENDS()
private:
	bool _result;

public:
	bool execute(const Formula* f){
		_result = false;
		f->accept(this);
		return _result;
	}

protected:
	void visit(const FuncTerm*){
		_result = true;
	}
};

#endif /* CONTAINSFUNCTERMS_HPP_ */