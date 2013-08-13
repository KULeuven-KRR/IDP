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

#ifndef ADDMULTSIMPLIFIER_HPP_
#define ADDMULTSIMPLIFIER_HPP_

#include "IncludeComponents.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

/**
 * Recursively from leaves to top do:
 * 		Replace 0+x with x
 * 		Replace 0*x with 0, 1*x with x
 *		Replace x+y or x*y with x and y both domain terms with their result
 * 		Replace x+(y+z) or x*(y*z) with x and y both domain terms with result+z or result*z
 */
class AddMultSimplifier: public FOBDDVisitor {
public:
	AddMultSimplifier(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}

	const FOBDDTerm* change(const FOBDDFuncTerm* functerm) {
		// Depth first: recurse
		auto recurterm = FOBDDVisitor::change(functerm);

		if (not isBddFuncTerm(recurterm)) {
			return recurterm;
		}

		functerm = castBddFuncTerm(recurterm);

		if (not isAddition(functerm) && not isMultiplication(functerm)) {
			return recurterm;
		}

		if (not isBddDomainTerm(functerm->args(0))) {
			return recurterm;
		}

		auto leftconstant = castBddDomainTerm(functerm->args(0));

		auto zero = createDomElem(0);
		auto one = createDomElem(1);
		if (isAddition(functerm) && leftconstant->value() == zero) {
			return functerm->args(1)->acceptchange(this);
		} else if (isMultiplication(functerm) && leftconstant->value() == one) {
			return functerm->args(1)->acceptchange(this);
		} else if (isMultiplication(functerm) && leftconstant->value() == zero) {
			return _manager->getDomainTerm(functerm->sort(), zero);
		}

		if (isBddDomainTerm(functerm->args(1))) {
			auto rightconstant = castBddDomainTerm(functerm->args(1));
			auto fi = functerm->func()->interpretation(NULL);
			auto result = fi->funcTable()->operator[]( { leftconstant->value(), rightconstant->value() });
			Assert(result != NULL);
			return _manager->getDomainTerm(functerm->func()->outsort(), result);
		}

		if (isBddFuncTerm(functerm->args(1))) {
			auto rightterm = castBddFuncTerm(functerm->args(1));
			if (rightterm->func()->name() == functerm->func()->name() && isBddDomainTerm(rightterm->args(0))) {
				auto rightconstant = castBddDomainTerm(rightterm->args(0));
				auto inter = functerm->func()->interpretation(0);
				auto result = inter->funcTable()->operator[]( { leftconstant->value(), rightconstant->value() });
				Assert(result != NULL);
				auto resultterm = _manager->getDomainTerm(functerm->func()->outsort(), result);
				auto newterm = _manager->getFuncTerm(rightterm->func(), { resultterm, rightterm->args(1) });
				return newterm->acceptchange(this);
			}
		}

		return recurterm;
	}
};

#endif /* ADDMULTSIMPLIFIER_HPP_ */
