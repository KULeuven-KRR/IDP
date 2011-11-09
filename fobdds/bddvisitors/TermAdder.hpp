#ifndef TERMADDER_HPP_
#define TERMADDER_HPP_

#include <vector>
#include <cassert>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

// FIXME geen idee van gebruik
class TermAdder: public FOBDDVisitor {
public:
	TermAdder(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}

	const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
		if (not isAddition(functerm)) {
			return FOBDDVisitor::change(functerm);
		}
		NonConstTermExtractor ncte;
		const FOBDDArgument* leftncte = ncte.run(functerm->args(0));

		if (typeid(*(functerm->args(1))) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(functerm->args(1));
			if (rightterm->func()->name() == "+/2") {
				const FOBDDArgument* rightncte = ncte.run(rightterm->args(0));
				if (leftncte == rightncte) {
					ConstTermExtractor cte(_manager);
					const FOBDDDomainTerm* leftconst = cte.run(functerm->args(0));
					const FOBDDDomainTerm* rightconst = cte.run(rightterm->args(0));
					const FOBDDDomainTerm* addterm = add(leftconst, rightconst);
					Function* mult = Vocabulary::std()->func("*/2");
					Sort* multsort = SortUtils::resolve(addterm->sort(), leftncte->sort());
					std::vector<Sort*> multsorts(3, multsort);
					mult = mult->disambiguate(multsorts, 0);
					assert(mult);
					std::vector<const FOBDDArgument*> multargs(2);
					multargs[0] = addterm;
					multargs[1] = leftncte;
					const FOBDDArgument* newterm = _manager->getFuncTerm(mult, multargs);
					Function* plus = Vocabulary::std()->func("+/2");
					Sort* plussort = SortUtils::resolve(newterm->sort(), rightterm->args(1)->sort());
					std::vector<Sort*> plussorts(3, plussort);
					plus = plus->disambiguate(plussorts, 0);
					assert(plus);
					std::vector<const FOBDDArgument*> plusargs(2);
					plusargs[0] = newterm;
					plusargs[1] = rightterm->args(1);
					const FOBDDArgument* addbddterm = _manager->getFuncTerm(plus, plusargs);
					return addbddterm->acceptchange(this);
				} else {
					return FOBDDVisitor::change(functerm);
				}
			}
		}

		const FOBDDArgument* rightncte = ncte.run(functerm->args(1));
		if (leftncte == rightncte) {
			ConstTermExtractor cte(_manager);
			const FOBDDDomainTerm* leftconst = cte.run(functerm->args(0));
			const FOBDDDomainTerm* rightconst = cte.run(functerm->args(1));
			const FOBDDDomainTerm* addterm = add(_manager, leftconst, rightconst);
			Function* mult = Vocabulary::std()->func("*/2");
			Sort* multsort = SortUtils::resolve(addterm->sort(), leftncte->sort());
			std::vector<Sort*> multsorts(3, multsort);
			mult = mult->disambiguate(multsorts, 0);
			assert(mult);
			std::vector<const FOBDDArgument*> multargs(2);
			multargs[0] = addterm;
			multargs[1] = leftncte;
			return _manager->getFuncTerm(mult, multargs);
		}
		return FOBDDVisitor::change(functerm);
	}
};

#endif /* TERMADDER_HPP_ */
