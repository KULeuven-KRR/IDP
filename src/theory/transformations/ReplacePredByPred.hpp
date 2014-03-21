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
#include "IncludeComponents.hpp"

class ReplacePredByPred: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	Predicate* _origPred, *_newPred;
public:
	template<typename T>
	T execute(T t, Predicate* origPred, Predicate* newPred){
		_origPred = origPred;
		_newPred = newPred;
		Assert(origPred->sorts()==newPred->sorts());
		t = t->accept(this);
		return t;
	}

protected:
	Formula* visit(PredForm* pf){
		if(pf->symbol()==_origPred){
			pf->symbol(_newPred);
		}
		return pf;
	}
};
