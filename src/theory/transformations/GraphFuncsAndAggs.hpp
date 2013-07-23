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
#include "parseinfo.hpp"
#include "commontypes.hpp"
#include "utils/CPUtils.hpp"
#include "IncludeComponents.hpp"

class Structure;

/**
 * Graph all direct occurrences of functions in equality and aggregates in any comparison in a predform,
 * unless they can be turned into cp (and it is enabled)
 */
class GraphFuncsAndAggs: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	bool _all3valued; // True if during unnesting, should consider all symbols as three-valued
	const Structure* _structure;
	Vocabulary* _vocabulary;
	Context _context;
	bool _cpsupport;
public:
	template<typename T>
	T execute(T t, const Structure* str = NULL, bool unnestAll = true, bool cpsupport = false, Context c = Context::POSITIVE) {
		_all3valued = unnestAll;
		_structure = str;
		_vocabulary = (_structure != NULL) ? _structure->vocabulary() : NULL;
		_context = c;
		_cpsupport = cpsupport;
		return t->accept(this);
	}

	static PredForm* makeFuncGraph(SIGN, Term* functerm, Term* valueterm, const FormulaParseInfo&, const Structure* structure);
	static AggForm* makeAggForm(Term* valueterm, CompType, AggTerm* aggterm, const FormulaParseInfo&, const Structure* structure);

protected:
	Formula* visit(PredForm* pf);
	Formula* visit(EqChainForm* ef);
};
