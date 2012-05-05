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
	//std::cerr <<"should i move"<<toString(t)<<endl;
	if (getAllowedToUnnest()) {
	//	std::cerr <<"i can"<<endl;

		switch (t->type()) {
		case TT_FUNC: {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(t);
			Function* func = ft->function();
			FuncInter* finter = _structure->inter(func);
		//	std::cerr <<(not finter->approxTwoValued() and not (_cpsupport and getAllowedToLeave() and CPSupport::eligibleForCP(ft, _vocabulary)))<<endl;
			return not finter->approxTwoValued() and not (_cpsupport and getAllowedToLeave() and CPSupport::eligibleForCP(ft, _vocabulary));
		}
		case TT_AGG: {
			AggTerm* at = dynamic_cast<AggTerm*>(t);
			return not SetUtils::approxTwoValued(at->set(), _structure)
					and not (_cpsupport and getAllowedToLeave() and CPSupport::eligibleForCP(at, _structure));
		}
		case TT_VAR:
		case TT_DOM:
			break;
		}
	}
	return false;
}

Formula* UnnestThreeValuedTerms::visit(PredForm* predform) {
	bool saveAllowedToLeave = getAllowedToLeave();
	setAllowedToLeave(_cpsupport and CPSupport::eligibleForCP(predform, _vocabulary));
	auto newf = specialTraverse(predform);
	setAllowedToLeave(saveAllowedToLeave);
	return doRewrite(newf);
}

Rule* UnnestThreeValuedTerms::visit(Rule* rule) {
	bool saveAllowedToLeave = getAllowedToLeave();
	setAllowedToLeave(_cpsupport and CPSupport::eligibleForCP(rule->head(), _vocabulary));
	auto newrule = UnnestTerms::visit(rule);
	setAllowedToLeave(saveAllowedToLeave);
	return newrule;
}

