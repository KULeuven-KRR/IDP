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
	return VocabularyUtils::isIntComparisonPredicate(pf->symbol(), voc);
}

bool nonOverloadedNonBuiltinEligibleForCP(Function* f, const Vocabulary* v) {
	if (f->partial()) { // TODO at the moment, partial terms are never eligible for CP
		return false;
	}
	if (not FuncUtils::isIntFunc(f, v)) {
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
			if (not nonOverloadedNonBuiltinEligibleForCP(*nbfit, voc)) {
				return false;
			}
		}
		passtocp = true;
	} else if (not function->builtin()) {
		passtocp = nonOverloadedNonBuiltinEligibleForCP(function, voc);
	} else {
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
		auto enumset = at->set();
		for (auto i = enumset->getSets().cbegin(); i < enumset->getSets().cend(); ++i) {
//			if (not FormulaUtils::approxTwoValued((*i)->getCondition(), str)) {
//				return false;
//			}
			if (not eligibleForCP((*i)->getTerm(), str)) {
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
	case TermType::FUNC: {
		return eligibleForCP(dynamic_cast<const FuncTerm*>(t), voc);
	}
	case TermType::AGG: {
		return eligibleForCP(dynamic_cast<const AggTerm*>(t), str);
	}
	case TermType::VAR:
	case TermType::DOM:
		SortUtils::isSubsort(t->sort(), get(STDSORT::INTSORT), voc);
		return true;
	}
}

}
