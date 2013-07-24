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

#include "CollectSymbols.hpp"

#include "IncludeComponents.hpp"

using namespace std;

void CollectSymbols::visit(const PredForm* pf) {
	_result.insert(pf->symbol());
	traverse(pf);
}
void CollectSymbols::visit(const FuncTerm* ft) {
	_result.insert(ft->function());
	traverse(ft);
}
