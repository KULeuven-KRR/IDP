/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef INDEXCOLLECTOR_HPP_
#define INDEXCOLLECTOR_HPP_

#include <set>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddKernel.hpp"
#include "fobdds/FoBddQuantKernel.hpp"

/**
 * Class to obtain all unquantified DeBruijnIndices of a bdd.
 */
class IndexCollector: public FOBDDVisitor {
private:
	std::set<const FOBDDDeBruijnIndex*> _result;
	unsigned int _minimaldepth;

public:
	IndexCollector(FOBDDManager* m)
			: FOBDDVisitor(m), _minimaldepth(0) {
	}

	void visit(const FOBDDQuantKernel* kernel) {
		++_minimaldepth;
		FOBDDVisitor::visit(kernel->bdd());
		--_minimaldepth;
	}

	void visit(const FOBDDAggKernel* ak){
		throw notyetimplemented("Collecting indices from an agg kernel");
	}

	void visit(const FOBDDDeBruijnIndex* index) {
		if (index->index() >= _minimaldepth) {
			auto i = _manager->getDeBruijnIndex(index->sort(), index->index() - _minimaldepth);
			_result.insert(i);
		}
	}

	template<typename Node>
	const std::set<const FOBDDDeBruijnIndex*>& getVariables(const Node* arg) {
		_result.clear();
		arg->accept(this);
		return _result;
	}
};

#endif /* INDEXCOLLECTOR_HPP_ */
