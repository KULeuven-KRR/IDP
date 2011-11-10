#ifndef ORDERTERMS_HPP_
#define ORDERTERMS_HPP_

#include <vector>
#include <algorithm>
#include <cassert>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/bddvisitors/TermCollector.hpp"

#include "vocabulary.hpp"

/**
 * Recursively from leaves to top do:
 * 		If it is an addition, order the leaves in reverse SWO order
 */
template<typename Ordering>
class OrderTerms: public FOBDDVisitor {
public:
	OrderTerms(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}

	const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
		if (functerm->func()->name() != Ordering::getFuncName()) {
			return FOBDDVisitor::change(functerm);
		}

		TermCollector mte(_manager); // Collects all subterms which are reachable only by functerms of the provided type
		auto terms = mte.getTerms(functerm, Ordering::getFuncName());
		for (auto i = terms.begin(); i < terms.end(); ++i) {
			*i = (*i)->acceptchange(this);
		}

		Ordering mtswo;
		std::sort(terms.begin(), terms.end(), mtswo);

		const FOBDDArgument* currarg = terms.back();
		for (auto i = terms.crbegin(); i < terms.crend(); ++i) { // NOTE: reverse!
			auto nextarg = *i;
			auto sort = SortUtils::resolve(currarg->sort(), nextarg->sort());
			auto add = Vocabulary::std()->func(Ordering::getFuncName());
			add = add->disambiguate(std::vector<Sort*>(3, sort), NULL);
			assert(add!=NULL);
			currarg = _manager->getFuncTerm(add, { nextarg, currarg });
		}
		return currarg;
	}
};

#endif /* ORDERTERMS_HPP_ */