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

#ifndef TERMSTOLEFT_HPP_
#define TERMSTOLEFT_HPP_

#include "IncludeComponents.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"
#include "fobdds/FoBddAtomKernel.hpp"

/**
 * Class to move all terms in a comparison to the left hand side
 */
class TermsToLeft: public FOBDDVisitor {
public:
	TermsToLeft(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}

	const FOBDDKernel* change(const FOBDDAtomKernel* atom) {
		const FOBDDTerm* lhs = NULL;
		const FOBDDTerm* rhs = NULL;
		if (isa<Function>(*(atom->symbol()))) { // f(\xx)=y
			auto lhsterms = atom->args();
			lhsterms.pop_back();
			lhs = _manager->getFuncTerm(dynamic_cast<Function*>(atom->symbol()), lhsterms);
			rhs = atom->args().back();
		} else { // x op y
			if (VocabularyUtils::isComparisonPredicate(atom->symbol())) {
				lhs = atom->args(0);
				rhs = atom->args(1);
			}
		}

		if (lhs == NULL || rhs == NULL || not SortUtils::isSubsort(rhs->sort(), get(STDSORT::FLOATSORT))) {
			return atom;
		}

		auto zeroterm = _manager->getDomainTerm(rhs->sort(), createDomElem(0));
		if (rhs == zeroterm) {
			return atom;
		}

		auto sort = SortUtils::resolve(lhs->sort(), rhs->sort());
		if (sort == NULL) { // No common ancestor, so cannot do minus? TODO (can we allow this, or even simplify (can never become equal?) Maybe this is done somewhere else
			return atom;
		}

		// => got a comparison lhs op rhs, rhs is the only term not on the left (op is =, < or >
		// so move it by negating and setting the other side to 0
		auto minus = get(STDFUNC::SUBSTRACTION);
		minus = minus->disambiguate(std::vector<Sort*>(2, sort), NULL);
		Assert(minus!=NULL);
		auto newlhs = _manager->getFuncTerm(minus, { lhs, rhs });
		return _manager->getAtomKernel(atom->symbol(), atom->type(), { newlhs, zeroterm });
	}
};

#endif /* TERMSTOLEFT_HPP_ */
