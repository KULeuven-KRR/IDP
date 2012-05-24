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

bool UnnestThreeValuedTerms::shouldMove(Term* t) {
	if (getAllowedToUnnest()) {
		switch (t->type()) {
		case TT_FUNC: {
			auto ft = dynamic_cast<FuncTerm*>(t);
			if(_structure->inter(ft->function())->approxTwoValued()){
				return false;
			}
			if(_cpsupport and getAllowedToLeave() and CPSupport::eligibleForCP(ft, _vocabulary)){
				return false;
			}
			return true;
		}
		case TT_AGG: {
			auto at = dynamic_cast<AggTerm*>(t);
			if(SetUtils::approxTwoValued(at->set(), _structure)){
				return false;
			}
			if(_cpsupport and getAllowedToLeave() and CPSupport::eligibleForCP(at, _structure)){
				return false;
			}
			return true;
		}
		case TT_VAR:
		case TT_DOM:
			break;
		}
	}
	return false;
}

Formula* UnnestThreeValuedTerms::visit(PredForm* predform) {
	auto saveAllowedToLeave = getAllowedToLeave();
	setAllowedToLeave(_cpsupport and CPSupport::eligibleForCP(predform, _vocabulary));
	auto newf = specialTraverse(predform);
	setAllowedToLeave(saveAllowedToLeave);
	return doRewrite(newf);
}

Term* UnnestThreeValuedTerms::visit(AggTerm* t) {
	bool savemovecontext = getAllowedToUnnest();
	bool saveAllowedToLeave = getAllowedToLeave();
	setAllowedToLeave(_cpsupport and CPSupport::eligibleForCP(t, _structure));
	auto result = traverse(t);
	setAllowedToUnnest(savemovecontext);
	setAllowedToLeave(saveAllowedToLeave);
	return doMove(result);
}

Rule* UnnestThreeValuedTerms::visit(Rule* rule) {
	auto saveAllowedToLeave = getAllowedToLeave();
	setAllowedToLeave(_cpsupport and CPSupport::eligibleForCP(rule->head(), _vocabulary));
	auto newrule = UnnestTerms::visit(rule);
	setAllowedToLeave(saveAllowedToLeave);
	return newrule;
}

