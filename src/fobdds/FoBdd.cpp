/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddKernel.hpp"
#include "fobdds/FoBddVisitor.hpp"

bool FOBDD::containsDeBruijnIndex(unsigned int index) const {
	if (_kernel->containsDeBruijnIndex(index)) {
		return true;
	} else if (_falsebranch != NULL && _falsebranch->containsDeBruijnIndex(index)) {
		return true;
	} else if (_truebranch != NULL && _truebranch->containsDeBruijnIndex(index)) {
		return true;
	} else {
		return false;
	}
}

void FOBDD::accept(FOBDDVisitor* visitor) const {
	visitor->visit(this);
}
