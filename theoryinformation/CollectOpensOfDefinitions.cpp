/************************************
  	CollectOpensOfDefinitions.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theoryinformation/CollectOpensOfDefinitions.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

const std::set<PFSymbol*>& CollectOpensOfDefinitions::run(Definition* d) {
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
