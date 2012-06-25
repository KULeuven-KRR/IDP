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
		if (_cpsupport and nestingIsAllowed() and eligibleForCP(ft, _vocabulary)) {
			return false;
		}
		return true;
	}
	case TermType::AGG: {
		auto at = dynamic_cast<AggTerm*>(t);
		if (SetUtils::approxTwoValued(at->set(), _structure)) {
			return false;
		}
		if (_cpsupport and nestingIsAllowed() and eligibleForCP(at, _structure)) {
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
	auto saveAllowedToLeave = nestingIsAllowed();
	setNestingIsAllowed(_cpsupport and eligibleForCP(predform, _vocabulary));

	auto newf = unnest(predform);

	setNestingIsAllowed(saveAllowedToLeave);

	return doRewrite(newf);
}

Term* UnnestThreeValuedTerms::visit(AggTerm* t) {
	auto savemovecontext = isAllowedToUnnest();

	auto saveAllowedToLeave = nestingIsAllowed();
	setNestingIsAllowed(_cpsupport and eligibleForCP(t, _structure));

	auto result = traverse(t);

	setNestingIsAllowed(saveAllowedToLeave);

	setAllowedToUnnest(savemovecontext);

	return doMove(result);
}

Rule* UnnestThreeValuedTerms::visit(Rule* rule) {
	auto saveAllowedToLeave = nestingIsAllowed();
	setNestingIsAllowed(_cpsupport and eligibleForCP(rule->head(), _vocabulary));

	auto newrule = UnnestTerms::visit(rule);

	setNestingIsAllowed(saveAllowedToLeave);

	return newrule;
}
