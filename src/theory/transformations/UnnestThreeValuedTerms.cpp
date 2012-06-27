/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "UnnestThreeValuedTerms.hpp"
#include "IncludeComponents.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;
using namespace CPSupport;

bool UnnestThreeValuedTerms::shouldMove(Term* t) {
	if (not isAllowedToUnnest()) {
		return false;
	}

	switch (t->type()) {
	case TermType::FUNC: {
		auto ft = dynamic_cast<FuncTerm*>(t);
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
		if (SetUtils::approxTwoValued(at->set(), _structure)) {
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
}

Formula* UnnestThreeValuedTerms::visit(PredForm* predform) {
	auto savedrel = _cpablerelation;
	if (_cpablerelation != TruthValue::False) {
		_cpablerelation = (_cpsupport and eligibleForCP(predform, _vocabulary)) ? TruthValue::True : TruthValue::False;
	}
	if (predform->isGraphedFunction() && _cpablerelation == TruthValue::True) {
		auto args = predform->args();
		args.pop_back();
		auto ft = new FuncTerm(dynamic_cast<Function*>(predform->symbol()), args, TermParseInfo()); // TODO parseinfo
		auto pf = new PredForm(predform->sign(), get(STDPRED::EQ), { ft, predform->args().back() }, predform->pi());
		return pf->accept(this);
	}

	auto result = UnnestTerms::visit(predform);

	_cpablerelation = savedrel;

	return result;
}

Term* UnnestThreeValuedTerms::visit(AggTerm* t) {
	auto savedcp = _cpablefunction;
	auto savedparent = _cpablerelation;
	if (_cpablerelation == TruthValue::True) {
		_cpablerelation = (_cpsupport and eligibleForCP(t, _structure)) ? TruthValue::True : TruthValue::False;
		_cpablefunction = _cpablerelation == TruthValue::True;
	} else {
		_cpablerelation = TruthValue::False;
		_cpablefunction = false;
	}

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
	_cpablerelation = TruthValue::False;

	auto result = UnnestTerms::visit(t);

	_cpablefunction = savedcp;
	_cpablerelation = savedparent;
	return result;
}

Rule* UnnestThreeValuedTerms::visit(Rule* r) {
	// FIXME allowed to unnest non-recursively defined functions!
	auto savedrel = _cpablerelation;
	_cpablerelation = TruthValue::False;
	auto result = UnnestTerms::visit(r);
	_cpablerelation = savedrel;
	return result;
}
