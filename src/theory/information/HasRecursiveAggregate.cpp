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

#include "HasRecursiveAggregate.hpp"

#include "IncludeComponents.hpp"

using namespace std;

bool HasRecursiveAggregate::execute(Definition* d) {
	_definition = d;
	_result = false;
	_inAggForm = false;
	for(auto it = d->rules().cbegin(); it != d->rules().cend(); it++) {
		(*it)->body()->accept(this);
		if(_result) {
			return true;
		}
	}
	return _result;
}

void HasRecursiveAggregate::visit(const AggTerm* at) {
	_inAggForm = true;
	traverse(at);
	_inAggForm = false;
}

void HasRecursiveAggregate::visit(const PredForm* pf) {
	if (_inAggForm and _definition->defsymbols().find(pf->symbol()) != _definition->defsymbols().cend()) {
		_result = true;
	} else {
		traverse(pf);
	}
}
void HasRecursiveAggregate::visit(const FuncTerm* ft) {
	if (_inAggForm and _definition->defsymbols().find(ft->function()) != _definition->defsymbols().cend()) {
		_result = true;
	} else {
		traverse(ft);
	}
}
