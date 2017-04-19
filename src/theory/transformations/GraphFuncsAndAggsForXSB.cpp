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

#include "GraphFuncsAndAggsForXSB.hpp"

#include "theory/TheoryUtils.hpp"
#include "structure/information/IsTwoValued.hpp"
#include "IncludeComponents.hpp"
#include "utils/ListUtils.hpp"

using namespace std;
using namespace TermUtils;

bool GraphFuncsAndAggsForXSB::wouldGraph(Term* t) const {
	return isAggOrNonConstructorFunction(t);
}
