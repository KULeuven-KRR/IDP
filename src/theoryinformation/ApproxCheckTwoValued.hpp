/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef APPROXCHECKTWOVALUED_HPP_
#define APPROXCHECKTWOVALUED_HPP_

#include "common.hpp"
#include "visitors/TheoryVisitor.hpp"

class AbstractStructure;

class ApproxCheckTwoValued: public TheoryVisitor {
	VISITORFRIENDS()
private:
	AbstractStructure* _structure;
	bool _returnvalue;
public:
	template<typename T>
	bool execute(AbstractStructure* str, const T f){
		_structure = str;
		_returnvalue = true;
		f->accept(this);
		return _returnvalue;
	}
protected:
	void visit(const PredForm*);
	void visit(const FuncTerm*);
	void visit(const SetExpr*);
};

#endif /* APPROXCHECKTWOVALUED_HPP_ */