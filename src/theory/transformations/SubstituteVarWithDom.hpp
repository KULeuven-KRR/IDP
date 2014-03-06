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

class SubstituteVarWithDom: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::map<Variable*, const DomainElement*> _var2domelem;

public:
	template<typename T>
	T execute(T t, const std::map<Variable*, const DomainElement*>& var2domelem) {
		_var2domelem = var2domelem;
		return t->accept(this);
	}
protected:
	Term* traverse(Term* t) {
		if (t->type() == TermType::VAR) {
			auto varterm = dynamic_cast<VarTerm*>(t);
			auto it = _var2domelem.find(varterm->var());
			if (it != _var2domelem.cend()) {
				auto sort = varterm->var()->sort();
				delete (varterm);
				return new DomainTerm(sort, it->second, TermParseInfo());
			}
		}
		return TheoryMutatingVisitor::traverse(t);
	}
};
