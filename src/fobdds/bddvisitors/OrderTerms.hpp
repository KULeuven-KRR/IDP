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

#pragma once

#include "IncludeComponents.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/bddvisitors/TermCollector.hpp"
#include <algorithm>

/**
 * Recursively from leaves to top do:
 * Order terms that are reachable by a certain function (ordering::getFuncName)
 * Can be used for example for Multiplication or Addition
 */
template<typename Ordering>
class OrderTerms: public FOBDDVisitor {
public:
	OrderTerms(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}

	const FOBDDTerm* change(const FOBDDFuncTerm* functerm) {
		if (functerm->func()->name() != Ordering::getFuncName()) {
			return FOBDDVisitor::change(functerm);
		}

		TermCollector mte(_manager); // Collects all subterms which are reachable only by functerms of the provided type
		auto terms = mte.getTerms(functerm, Ordering::getFuncName());
		for (auto i = terms.begin(); i < terms.end(); ++i) {
			*i = (*i)->acceptchange(this);
		}

		Ordering mtswo(_manager);
		std::sort(terms.begin(), terms.end(), mtswo);

		const FOBDDTerm* currarg = terms.back();
		bool begin = true;
		for (auto i = terms.crbegin(); i < terms.crend(); ++i) { // NOTE: reverse!
			if (not begin) {
				auto nextarg = *i;
				auto sort = SortUtils::resolve(currarg->sort(), nextarg->sort());
				auto add = Vocabulary::std()->func(Ordering::getFuncName());
				add = add->disambiguate(std::vector<Sort*>(3, sort), NULL);
				Assert(add!=NULL);
				currarg = _manager->getFuncTerm(add, { nextarg, currarg });
			} else {
				begin = false;
			}
		}
		return currarg;
	}
};
