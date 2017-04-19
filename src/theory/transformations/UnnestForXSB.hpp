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

#include "UnnestTerms.hpp"
#include "IncludeComponents.hpp"

class Structure;
class Term;

class UnnestForXSB : public UnnestTerms {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t, const Structure* str) {
		auto voc = (str != NULL) ? str->vocabulary() : NULL;
		return UnnestTerms::execute(t, str, voc);
	}

protected:
	bool wouldMove(Term* t) {
		if (t->type() == TermType::FUNC) {
			FuncTerm* f = dynamic_cast<FuncTerm*>(t);
			return not f->function()->isConstructorFunction();
		} else {
			return (t->type() == TermType::AGG);
		}
	}
};

