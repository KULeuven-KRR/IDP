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

#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "IncludeComponents.hpp"

using namespace std;

Sort* FOBDDVariable::sort() const {
	return _variable->sort();
}

Sort* FOBDDFuncTerm::sort() const {
	return _function->outsort();
}

bool CompareBDDVars::operator()(const FOBDDVariable* lhs, const FOBDDVariable* rhs) const {
	if(lhs==NULL){
		return true;
	}
	if(rhs==NULL){
		return false;
	}
	VarCompare v;
	return v.operator ()(lhs->variable(), rhs->variable());
}
