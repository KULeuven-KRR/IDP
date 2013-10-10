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

#include "visitors/TheoryMutatingVisitor.hpp"
#include "common.hpp"
#include <vector>
#include "structure/DomainElement.hpp"

class CalculateKnownArithmetic: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t, const Structure* s) {
		_struc = s;
		return t->accept(this);
	}
private:
	const Structure* _struc;
protected:
	//TODO can still be improved: calculate more then only functerms
	Term* visit(FuncTerm* ft) {
		auto newFuncTerm = dynamic_cast<FuncTerm*>(traverse(ft));
		auto f = newFuncTerm->function();
		if (_struc->inter(f)->approxTwoValued()) {
			std::vector<const DomainElement*> tuple(newFuncTerm->subterms().size());
			int i = 0;
			bool allDomainElements = true;
			for (auto term = newFuncTerm->subterms().cbegin(); term != newFuncTerm->subterms().cend(); term++, i++) {
				auto temp = dynamic_cast<const DomainTerm*>(*term);
				if (temp != NULL) {
					tuple[i] = temp->value();
				} else {
					allDomainElements = false;
					break;
				}
			}
			if (allDomainElements) {
				auto result = f->interpretation(_struc)->funcTable()->operator [](tuple);
				if (result == NULL) {
					//TODO: what should happen here?
					//I think smarter things can be done (like passing on the NULL to the superformula)
					return newFuncTerm;
				}
				auto finalresult = new DomainTerm(ft->sort(), result, ft->pi());
				newFuncTerm->recursiveDelete();
				return finalresult;
			}
		}
		return newFuncTerm;
	}
};
