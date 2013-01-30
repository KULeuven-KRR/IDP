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

#include "CheckContainment.hpp"

#include "IncludeComponents.hpp"

using namespace std;

bool CheckContainment::execute(const PFSymbol* s, const Formula* f) {
	_symbol = s;
	_result = false;
	f->accept(this);
	return _result;
}

void CheckContainment::visit(const PredForm* pf) {
	if (pf->symbol() == _symbol) {
		_result = true;
		return;
	} else {
		traverse(pf);
	}
}

void CheckContainment::visit(const FuncTerm* ft) {
	if (ft->function() == _symbol) {
		_result = true;
		return;
	} else {
		traverse(ft);
	}
}
