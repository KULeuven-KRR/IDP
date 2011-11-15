/************************************
	AddMultSimplifier.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ADDMULTSIMPLIFIER_HPP_
#define ADDMULTSIMPLIFIER_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

#include "vocabulary.hpp"
#include "structure.hpp"

/**
 * Recursively from leaves to top do:
 * 		Replace 0+x with x
 * 		Replace 0*x with 0, 1*x with x
 *		Replace x+y or x*y with x and y both domain terms with their result
 * 		Replace x+(y+z) or x*(y*z) with x and y both domain terms with result+z or result*z
 */
class AddMultSimplifier: public FOBDDVisitor {
public:
	AddMultSimplifier(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}

	const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
		// Depth first: recurse
		auto recurterm = FOBDDVisitor::change(functerm);

		if (not isBddFuncTerm(recurterm)) {
			return recurterm;
		}

		functerm = getBddFuncTerm(recurterm);

		if (not isAddition(functerm) && not isMultiplication(functerm)) {
			return recurterm;
		}

		if (not isBddDomainTerm(functerm->args(0))) {
			return recurterm;
		}

		auto leftconstant = getBddDomainTerm(functerm->args(0));

		auto zero = createDomElem(0); auto one = createDomElem(1);
		if(isAddition(functerm) && leftconstant->value()==zero){
			return functerm->args(1)->acceptchange(this);
		}else if(isMultiplication(functerm) && leftconstant->value()==one){
			return functerm->args(1)->acceptchange(this);
		}else if(isMultiplication(functerm) && leftconstant->value()==zero){
			return _manager->getDomainTerm(functerm->sort(), zero);
		}

		if (isBddDomainTerm(functerm->args(1))) {
			auto rightconstant = getBddDomainTerm(functerm->args(1));
			auto fi = functerm->func()->interpretation(NULL);
			auto result = fi->funcTable()->operator[]( { leftconstant->value(), rightconstant->value() });
			return _manager->getDomainTerm(functerm->func()->outsort(), result);
		}

		if (isBddFuncTerm(functerm->args(1))) {
			auto rightterm = getBddFuncTerm(functerm->args(1));
			if (rightterm->func()->name() == functerm->func()->name() && isBddDomainTerm(rightterm->args(0))) {
				auto rightconstant = getBddDomainTerm(rightterm->args(0));
				auto inter = functerm->func()->interpretation(0);
				auto result = inter->funcTable()->operator[]( { leftconstant->value(), rightconstant->value() });
				auto resultterm = _manager->getDomainTerm(functerm->func()->outsort(), result);
				auto newterm = _manager->getFuncTerm(rightterm->func(), { resultterm, rightterm->args(1) });
				return newterm->acceptchange(this);
			}
		}

		return recurterm;
	}
};

#endif /* ADDMULTSIMPLIFIER_HPP_ */
