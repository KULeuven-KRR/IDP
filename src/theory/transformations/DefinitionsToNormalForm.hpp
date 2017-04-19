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

#include <stack>
#include "visitors/TheoryMutatingVisitor.hpp"

class DefinitionsToNormalForm: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::stack<Rule*> _rulesToProcess;
	bool _isNested;

	Formula* processFormula(Formula*);

public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}
protected:
	Formula* traverse(Formula*);
	Theory* visit(Theory*);
	Definition* visit(Definition*);
	Rule* visit(Rule*);

	Formula* visit(AggForm*);
	Formula* visit(BoolForm*);
	Formula* visit(EqChainForm*);
	Formula* visit(QuantForm*);
};
