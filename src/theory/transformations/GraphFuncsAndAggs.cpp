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

#include "GraphFuncsAndAggs.hpp"

#include "theory/TheoryUtils.hpp"
#include "structure/information/IsTwoValued.hpp"
#include "IncludeComponents.hpp"
#include "utils/ListUtils.hpp"

using namespace std;
using namespace CPSupport;
using namespace TermUtils;

/**
 * Given functerm = dom/varterm, construct graph
 */
PredForm* GraphFuncsAndAggs::makeFuncGraph(SIGN sign, Term* functerm, Term* valueterm, const FormulaParseInfo& pi, const Structure* structure) {
	Assert(not isAgg(valueterm) || isTwoValued(valueterm, structure));

	Assert(isFunc(functerm));
	auto ft = dynamic_cast<FuncTerm*>(functerm);
	auto args = ft->subterms();
	args.push_back(valueterm);
	return new PredForm(sign, ft->function(), args, pi);
}

/**
 * Given aggterm ~ dom/varterm, construct aggform
 */
AggForm* GraphFuncsAndAggs::makeAggForm(Term* valueterm, CompType comp, AggTerm* aggterm, const FormulaParseInfo& pi, const Structure* structure) {
	Assert((not isFunc(valueterm) && not isAgg(valueterm)) || isTwoValued(valueterm, structure));
	return new AggForm(SIGN::POS, valueterm, comp, aggterm, pi);
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
	bool usecp = _cpsupport and eligibleForCP(pf, _vocabulary) and eligibleForCP(left, _structure) and eligibleForCP(right, _structure);

	if (usecp) {
		return traverse(pf);
	}

	auto threevalleft = _all3valued || not isTwoValued(left, _structure) || (isFunc(left) && contains(_definedsymbols,dynamic_cast<FuncTerm*>(left)->function()));
	auto threevalright = _all3valued || not isTwoValued(right, _structure) || (isFunc(right) && contains(_definedsymbols,dynamic_cast<FuncTerm*>(right)->function()));;

	if (not threevalleft && not threevalright) {
		return pf;
	}

	if ((wouldGraph(left) and wouldGraph(right)) && threevalleft && threevalright) {
		auto splitformula = FormulaUtils::unnestFuncsAndAggsNonRecursive(pf, _structure);
		return splitformula->accept(this);
	}

	Formula* newformula = NULL;
	if (is(pf->symbol(), STDPRED::EQ)) {
		if (isFunc(left) and threevalleft) {
			newformula = makeFuncGraph(pf->sign(), left, right, pf->pi(), _structure);
			delete (left);
		} else if (isFunc(right) and threevalright) {
			newformula = makeFuncGraph(pf->sign(), right, left, pf->pi(), _structure);
			delete (right);
		}
	}
	if (newformula == NULL and isAgg(left) and threevalleft) {
		newformula = makeAggForm(right, invertComp(FormulaUtils::getComparison(pf)), dynamic_cast<AggTerm*>(left), pf->pi(), _structure);
	} else if (newformula == NULL and isAgg(right) and threevalright) {
		newformula = makeAggForm(left, FormulaUtils::getComparison(pf), dynamic_cast<AggTerm*>(right), pf->pi(), _structure);
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
	for (auto i = ef->subterms().cbegin(); i < ef->subterms().cend(); ++i) {
		if (wouldGraph(*i)) {
			needsSplit = true;
			break;
		}
	}
	if (needsSplit) {
		auto newformula = FormulaUtils::splitComparisonChains((Formula*)ef);
		return newformula->accept(this);
	} else {
		return traverse(ef);
	}
}

bool GraphFuncsAndAggs::wouldGraph(Term* t) const {
	return isAggOrFunc(t);
}
