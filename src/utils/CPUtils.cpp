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
//TODO Keep a list of CP symbols in GlobalData? Or in Translators?
std::set<const PFSymbol*> _cppredsymbols;
std::set<const Function*> _cpfuncsymbols;

std::set<const Function*> findCPSymbols(const Vocabulary* vocabulary) {
	Assert(vocabulary != NULL);
	if (_cpfuncsymbols.empty()) {
		for (auto funcit = vocabulary->firstFunc(); funcit != vocabulary->lastFunc(); ++funcit) {
			Function* function = funcit->second;
			bool passtocp = false;
			// Check whether the (user-defined) function's outsort is over integers
			if (function->overloaded()) {
				set<Function*> nonbuiltins = function->nonbuiltins();
				for (auto nbfit = nonbuiltins.cbegin(); nbfit != nonbuiltins.cend(); ++nbfit) {
					passtocp = FuncUtils::isIntFunc(*nbfit, vocabulary);
				}
			} else if (not function->builtin()) {
				passtocp = FuncUtils::isIntFunc(function, vocabulary);
			}
			if (passtocp) {
				_cpfuncsymbols.insert(function);
			}
		}
	}
	return _cpfuncsymbols;
}

bool eligibleForCP(const PredForm* pf, const Vocabulary* voc) {
	if (_cppredsymbols.find(pf->symbol()) != _cppredsymbols.cend()) {
		return true;
	} else if (VocabularyUtils::isIntComparisonPredicate(pf->symbol(),voc)) {
		_cppredsymbols.insert(pf->symbol());
		return true;
	}
	return false;
}

bool eligibleForCP(const FuncTerm* ft, const Vocabulary* voc) {
	if (_cpfuncsymbols.find(ft->function()) != _cpfuncsymbols.cend()) {
		return true;
	} else if (FuncUtils::isIntFunc(ft->function(),voc)) {
		_cpfuncsymbols.insert(ft->function());
		return true;
	}
	return false;
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
		SortUtils::isSubsort(t->sort(),VocabularyUtils::intsort(),voc);
		return true;
	}
}

} /* namespace CPUtils */


