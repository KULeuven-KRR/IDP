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

#ifndef REMOVEMINUS_HPP_
#define REMOVEMINUS_HPP_

#include "IncludeComponents.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

/**
 * Class to replace (t1 - t2) by (t1 + (-1) * t2) and (-t) by ((-1) * t)
 */
class RewriteMinus: public FOBDDVisitor {
public:
	RewriteMinus(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}

	// Replace (t1 - t2) by (t1 + (-1) * t2)
	const FOBDDTerm* rewriteBinaryMinus(const FOBDDFuncTerm *& functerm) {
		auto plus = get(STDFUNC::ADDITION);
		plus = plus->disambiguate(functerm->func()->sorts(), NULL);
		Assert(plus!=NULL);
		auto rhs = functerm->args(1);
		auto minusoneterm = _manager->getDomainTerm(rhs->sort(), createDomElem(-1));
		auto times = get(STDFUNC::PRODUCT);
		times = times->disambiguate(std::vector<Sort*>(3, rhs->sort()), NULL);
		auto newprodterm = _manager->getFuncTerm(times, { minusoneterm, rhs });
		auto newterm = _manager->getFuncTerm(plus, { functerm->args(0), newprodterm });
		return newterm->acceptchange(this);
	}

	// Replace -t by (-1)*t
	const FOBDDTerm* rewriteUnaryMinus(const FOBDDFuncTerm *& functerm) {
		auto minusoneterm = _manager->getDomainTerm(functerm->args(0)->sort(), createDomElem(-1));
		auto times = get(STDFUNC::PRODUCT);
		times = times->disambiguate(std::vector<Sort*>(3, functerm->args(0)->sort()), NULL);
		auto newterm = _manager->getFuncTerm(times, { minusoneterm, functerm->args(0) });
		return newterm->acceptchange(this);
	}

	const FOBDDTerm *change(const FOBDDFuncTerm *functerm) {
		if (is(functerm->func(), STDFUNC::SUBSTRACTION)) {
			return rewriteBinaryMinus(functerm);
		} else if (is(functerm->func(), STDFUNC::UNARYMINUS)) {
			return rewriteUnaryMinus(functerm);
		} else {
			return FOBDDVisitor::change(functerm);
		}
	}
};

#endif /* REMOVEMINUS_HPP_ */
