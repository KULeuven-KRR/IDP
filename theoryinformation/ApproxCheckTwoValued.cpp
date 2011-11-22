/************************************
  	ApproxCheckTwoValued.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theoryinformation/ApproxCheckTwoValued.hpp"

#include "theory.hpp"
#include "structure.hpp"
#include "term.hpp"

void ApproxCheckTwoValued::visit(const PredForm* pf) {
	PredInter* inter = _structure->inter(pf->symbol());
	if(inter->approxTwoValued()) {
		for(auto it = pf->subterms().cbegin(); it != pf->subterms().cend(); ++it) {
			(*it)->accept(this);
			if(not _returnvalue) { return; }
		}
	}
	else { _returnvalue = false; }
}

void ApproxCheckTwoValued::visit(const FuncTerm* ft) {
	FuncInter* inter = _structure->inter(ft->function());
	if(inter->approxTwoValued()) {
		for(auto it = ft->subterms().cbegin(); it != ft->subterms().cend(); ++it) {
			(*it)->accept(this);
			if(not _returnvalue) { return; }
		}
	}
	else { _returnvalue = false; }
}

void ApproxCheckTwoValued::visit(const SetExpr* se) {
	for(auto it = se->subformulas().cbegin(); it != se->subformulas().cend(); ++it) {
		(*it)->accept(this);
		if(not _returnvalue) { return; }
	}
	for(auto it = se->subterms().cbegin(); it != se->subterms().cend(); ++it) {
		(*it)->accept(this);
		if(not _returnvalue) { return; }
	}
	_returnvalue = true;
}
