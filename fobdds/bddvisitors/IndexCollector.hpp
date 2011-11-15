/************************************
	IndexCollector.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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
	IndexCollector(FOBDDManager* m) :
			FOBDDVisitor(m), _minimaldepth(0) {
	}

	void visit(const FOBDDQuantKernel* kernel) {
		++_minimaldepth;
		FOBDDVisitor::visit(kernel->bdd());
		--_minimaldepth;
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
