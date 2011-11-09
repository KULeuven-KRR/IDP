#ifndef GRAPHAGGREGATES_HPP_
#define GRAPHAGGREGATES_HPP_

#include "TheoryMutatingVisitor.hpp"

class GraphAggregates: public TheoryMutatingVisitor {
private:
	bool _recursive;
public:
	GraphAggregates(bool recursive = false) :
			_recursive(recursive) {
	}
	Formula* visit(PredForm* pf);
	Formula* visit(EqChainForm* ef);
};

#endif /* GRAPHAGGREGATES_HPP_ */
