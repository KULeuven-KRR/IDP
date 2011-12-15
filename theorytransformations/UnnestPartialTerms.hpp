/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef REMOVEPARTIALTERMS_HPP_
#define REMOVEPARTIALTERMS_HPP_

#include "theorytransformations/UnnestTerms.hpp"

class UnnestPartialTerms: public UnnestTerms {
	VISITORFRIENDS()
protected:
	bool shouldMove(Term* t) {
		return (getAllowedToUnnest() && TermUtils::isPartial(t));
	}
};

#endif /* REMOVEPARTIALTERMS_HPP_ */
