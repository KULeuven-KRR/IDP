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

#include "CollectOpensOfDefinitions.hpp"

#include "IncludeComponents.hpp"

using namespace std;

const std::set<PFSymbol*>& CollectOpensOfDefinitions::execute(const Definition* d) {
	_definition = d;
	_result.clear();
	d->accept(this);
	return _result;
}

void CollectOpensOfDefinitions::visit(const PredForm* pf) {
	if (_definition->defsymbols().find(pf->symbol()) == _definition->defsymbols().cend()) {
		_result.insert(pf->symbol());
	}
	traverse(pf);
}
void CollectOpensOfDefinitions::visit(const FuncTerm* ft) {
	if (_definition->defsymbols().find(ft->function()) == _definition->defsymbols().cend()) {
		_result.insert(ft->function());
	}
	traverse(ft);
}
