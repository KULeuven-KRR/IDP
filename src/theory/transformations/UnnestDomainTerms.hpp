/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef UNNESTDOMTERMS_HPP_
#define UNNESTDOMTERMS_HPP_

#include "UnnestTerms.hpp"

#include "IncludeComponents.hpp"

class AbstractStructure;
class Term;

class UnnestDomainTerms: public UnnestTerms {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t, const AbstractStructure* str, Context con) {
		auto voc = (str != NULL) ? str->vocabulary() : NULL;
		return UnnestTerms::execute(t, con, str, voc);
	}

protected:
	virtual bool shouldMove(Term* t);
};

#endif /* UNNESTDOMTERMS_HPP_ */
