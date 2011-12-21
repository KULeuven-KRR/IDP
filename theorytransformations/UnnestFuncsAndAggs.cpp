/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "theorytransformations/UnnestFuncsAndAggs.hpp"

bool UnnestFuncsAndAggs::shouldMove(Term* t) {
	if(getAllowedToUnnest()) {
		switch (t->type()) {
		case TT_FUNC: 
			return true;
		case TT_AGG:
			return true;
		default:
			break;
		}
	}
	return false;
}

