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

#include "UnnestThreeValuedTerms.hpp"
#include "IncludeComponents.hpp"

#include "utils/ListUtils.hpp"
#include "theory/TheoryUtils.hpp"
#include "GraphFuncsAndAggs.hpp"

using namespace std;
using namespace CPSupport;

bool UnnestThreeValuedTerms::wouldMove(Term* t) {
	switch (t->type()) {
	case TermType::FUNC: {
		auto ft = dynamic_cast<FuncTerm*>(t);
		if(contains(_definedsymbols, ft->function())){
			return true;
		}
		if (_structure->inter(ft->function())->approxTwoValued()) {
			return false;
		}
		if (_cpsupport and _cpablefunction and eligibleForCP(ft, _vocabulary)) {
			return false;
		}
		return true;
	}
	case TermType::AGG: {
		auto at = dynamic_cast<AggTerm*>(t);
		bool alltwovalued = true;
		for(auto qset: at->set()->getSets()){
			if(not SetUtils::approxTwoValued(qset, _structure) || not getOption(REDUCEDGROUNDING)){ // TODO: should approxTwoValued only go to atom level here? Or will the remainder already have been moved?
				alltwovalued = false;
				break;
			}
		}
		if(alltwovalued){
			return false;
		}
		if (_cpsupport and _cpablefunction and eligibleForCP(at, _structure)) {
			return false;
		}
		return true;
	}
	case TermType::VAR:
	case TermType::DOM:
		return false;
	}
	Assert(false);
	return false;
}

Formula* UnnestThreeValuedTerms::visit(PredForm* predform) {
	// FIXME check whether it correctly handles recursively defined predicates
	auto savedrel = _cpablerelation;
	if (_cpablerelation != TruthValue::False) {
		if(getOption(CPGROUNDATOMS)){
			_cpablerelation = _cpsupport ? TruthValue::True : TruthValue::False;
		}else{
			_cpablerelation = (_cpsupport and eligibleForCP(predform, _vocabulary) and VocabularyUtils::isIntComparisonPredicate(predform->symbol(), _vocabulary)) ? TruthValue::True : TruthValue::False;
		}
	}

	Formula* result = NULL;

	// Optimization to prevent aggregate duplication (TODO might be done for functions too?)
	if (_cpsupport and not CPSupport::eligibleForCP(predform, _vocabulary) && not is(predform->symbol(), STDPRED::EQ)) {
		std::vector<Formula*> aggforms;
		for (size_t i = 0; i < predform->args().size(); ++i) {
			auto origterm = predform->args().front();
			if (origterm->freeVars().size() != 0 or origterm->type() != TermType::AGG) { // TODO handle free vars
				continue;
			}
			if (not eligibleForCP(dynamic_cast<AggTerm*>(origterm)->function())) { // FIXME this seems necessary, because aggform unnesting is incorrect
				continue;
			}
			auto sort = origterm->sort();
			if (_structure != NULL and SortUtils::isSubsort(sort, get(STDSORT::INTSORT), _vocabulary)) {
				sort = TermUtils::deriveSmallerSort(origterm, _structure);
			}
			auto constant = new Function( { }, sort, origterm->pi());
			_vocabulary->add(constant);
			auto newterm = new FuncTerm(constant, { }, origterm->pi());
			predform->arg(i, newterm->clone());
			aggforms.push_back(new AggForm(SIGN::POS, newterm, CompType::EQ, dynamic_cast<AggTerm*>(origterm), FormulaParseInfo()));
		}
		if (aggforms.size() > 0) {
			aggforms.push_back(predform);
			auto boolform = new BoolForm(SIGN::POS, true, aggforms, predform->pi());
			result = boolform->accept(this);
		}
	}

    if(result==NULL){
		 result = UnnestTerms::visit(predform);
     }
	_cpablerelation = savedrel;

	return result;
}

Formula* UnnestThreeValuedTerms::visit(AggForm* af) {
	auto savedparent = _cpablerelation;
	if (_cpablerelation != TruthValue::False) {
		_cpablerelation = (_cpsupport and eligibleForCP(af->getAggTerm(), _structure)) ? TruthValue::True : TruthValue::False;
	}

	auto result = UnnestTerms::visit(af);

	_cpablerelation = savedparent;
	return result;
}

Term* UnnestThreeValuedTerms::visit(AggTerm* t) {
	auto savedcp = _cpablefunction;
	auto savedparent = _cpablerelation;
	if (_cpablerelation == TruthValue::True) {
		_cpablefunction = _cpsupport and eligibleForCP(t, _structure);
	} else {
		_cpablefunction = false;
	}
	_cpablerelation = (_cpsupport and eligibleForCP(t, _structure)) ? TruthValue::True : TruthValue::False;

	auto result = UnnestTerms::visit(t);

	_cpablefunction = savedcp;
	_cpablerelation = savedparent;
	return result;
}

Term* UnnestThreeValuedTerms::visit(FuncTerm* t) {
	auto savedcp = _cpablefunction;
	auto savedparent = _cpablerelation;
	if (_cpablerelation == TruthValue::True) {
		_cpablefunction = _cpsupport and eligibleForCP(t, _structure);
	} else {
		_cpablefunction = false;
	}
	if (not FuncUtils::isIntSum(t->function(), _structure->vocabulary())
			and not FuncUtils::isIntProduct(t->function(), _structure->vocabulary())
			and not is(t->function(), STDFUNC::UNARYMINUS)) {
		//Note: Leave cpable flag as is when the current functerm is a sum or a term with a factor!
		// They get a special treatment for CP.
		_cpablerelation = TruthValue::False;
	}

	auto result = UnnestTerms::visit(t);

	_cpablefunction = savedcp;
	_cpablerelation = savedparent;
	return result;
}

Rule* UnnestThreeValuedTerms::visit(Rule* r) {
	// FIXME allowed to unnest non-recursively defined functions!
//	auto savedrel = _cpablerelation;
//	_cpablerelation = TruthValue::False;
	auto result = UnnestTerms::visit(r);
//	_cpablerelation = savedrel;
	return result;
}
