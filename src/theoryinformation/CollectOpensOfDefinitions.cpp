/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "theoryinformation/CollectOpensOfDefinitions.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

const std::set<PFSymbol*>& CollectOpensOfDefinitions::execute(Definition* d) {
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
