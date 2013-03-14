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

#include "visitors/TheoryVisitor.hpp"

class Definition;

class HasRecursionOverNegation: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	Definition* _definition;
	bool _currentlyNegated;
	bool _result;

public:
	bool execute(Definition* d);

protected:
	void visit(const PredForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const EqChainForm*);
	void visit(const AggForm*);
	void visit(const EquivForm*);
	void visit(const FuncTerm* ft);
};
