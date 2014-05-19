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

#include "CPUtils.hpp"

#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

using namespace std;

namespace CPSupport {

bool eligibleForCP(const PredForm* pf, const Vocabulary* voc) {
	//return VocabularyUtils::isIntComparisonPredicate(pf->symbol(), voc);
	if(pf->symbol()->isFunction()){
		return eligibleForCP(dynamic_cast<Function*>(pf->symbol()), voc);
	}else{
		return VocabularyUtils::isIntPredicate(pf->symbol(), voc);
	}
}

bool nonOverloadedNonBuiltinEligibleForCP(const Function* f, const Vocabulary* v) {
	return FuncUtils::isIntFunc(f, v);
}

bool eligibleForCP(const Function* function, const Vocabulary* voc) {
	// Check whether the (user-defined) function's outsort is over integers
	if (function->overloaded()) {
		auto nonbuiltins = const_cast<Function*>(function)->nonbuiltins();
		for (auto nbfit = nonbuiltins.cbegin(); nbfit != nonbuiltins.cend(); ++nbfit) {
			if (not nonOverloadedNonBuiltinEligibleForCP(*nbfit, voc)) {
				return false;
			}
		}
		return true;
	} else if (not function->builtin()) {
		return nonOverloadedNonBuiltinEligibleForCP(function, voc);
	} else {
		Assert(function->builtin() and not function->overloaded());
		return FuncUtils::isIntFunc(function, voc);
	}
	return false;
}

bool eligibleForCP(const FuncTerm* ft, const Vocabulary* voc) {
	return eligibleForCP(ft->function(), voc);
}

bool eligibleForCP(const AggFunction& f) {
	return (f == AggFunction::SUM) or (f == AggFunction::PROD) or (f==AggFunction::CARD) or (f == AggFunction::MAX) or (f == AggFunction::MIN);
}

bool eligibleForCP(const AggTerm* at, const Structure* str) {
	if (eligibleForCP(at->function()) and str != NULL) {
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

bool allSymbolsEligible(const Term* t, const Structure* str){
	if(not eligibleForCP(t, str)){
		return false;
	}
	for(auto term: t->subterms()){
		if(not allSymbolsEligible(term,str)){
			return false;
		}
	}
	return true;
}

bool eligibleForCP(const Term* t, const Structure* str) {
	auto voc = (str != NULL) ? str->vocabulary() : NULL;
	switch (t->type()) {
	case TermType::FUNC: {
		auto ft = dynamic_cast<const FuncTerm*>(t);
		if(str->inter(ft->function())->approxTwoValued() && getOption(REDUCEDGROUNDING)){
			return true;
		}
		return eligibleForCP(ft, voc);
	}
	case TermType::AGG: {
		return eligibleForCP(dynamic_cast<const AggTerm*>(t), str);
	}
	case TermType::VAR:
		SortUtils::isSubsort(t->sort(), get(STDSORT::INTSORT), voc);
		return true;
	case TermType::DOM:
		SortUtils::isSubsort(t->sort(), get(STDSORT::INTSORT), voc);
		return true;
	}
	Assert(false);
	return false;
}

}
