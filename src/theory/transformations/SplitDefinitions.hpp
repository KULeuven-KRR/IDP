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


#include <set>
#include <map>
#include "vocabulary/vocabulary.hpp"

/**
 * Split definitions as much as possible. We do this so we can detect more
 * definitions that can be calculated in advance. For example, the definition
 *   { p <- true .
 *     q <- r. }
 * cannot be calculated in advance if r is not two-valued in the input structure.
 * However, the rule p <- true can still be calculated independently, so if we split
 * the definition to the following:
 *   { p <- true. }
 *   { q <- r.    }
 * The value of p can be calculated in advance.
 *
 * This transformation splits definitions as much as possible, without breaking dependency
 * loops. For example, the definition
 *   { q <- p.
 *     p <- q. }
 *  Cannot be split into two separate definitions because it would change the meaning
 *  under the Well-Founded Semantics.
 */

class SplitDefinitions: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::set<PFSymbol*> _curr_opens;
	std::set<Definition*> _definitions_to_add;
	std::map<PFSymbol*, std::set<Rule*>> _defsymbol_to_rule;
	Definition* merge_defs(Definition*, Definition*);

public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}
protected:
	Theory* visit(Theory*);
	Definition* visit(Definition*);
};
