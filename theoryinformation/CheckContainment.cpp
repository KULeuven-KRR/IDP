/************************************
  	CheckContainment.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theoryinformation/CheckContainment.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

bool CheckContainment::containsSymbol(const Formula* f) {
	_result = false;
	f->accept(this);
	return _result;
}

void CheckContainment::visit(const PredForm* pf) {
	if (pf->symbol() == _symbol) {
		_result = true;
		return;
	} else
		traverse(pf);
}

void CheckContainment::visit(const FuncTerm* ft) {
	if (ft->function() == _symbol) {
		_result = true;
		return;
	} else
		traverse(ft);
}
