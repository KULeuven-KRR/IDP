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
	UngraphFunctions(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}

	const FOBDDKernel* change(const FOBDDAtomKernel* atom) {
		if (not isa<Function>(*(atom->symbol())) || atom->type() != AtomKernelType::AKT_TWOVALUED) {
			return atom;
		}

		auto function = dynamic_cast<Function*>(atom->symbol());
		auto outsort = SortUtils::resolve(function->outsort(), atom->args().back()->sort());
		Predicate* equalpred = get(STDPRED::EQ, outsort);
		auto funcargs = atom->args();
		funcargs.pop_back();
		auto functerm = _manager->getFuncTerm(function, funcargs);
		return _manager->getAtomKernel(equalpred, AtomKernelType::AKT_TWOVALUED, { functerm, atom->args().back() });
	}
};

#endif /* FUNCATOMREMOVER_HPP_ */
