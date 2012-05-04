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

class GraphFuncsAndAggs: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	AbstractStructure* _structure;
	Vocabulary* _vocabulary;
	Context _context;
	bool _cpsupport;
public:
	template<typename T>
	T execute(T t, AbstractStructure* str = NULL, Context c = Context::POSITIVE) {
		_structure = str;
		_vocabulary = (_structure != NULL) ? _structure->vocabulary() : NULL;
		_context = c;
		_cpsupport = getOption(BoolType::CPSUPPORT);
		return t->accept(this);
	}
protected:
	Formula* visit(PredForm* pf);
	Formula* visit(EqChainForm* ef);
private:
	CompType getCompType(const PredForm* pf) const;
	PredForm* makeFuncGraph(SIGN, Term* functerm, Term* valueterm, const FormulaParseInfo&) const;
	AggForm* makeAggForm(Term* valueterm, CompType, Term* aggterm, const FormulaParseInfo&) const;
};

#endif /* GRAPHFUNCSANDAGGS_HPP_ */
