/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef UNNESTFUNCSANDAGGSNR_HPP_
#define UNNESTFUNCSANDAGGSNR_HPP_

#include "UnnestFuncsAndAggs.hpp"

#include "IncludeComponents.hpp"

class AbstractStructure;
class Term;

/**
 * Unnests functions and aggregates.  This method does not recursively nested fucntions and aggregates.
 * For Example P(F(G(x),y) & Q(H(j)), with P and Q predicats, F,G,H functions will be unnested to
 * ? z: z= F(G(x)) & P(z,y) & ? v: v=H(j) & Q(j)
 */
class UnnestFuncsAndAggsNonRecursive: public UnnestFuncsAndAggs {
	VISITORFRIENDS()

protected:
	virtual Term* traverse(Term* term){
		return term;
	}
};

#endif /* UNNESTFUNCSANDAGGSNR_HPP_ */
