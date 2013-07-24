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

#include "IncludeComponents.hpp"
#include "RemoveQuantificationsOverSort.hpp"

Formula* RemoveQuantificationsOverSort::visit(QuantForm* qf) {
	auto vars = qf->quantVars();
	varset newvars = { };
	for (auto var : vars) {
		if (var->sort() != _sortToReplace) {
			newvars.insert(var);
		}
	}
	if (newvars.empty()) {
		return qf->subformula()->accept(this);
	} else {
		qf->quantVars(newvars);
		return traverse(qf);
	}
}
QuantSetExpr* RemoveQuantificationsOverSort::visit(QuantSetExpr* qse) {
	auto vars = qse->quantVars();
	varset newvars = { };
	for (auto var : vars) {
		if (var->sort() != _sortToReplace) {
			newvars.insert(var);
		}
	}
	qse->quantVars(newvars);
	return traverse(qse);
}
Rule* RemoveQuantificationsOverSort::visit(Rule* r) {
	auto vars = r->quantVars();
	varset newvars = { };
	for (auto var : vars) {
		if (var->sort() != _sortToReplace) {
			newvars.insert(var);
		}
	}
	r->quantVars(newvars);
	return TheoryMutatingVisitor::visit(r);
}
