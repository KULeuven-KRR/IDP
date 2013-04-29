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

class CombineAggregates: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}
protected:
	Term* visit(FuncTerm* ft) {
		auto term = traverse(ft);
		auto newft = dynamic_cast<FuncTerm*>(term);
		if(newft==NULL || not is(newft->function(), STDFUNC::ADDITION)){
			return term;
		}

		auto aggone = dynamic_cast<AggTerm*>(newft->subterms()[0]);
		auto aggtwo = dynamic_cast<AggTerm*>(newft->subterms()[1]);

		if(aggone==NULL || aggtwo==NULL || aggone->function()!=AggFunction::SUM || aggone->function()!=aggtwo->function()){
			return term;
		}

		aggone->addSet(aggtwo->set());
		//aggone = aggone->clone();
		//aggtwo->recursiveDelete();
		//ft->recursiveDelete();
		return aggone;
	}
};
