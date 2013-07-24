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

/**
 * Removes all quantifications over the given sort.
 * NOTE: this might turn sentences in formulas with free variables.
 * Example:
 * !t[sort]: P(t) => ? x[sort2] y[sort]: Q(t,x,y)
 * would become
 * P(t) => ? x[sort2]: Q(t,x,y)
 */
class RemoveQuantificationsOverSort: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	const Sort* _sortToReplace;
public:
	template<typename T>
	T execute(T t, const Sort* sort) {
		_sortToReplace = sort;
		return t->accept(this);
	}


protected:
	Formula* visit(QuantForm*);
	QuantSetExpr* visit(QuantSetExpr*);
	Rule* visit(Rule*);
};
