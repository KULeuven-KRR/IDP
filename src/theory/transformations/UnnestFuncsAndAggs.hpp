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

#ifndef UNNESTFUNCSANDAGGS_HPP_
#define UNNESTFUNCSANDAGGS_HPP_

#include "UnnestTerms.hpp"
#include "IncludeComponents.hpp"

class Structure;
class Term;

class UnnestFuncsAndAggs: public UnnestTerms {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t, const Structure* str) {
		auto voc = (str != NULL) ? str->vocabulary() : NULL;
		return UnnestTerms::execute(t, str, voc);
	}

protected:
	bool wouldMove(Term* t) {
		return (t->type() == TermType::FUNC or t->type() == TermType::AGG);
	}
};

#endif /* UNNESTFUNCSANDAGGS_HPP_ */
