#include <vector>
#include <cassert>

#include "theorytransformations/GraphAggregates.hpp"
#include "theorytransformations/SplitComparisonChains.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

Formula* GraphAggregates::visit(PredForm* pf) {
	if (_recursive) {
		pf = dynamic_cast<PredForm*>(traverse(pf));
	}
	if (VocabularyUtils::isComparisonPredicate(pf->symbol())) {
		CompType comparison;
		if (pf->symbol()->name() == "=/2") {
			if (isPos(pf->sign())) {
				comparison = CompType::EQ;
			} else {
				comparison = CompType::NEQ;
			}
		} else if (pf->symbol()->name() == "</2") {
			if (isPos(pf->sign())) {
				comparison = CompType::LT;
			} else {
				comparison = CompType::GEQ;
			}
		} else {
			assert(pf->symbol()->name() == ">/2");
			if (isPos(pf->sign())) {
				comparison = CompType::GT;
			} else {
				comparison = CompType::LEQ;
			}
		}
		Formula* newpf = 0;
		if (typeid(*(pf->subterms()[0])) == typeid(AggTerm)) {
			AggTerm* at = dynamic_cast<AggTerm*>(pf->subterms()[0]);
			newpf = new AggForm(SIGN::POS, pf->subterms()[1], comparison, at, pf->pi().clone());
			delete (pf);
		} else if (typeid(*(pf->subterms()[1])) == typeid(AggTerm)) {
			AggTerm* at = dynamic_cast<AggTerm*>(pf->subterms()[1]);
			newpf = new AggForm(SIGN::POS, pf->subterms()[0], comparison, at, pf->pi().clone());
			delete (pf);
		} else {
			newpf = pf;
		}
		return newpf;
	} else {
		return pf;
	}
}

Formula* GraphAggregates::visit(EqChainForm* ef) {
	if (_recursive) {
		ef = dynamic_cast<EqChainForm*>(traverse(ef));
	}
	bool containsaggregates = false;
	for (unsigned int n = 0; n < ef->subterms().size(); ++n) {
		if (typeid(*(ef->subterms()[n])) == typeid(AggTerm)) {
			containsaggregates = true;
			break;
		}
	}
	if (containsaggregates) {
		SplitComparisonChains splitter;
		auto f = splitter.visit(ef);
		return f->accept(this);
	} else {
		return ef;
	}
}
