/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef GRAPHFUNCSANDAGGS_HPP_
#define GRAPHFUNCSANDAGGS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"
#include "parseinfo.hpp"
#include "commontypes.hpp"
#include "utils/CPUtils.hpp"
#include "IncludeComponents.hpp"

class AbstractStructure;

/**
 * Graph all direct occurrences of functions in equality and aggregates in any comparison in a predform,
 * unless they can be turned into cp (and it is enabled)
 */
class GraphFuncsAndAggs: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	bool _alsoTwoValued;
	const AbstractStructure* _structure;
	Vocabulary* _vocabulary;
	Context _context;
	bool _cpsupport;
public:
	template<typename T>
	T execute(T t, const AbstractStructure* str = NULL, bool unnestAll = true, bool cpsupport = false, Context c = Context::POSITIVE) {
		_alsoTwoValued = unnestAll;
		_structure = str;
		_vocabulary = (_structure != NULL) ? _structure->vocabulary() : NULL;
		_context = c;
		_cpsupport = cpsupport;
		return t->accept(this);
	}

	static PredForm* makeFuncGraph(SIGN, Term* functerm, Term* valueterm, const FormulaParseInfo&);
	static AggForm* makeAggForm(Term* valueterm, CompType, AggTerm* aggterm, const FormulaParseInfo&);

protected:
	Formula* visit(PredForm* pf);
	Formula* visit(EqChainForm* ef);
};

#endif /* GRAPHFUNCSANDAGGS_HPP_ */
