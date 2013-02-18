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

#include "ApproxCheckTwoValued.hpp"

#include "IncludeComponents.hpp"

void ApproxCheckTwoValued::visit(const PredForm* pf) {
	PredInter* inter = _structure->inter(pf->symbol());
	if (inter->approxTwoValued()) {
		for (auto it = pf->subterms().cbegin(); it != pf->subterms().cend(); ++it) {
			(*it)->accept(this);
			if (not _returnvalue) {
				return;
			}
		}
	} else {
		_returnvalue = false;
	}
}

void ApproxCheckTwoValued::visit(const FuncTerm* ft) {
	FuncInter* inter = _structure->inter(ft->function());
	if (inter->approxTwoValued()) {
		for (auto it = ft->subterms().cbegin(); it != ft->subterms().cend(); ++it) {
			(*it)->accept(this);
			if (not _returnvalue) {
				return;
			}
		}
	} else {
		_returnvalue = false;
	}
}

void ApproxCheckTwoValued::visit(const QuantSetExpr* set) {
	set->getCondition()->accept(this);
	if (not _returnvalue) {
		return;
	}
	set->getTerm()->accept(this);
	if (not _returnvalue) {
		return;
	}
	_returnvalue = true;
}
