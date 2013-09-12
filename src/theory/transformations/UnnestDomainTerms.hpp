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

#ifndef UNNESTDOMTERMS_HPP_
#define UNNESTDOMTERMS_HPP_

#include "UnnestTerms.hpp"

#include "IncludeComponents.hpp"

class Structure;
class Term;

class UnnestDomainTerms: public UnnestTerms {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return UnnestTerms::execute(t, NULL, NULL);
	}

protected:
	virtual bool wouldMove(Term* t);
};

class UnnestDomainTermsFromNonBuiltins: public UnnestDomainTerms {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return UnnestDomainTerms::execute(t);
	}
protected:
	virtual Formula* visit(PredForm*);


};

#endif /* UNNESTDOMTERMS_HPP_ */
