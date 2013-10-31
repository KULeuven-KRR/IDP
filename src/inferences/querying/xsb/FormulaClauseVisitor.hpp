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

#include "FormulaClause.hpp"
#include "visitors/TheoryVisitor.hpp"

class FormulaClauseVisitor: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
public:
	virtual void visit(PrologTerm*) = 0;
	virtual void visit(ExistsClause*) = 0;
	virtual void visit(ForallClause*) = 0;
	virtual void visit(AndClause*) = 0;
	virtual void visit(OrClause*) = 0;
	virtual void visit(AggregateClause*) = 0;
	virtual void visit(AggregateTerm*) = 0;
	virtual void visit(EnumSetExpression*) = 0;
	virtual void visit(QuantSetExpression*) = 0;
};
