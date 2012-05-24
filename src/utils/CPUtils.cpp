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

bool nonOverloadedNonBuiltinEligibleForCP(Function* f, const Vocabulary* v){
	if(f->partial()){ // TODO at the moment, partial terms are never eligible for CP
		return false;
	}
	if(not FuncUtils::isIntFunc(f, v)){
		return false;
	}
	return true;
}

bool eligibleForCP(const FuncTerm* ft, const Vocabulary* voc) {
	auto function = ft->function();
	bool passtocp = false;
	// Check whether the (user-defined) function's outsort is over integers
	if (function->overloaded()) {
		auto nonbuiltins = function->nonbuiltins();
		for (auto nbfit = nonbuiltins.cbegin(); nbfit != nonbuiltins.cend(); ++nbfit) {
			if(not nonOverloadedNonBuiltinEligibleForCP(*nbfit, voc)){
				return false;
			}
		}
		passtocp = true;
	} else if (not function->builtin()) {
		passtocp = nonOverloadedNonBuiltinEligibleForCP(function, voc);
	} else{
		Assert(function->builtin() and not function->overloaded());
		passtocp = FuncUtils::isIntFunc(function, voc);
	}
	return passtocp;
}

bool eligibleForCP(const AggFunction& f) {
	return (f == AggFunction::SUM);
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
	auto voc = (str != NULL) ? str->vocabulary() : NULL;
	switch (t->type()) {
	case TermType::TT_FUNC: {
		return eligibleForCP(dynamic_cast<const FuncTerm*>(t),voc);
	}
	case TermType::TT_AGG: {
		return eligibleForCP(dynamic_cast<const AggTerm*>(t),str);
	}
	case TermType::TT_VAR:
	case TermType::TT_DOM:
		SortUtils::isSubsort(t->sort(), get(STDSORT::INTSORT),voc);
		return true;
	}
	//To avoid compiler warnings
	Assert(false);
	return true;
}

}
