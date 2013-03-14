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

#include "HasRecursionOverNegation.hpp"

#include "IncludeComponents.hpp"

using namespace std;

bool HasRecursionOverNegation::execute(Definition* d) {
	_definition = d;
	_result = false;
	for(auto it = d->rules().cbegin(); it != d->rules().cend(); it++) {
		_currentlyNegated = false;
		(*it)->accept(this);
		if(_result) {
			return true;
		}
	}
	return _result;
}

void HasRecursionOverNegation::visit(const EqChainForm* f) {
	if(f->sign() == SIGN::NEG) {
		_currentlyNegated = !_currentlyNegated;
	}
	traverse(f);
}

void HasRecursionOverNegation::visit(const EquivForm* f) {
	if(f->sign() == SIGN::NEG) {
		_currentlyNegated = !_currentlyNegated;
	}
	traverse(f);
}

void HasRecursionOverNegation::visit(const BoolForm* f) {
	if(f->sign() == SIGN::NEG) {
		_currentlyNegated = !_currentlyNegated;
	}
	traverse(f);
}

void HasRecursionOverNegation::visit(const QuantForm* f) {
	if(f->sign() == SIGN::NEG) {
		_currentlyNegated = !_currentlyNegated;
	}
	traverse(f);
}

void HasRecursionOverNegation::visit(const AggForm* f) {
	if(f->sign() == SIGN::NEG) {
		_currentlyNegated = !_currentlyNegated;
	}
	traverse(f);
}

void HasRecursionOverNegation::visit(const PredForm* pf) {
	if(pf->sign() == SIGN::NEG) {
		_currentlyNegated = !_currentlyNegated;
	}

	if (_definition->defsymbols().find(pf->symbol()) != _definition->defsymbols().cend()
			&& _currentlyNegated ) {
		_result = true;
	} else {
		traverse(pf);
	}
}
void HasRecursionOverNegation::visit(const FuncTerm* ft) {
	if (_definition->defsymbols().find(ft->function()) != _definition->defsymbols().cend()
			&& _currentlyNegated ) {
		_result = true;
	} else {
		traverse(ft);
	}
}