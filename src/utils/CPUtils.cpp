/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "CPUtils.hpp"

#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

using namespace std;

namespace CPSupport {

bool eligibleForCP(const PredForm* pf, const Vocabulary* voc) {
	return VocabularyUtils::isIntComparisonPredicate(pf->symbol(),voc);
}

bool eligibleForCP(const FuncTerm* ft, const Vocabulary* voc) {
	auto function = ft->function();
	if(FuncUtils::isIntFunc(function,voc)){
		return true;
	}
	bool passtocp = false;
	// Check whether the (user-defined) function's outsort is over integers
	if (function->overloaded()) {
		auto nonbuiltins = function->nonbuiltins();
		auto allint = true;
		for (auto nbfit = nonbuiltins.cbegin(); allint && nbfit != nonbuiltins.cend(); ++nbfit) {
			if(not FuncUtils::isIntFunc(*nbfit, voc)){
				allint = false;
			}
		}
		passtocp = allint;
	} else if (not function->builtin()) {
		passtocp = FuncUtils::isIntFunc(function, voc);
	}
	return passtocp;
}

bool eligibleForCP(const AggFunction& f) {
	return (f == SUM);
}

bool eligibleForCP(const AggTerm* at, AbstractStructure* str) {
	if (eligibleForCP(at->function()) && str != NULL) {
		for (auto it = at->set()->subformulas().cbegin(); it != at->set()->subformulas().cend(); ++it) {
			if (not FormulaUtils::approxTwoValued(*it,str)) {
				return false;
			}
		}
		for (auto it = at->set()->subterms().cbegin(); it != at->set()->subterms().cend(); ++it) {
			if (not eligibleForCP(*it,str)) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool eligibleForCP(const Term* t, AbstractStructure* str) {
	Vocabulary* voc = (str != NULL) ? str->vocabulary() : NULL;
	switch (t->type()) {
	case TT_FUNC: {
		auto ft = dynamic_cast<const FuncTerm*>(t);
		return eligibleForCP(ft,voc);
	}
	case TT_AGG: {
		auto at = dynamic_cast<const AggTerm*>(t);
		return eligibleForCP(at,str);
	}
	case TT_VAR:
	case TT_DOM:
		SortUtils::isSubsort(t->sort(), get(STDSORT::INTSORT),voc);
		return true;
	}
}

} /* namespace CPUtils */


