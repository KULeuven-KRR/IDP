/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "UnnestDomainTerms.hpp"

bool UnnestDomainTerms::shouldMove(Term* t) {
	return isAllowedToUnnest() && t->type() == TermType::DOM;
}

Formula* UnnestDomainTermsFromNonBuiltins::visit(PredForm* pf){
	if( pf->symbol()->builtin()){
		return pf;
	}
	return UnnestDomainTerms::visit(pf);
}
