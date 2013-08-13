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

#ifndef DISTRIBUTIVITY_HPP_
#define DISTRIBUTIVITY_HPP_

#include <vector>
#include <map>
#include <set>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

/**
 * Class to exhaustively distribute addition with respect to multiplication in a functerm
 */
class ApplyDistributivity: public FOBDDVisitor {
public:
	ApplyDistributivity(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}

	const FOBDDTerm* change(const FOBDDFuncTerm *functerm) {
		if (!isMultiplication(functerm)) {
			return FOBDDVisitor::change(functerm);
		}
		auto leftterm = functerm->args(0);
		auto rightterm = functerm->args(1);

		if (isBddFuncTerm(leftterm)) {
			auto leftfuncterm = castBddFuncTerm(leftterm);
			if (isAddition(leftfuncterm)) {
				auto newterm = distribute(functerm, leftfuncterm, rightterm);
				return newterm->acceptchange(this);
			}
		}
		if (isBddFuncTerm(rightterm)) {
			auto rightfuncterm = castBddFuncTerm(rightterm);
			if (isAddition(rightfuncterm)) {
				auto newterm = distribute(functerm, rightfuncterm, leftterm);
				return newterm->acceptchange(this);
			}
		}

		return FOBDDVisitor::change(functerm);
	}

private:
	const FOBDDTerm* distribute(const FOBDDFuncTerm* functerm, const FOBDDFuncTerm* leftfuncterm, const FOBDDTerm* & rightterm) {
		auto newleft = _manager->getFuncTerm(functerm->func(), { leftfuncterm->args(0), rightterm });
		auto newright = _manager->getFuncTerm(functerm->func(), { leftfuncterm->args(1), rightterm });
		auto newterm = _manager->getFuncTerm(leftfuncterm->func(), { newleft, newright });
		return newterm;
	}
};

#endif /* DISTRIBUTIVITY_HPP_ */
