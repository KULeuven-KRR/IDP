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

#pragma once

#include "visitors/TheoryMutatingVisitor.hpp"

class PullQuantifications: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}

protected:
	// TODO Extend

	Rule* visit(Rule* rule) {
		auto qf = dynamic_cast<QuantForm*>(rule->body());
		if (qf != NULL && not qf->isUnivWithSign()) {
			for (auto var : qf->quantVars()) {
				rule->addvar(var);
			}
			rule->body(qf->subformula());
		}
		return rule;
	}
};
