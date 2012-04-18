/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef REMOVEEQUATIONCHAINS_HPP_
#define REMOVEEQUATIONCHAINS_HPP_
#include <cstddef>
#include <vector>
#include "visitors/TheoryMutatingVisitor.hpp"

class Vocabulary;

class SplitComparisonChains: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	Vocabulary* _vocab;
public:
	template<typename T>
	T execute(T t, Vocabulary* v = NULL) {
		_vocab = v;
		return t->accept(this);
	}

protected:
	Formula* visit(EqChainForm*);
};

#endif /* REMOVEEQUATIONCHAINS_HPP_ */
