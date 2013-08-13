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

#ifndef BUMP_HPP_
#define BUMP_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddIndex.hpp"

/*
 * Increases the indices of all free variables of a bdd with one (so that index 0 can be used
 * for a newly quantified variable)
 */
class BumpIndices: public FOBDDVisitor {
private:
	unsigned int _depth;
	const FOBDDVariable* _variable;

public:
	BumpIndices(std::shared_ptr<FOBDDManager> manager, const FOBDDVariable* variable, unsigned int depth)
			: FOBDDVisitor(manager), _depth(depth), _variable(variable) {
	}

	const FOBDDKernel* change(const FOBDDQuantKernel* kernel) {
		++_depth;
		auto bdd = FOBDDVisitor::change(kernel->bdd());
		--_depth;
		return _manager->getQuantKernel(kernel->sort(), bdd);
	}

	const FOBDDQuantSetExpr* change(const FOBDDQuantSetExpr* qse) {
		_depth += qse->quantvarsorts().size();
		auto newsetexpr =  FOBDDVisitor::change(qse);
		_depth -= qse->quantvarsorts().size();
		return newsetexpr;
	}


	const FOBDDTerm* change(const FOBDDVariable* var) {
		if (var == _variable) {
			return _manager->getDeBruijnIndex(var->variable()->sort(), _depth);
		} else {
			return var;
		}
	}

	const FOBDDTerm* change(const FOBDDDeBruijnIndex* dbr) {
		if (_depth <= dbr->index()) {
			return _manager->getDeBruijnIndex(dbr->sort(), dbr->index() + 1);
		} else {
			return dbr;
		}
	}
};

#endif /* BUMP_HPP_ */
