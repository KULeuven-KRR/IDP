/************************************
	BumpIndices.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BUMP_HPP_
#define BUMP_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddIndex.hpp"

// TODO what does it do?
class BumpIndices: public FOBDDVisitor {
private:
	unsigned int _depth;
	const FOBDDVariable* _variable;

public:
	BumpIndices(FOBDDManager* manager, const FOBDDVariable* variable, unsigned int depth) :
			FOBDDVisitor(manager), _depth(depth), _variable(variable) {
	}

	const FOBDDKernel* change(const FOBDDQuantKernel* kernel) {
		++_depth;
		auto bdd = FOBDDVisitor::change(kernel->bdd());
		--_depth;
		return _manager->getQuantKernel(kernel->sort(), bdd);
	}

	const FOBDDArgument* change(const FOBDDVariable* var) {
		if (var == _variable) {
			return _manager->getDeBruijnIndex(var->variable()->sort(), _depth);
		} else {
			return var;
		}
	}

	const FOBDDArgument* change(const FOBDDDeBruijnIndex* dbr) {
		if (_depth <= dbr->index()) {
			return _manager->getDeBruijnIndex(dbr->sort(), dbr->index() + 1);
		} else {
			return dbr;
		}
	}
};

#endif /* BUMP_HPP_ */
