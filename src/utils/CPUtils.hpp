/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef IDP_CPUTILS_HPP_
#define IDP_CPUTILS_HPP_

#include "common.hpp"
#include <set>

class PFSymbol;
class Function;
class PredForm;
class FuncTerm;
class AggTerm;
class Term;
class Vocabulary;
class AbstractStructure;

namespace CPSupport {

//TODO template function eligibleForCPSupport(Object, ...)

// Determine what should be passed to CP solver
std::set<const Function*> findCPSymbols(const Vocabulary* vocabulary);

bool eligibleForCP(const PredForm*, const Vocabulary*);
bool eligibleForCP(const FuncTerm*, const Vocabulary*);
bool eligibleForCP(const AggTerm*, AbstractStructure*);
bool eligibleForCP(const AggFunction&);

bool eligibleForCP(const Term*, AbstractStructure*);

}

#endif /* IDP_CPUTILS_HPP_ */
