/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CALCULATE_HPP_
#define CALCULATE_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"
#include "common.hpp"
#include <vector>
#include "structure/DomainElement.hpp"

class CalculateKnownArithmetic: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}
protected:
	Term* visit(FuncTerm* ft) {
		auto newFuncTerm = dynamic_cast<FuncTerm*>(traverse(ft));
		auto f = newFuncTerm->function();
		if (f->builtin()) {
			//TODO: can be improved: it can be given a structure so that the calculations not only happen for builtins...
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
			if(allDomainElements){
				auto result = f->interpretation(NULL)->funcTable()->operator [](tuple);
				if(result == NULL){
					//TODO: what should happen here?
					//I think smarter things can be done (like passing on the NULL to the superformula)
					return newFuncTerm;
				}
				return new DomainTerm(ft->sort(),result,ft->pi());;
			}
		}
		return newFuncTerm;
	}
};

#endif /* CALCULATE_HPP_ */
