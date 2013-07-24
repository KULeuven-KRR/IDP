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

#include "GetQuantifiedVariables.hpp"

#include "IncludeComponents.hpp"

using namespace std;

void CollectQuantifiedVariables::visit(const QuantForm* qf) {
	for (auto var : qf->quantVars()) {
		if (qf->isUniv()) {
			_quantvars[var] = QuantType::UNIV;
		} else {
			_quantvars[var] = QuantType::EXISTS;
		}
	}
	if (_recursive) {
		traverse(qf);
	}

}
void CollectQuantifiedVariables::visit(const QuantSetExpr* qse) {
	for (auto var : qse->quantVars()) {
		_quantvars[var] = QuantType::SET;
	}
	if (_recursive) {
		traverse(qse);
	}
}
void CollectQuantifiedVariables::visit(const Rule* r) {
	for (auto var : r->quantVars()) {
		_quantvars[var] = QuantType::UNIV;
	}
	if (_recursive) {
		r->head()->accept(this);
		r->body()->accept(this);
	}
}
