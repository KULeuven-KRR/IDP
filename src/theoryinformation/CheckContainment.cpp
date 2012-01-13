/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "theoryinformation/CheckContainment.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

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
