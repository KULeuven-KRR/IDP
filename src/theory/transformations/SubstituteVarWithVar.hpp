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
#include "visitors/TheoryMutatingVisitor.hpp"
#include <map>

class SubstituteVarWithVar: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::map<Variable*, Variable*> _var2var;

public:
	template<typename T>
	T execute(T t, const std::map<Variable*, Variable*>& var2var) {
		_var2var = var2var;
		return t->accept(this);
	}
protected:
	Term* traverse(Term* t) {
		if(t->type()==TermType::VAR){
			auto varterm = dynamic_cast<VarTerm*>(t);
			auto it = _var2var.find(varterm->var());
			if(it !=_var2var.cend()){
                            delete(t);
                            return new VarTerm(it->second,TermParseInfo());
			}
		}
		return TheoryMutatingVisitor::traverse(t);
	}
};
