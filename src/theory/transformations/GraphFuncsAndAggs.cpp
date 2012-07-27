/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "GraphFuncsAndAggs.hpp"

#include "theory/TheoryUtils.hpp"
#include "IncludeComponents.hpp"

using namespace std;
using namespace CPSupport;

CompType getComparison(const PredForm* pf) {
	auto sign = pf->sign();
	auto symbol = pf->symbol();
	Assert(VocabularyUtils::isComparisonPredicate(symbol));
	if (is(symbol, STDPRED::EQ)) {
		return isPos(sign) ? CompType::EQ : CompType::NEQ;
	} else if (is(symbol, STDPRED::LT)) {
		return isPos(sign) ? CompType::LT : CompType::GEQ;
	} else {
		Assert(is(symbol, STDPRED::GT));
		return isPos(sign) ? CompType::GT : CompType::LEQ;
	}
}

bool isAgg(Term* t) {
	return t->type() == TermType::AGG;
}
bool isFunc(Term* t) {
	return t->type() == TermType::FUNC;
}

bool isAggOrFunc(Term* t) {
	return isAgg(t) || isFunc(t);
}

/**
 * Given functerm = dom/varterm, construct graph
 */
PredForm* GraphFuncsAndAggs::makeFuncGraph(SIGN sign, Term* functerm, Term* valueterm, const FormulaParseInfo& pi) const {
	Assert(not isAgg(valueterm));
	Assert(not isFunc(valueterm));

	Assert(isFunc(functerm));
	auto ft = dynamic_cast<FuncTerm*>(functerm);

	auto args = ft->subterms();
	args.push_back(valueterm);

	return new PredForm(sign, ft->function(), args, pi);
}

/**
 * Given aggterm ~ dom/varterm, construct aggform
 */
AggForm* GraphFuncsAndAggs::makeAggForm(Term* valueterm, CompType comp, Term* aggterm, const FormulaParseInfo& pi) const {
	Assert(not isFunc(valueterm));
	Assert(not isAgg(valueterm));

	Assert(isAgg(aggterm));
	auto at = dynamic_cast<AggTerm*>(aggterm);

	return new AggForm(SIGN::POS, valueterm, comp, at, pi);
}

/**
 * Turn any func/agg comparison into its graphed version
 */
Formula* GraphFuncsAndAggs::visit(PredForm* pf) {
	if (not VocabularyUtils::isComparisonPredicate(pf->symbol())) {
		return traverse(pf);
	}

	auto left = pf->subterms()[0];
	auto right = pf->subterms()[1];
	bool usecp = _cpsupport
			and VocabularyUtils::isIntComparisonPredicate(pf->symbol(), _vocabulary)
			and eligibleForCP(left, _structure)
			and eligibleForCP(right, _structure);

	if (usecp) {
		return traverse(pf);
	}

	if ((isAggOrFunc(left) and isAggOrFunc(right))) {
		//TODO more fine-grained: don't split when at least one side can be calculated.
		auto splitformula = FormulaUtils::unnestFuncsAndAggsNonRecursive(pf, _structure, _context);
		return splitformula->accept(this);
	}

	Formula* newformula = NULL;
	if (is(pf->symbol(), STDPRED::EQ)) {
		if (isFunc(left)) {
			newformula = makeFuncGraph(pf->sign(), left, right, pf->pi());
			delete (left);
		} else if (isFunc(right)) {
			newformula = makeFuncGraph(pf->sign(), right, left, pf->pi());
			delete (right);
		}
	}
	if (newformula == NULL and isAgg(left)) {
		newformula = makeAggForm(right, invertComp(getComparison(pf)), left, pf->pi());
	} else if (newformula == NULL and isAgg(right)) {
		newformula = makeAggForm(left, getComparison(pf), right, pf->pi());
	}
	if (newformula != NULL) {
		delete (pf);
		return traverse(newformula);
	} else {
		return traverse(pf);
	}
}

Formula* GraphFuncsAndAggs::visit(EqChainForm* ef) {
	bool needsSplit = false;
	for(auto i=ef->subterms().cbegin(); i<ef->subterms().cend(); ++i) {
		if (isAggOrFunc(*i)) {
			needsSplit = true;
			break;
		}
	}
	if (needsSplit) {
		auto newformula = FormulaUtils::splitComparisonChains(ef);
		return newformula->accept(this);
	} else {
		return traverse(ef);
	}
}
