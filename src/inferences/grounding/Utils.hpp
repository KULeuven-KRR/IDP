/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include "common.hpp"
#include "GroundingContext.hpp"

class Variable;
class DomElemContainer;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;
class TsBody;

typedef std::vector<Variable*> varlist;
typedef std::map<Variable*, const DomElemContainer*> var2dommap;

typedef unsigned int VarId;

typedef std::pair<ElementTuple, Lit> Tuple2Atom;
typedef std::pair<Lit, TsBody*> tspair;

// TODO dynamically initialized global: no init order, so cannot safely use in other globals, might be unsafe?
const Lit _true(getMaxElem<int>());
const Lit _false(-getMaxElem<int>()); //NOT getMinElem<int> because -_true should be _false

#endif /* UTILS_HPP_ */
