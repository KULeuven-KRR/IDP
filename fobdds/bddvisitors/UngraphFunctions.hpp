/************************************
	UngraphFunctions.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FUNCATOMREMOVER_HPP_
#define FUNCATOMREMOVER_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

/**
 * Class to replace an atom F(x,y) by F(x) = y
 */
class UngraphFunctions: public FOBDDVisitor {
public:
	UngraphFunctions(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}

	const FOBDDKernel* change(const FOBDDAtomKernel* atom) {
		if (not sametypeid<Function>(*(atom->symbol())) || atom->type() != AtomKernelType::AKT_TWOVALUED) {
			return atom;
		}

		auto function = dynamic_cast<Function*>(atom->symbol());
		auto outsort = SortUtils::resolve(function->outsort(), atom->args().back()->sort());
		Predicate* equalpred = Vocabulary::std()->pred("=/2");
		equalpred = equalpred->disambiguate(std::vector<Sort*>(2, outsort));
		auto funcargs = atom->args();
		funcargs.pop_back();
		auto functerm = _manager->getFuncTerm(function, funcargs);
		return _manager->getAtomKernel(equalpred, AtomKernelType::AKT_TWOVALUED, {functerm, atom->args().back()});
	}
};

#endif /* FUNCATOMREMOVER_HPP_ */
