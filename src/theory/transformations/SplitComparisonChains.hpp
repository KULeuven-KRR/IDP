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

#include <cstddef>
#include <vector>
#include "visitors/TheoryMutatingVisitor.hpp"

class Vocabulary;

/**
 * Removes EqChainForm formulas from theory by replacing them with conjunctions
 * or disjunctions of binary comparisons (BoolForms and PredForms).
 */
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
