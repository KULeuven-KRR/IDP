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

#include "common.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

#include "fobdds/bddvisitors/FirstConstTermInFunc.hpp"
#include "fobdds/bddvisitors/FirstNonConstMultTerm.hpp"

/**
 * Given the root is an addition
 *
 * TODO should only be called on certain parse trees, document this!
 */
class CombineConstsOfMults: public FOBDDVisitor {
public:
	CombineConstsOfMults(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}

	const FOBDDTerm* change(const FOBDDFuncTerm* functerm) {
		if (not isAddition(functerm)) {
			return FOBDDVisitor::change(functerm);
		}

		FirstNonConstMultTerm ncte(_manager);
		auto leftncte = ncte.run(functerm->args(0));

		if (isBddFuncTerm(functerm->args(1))) {
			auto rightterm = castBddFuncTerm(functerm->args(1));
			if (isAddition(rightterm)) {
				auto rightncte = ncte.run(rightterm->args(0));
				if (leftncte != rightncte) {
					return FOBDDVisitor::change(functerm);
				}
				FirstConstMultTerm cte(_manager);
				auto leftconst = cte.run(functerm->args(0));
				auto rightconst = cte.run(rightterm->args(0));
				auto addterm = add(_manager, leftconst, rightconst);
				auto mult = get(STDFUNC::PRODUCT);
				auto multsort = SortUtils::resolve(addterm->sort(), leftncte->sort());
				mult = mult->disambiguate(std::vector<Sort*>(3, multsort), NULL);
				Assert(mult!=NULL);
				auto newterm = _manager->getFuncTerm(mult, { addterm, leftncte });
				auto plus = get(STDFUNC::ADDITION);
				auto plussort = SortUtils::resolve(newterm->sort(), rightterm->args(1)->sort());
				plus = plus->disambiguate(std::vector<Sort*>(3, plussort), NULL);
				Assert(plus!=NULL);
				auto addbddterm = _manager->getFuncTerm(plus, { newterm, rightterm->args(1) });
				return addbddterm->acceptchange(this);
			}
		}

		auto rightncte = ncte.run(functerm->args(1));
		if (leftncte == rightncte) {
			FirstConstMultTerm cte(_manager);
			auto leftconst = cte.run(functerm->args(0));
			auto rightconst = cte.run(functerm->args(1));
			auto addterm = add(_manager, leftconst, rightconst);
			auto mult = get(STDFUNC::PRODUCT);
			auto multsort = SortUtils::resolve(addterm->sort(), leftncte->sort());
			mult = mult->disambiguate(std::vector<Sort*>(3, multsort), NULL);
			Assert(mult!=NULL);
			return _manager->getFuncTerm(mult, { addterm, leftncte });
		}

		return FOBDDVisitor::change(functerm);
	}
};
