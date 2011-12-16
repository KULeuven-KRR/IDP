/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef SPLITPRODUCTS_HPP_
#define SPLITPRODUCTS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

/**
 * Serves for splitting product aggregates in aggregates conaining only positive values, is needed for the solver
 * If the solver is ever adapted, this can disappear
 * ALLWAYS call graphaggregates before calling this to ensure that all aggterms appear in an aggform
 * IMPORTANT: if you call this twice, it will keep on splitting.  There are no checks to see if something is allready split.
 */
class SplitProducts: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t){
		return t->accept(this);
	}

protected:
	Formula* visit(AggForm* af);
};

#endif /* SPLITPRODUCTS_HPP_ */
