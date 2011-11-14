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
	ApplyDistributivity(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}

	const FOBDDArgument* change(const FOBDDFuncTerm *functerm) {
		if (!isMultiplication(functerm)) {
			return FOBDDVisitor::change(functerm);
		}
		auto leftterm = functerm->args(0);
		auto rightterm = functerm->args(1);

		if (isBddFuncTerm(leftterm)) {
			auto leftfuncterm = getBddFuncTerm(leftterm);
			if (isAddition(leftfuncterm)) {
				auto newterm = distribute(functerm, leftfuncterm, rightterm);
				return newterm->acceptchange(this);
			}
		}
		if (isBddFuncTerm(rightterm)) {
			auto rightfuncterm = getBddFuncTerm(rightterm);
			if (isAddition(rightfuncterm)) {
				auto newterm = distribute(functerm, rightfuncterm, leftterm);
				return newterm->acceptchange(this);
			}
		}

		return FOBDDVisitor::change(functerm);
	}

private:
	const FOBDDArgument* distribute(const FOBDDFuncTerm* functerm, const FOBDDFuncTerm* leftfuncterm, const FOBDDArgument* & rightterm) {
		auto newleft = _manager->getFuncTerm(functerm->func(), { leftfuncterm->args(0), rightterm });
		auto newright = _manager->getFuncTerm(functerm->func(), { leftfuncterm->args(1), rightterm });
		auto newterm = _manager->getFuncTerm(leftfuncterm->func(), { newleft, newright });
		return newterm;
	}
};

#endif /* DISTRIBUTIVITY_HPP_ */
