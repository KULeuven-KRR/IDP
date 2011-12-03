#ifndef GRAPHAGGREGATES_HPP_
#define GRAPHAGGREGATES_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class GraphAggregates: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	bool _recursive;
public:
	template<typename T>
	T execute(T t, bool recursive = false){
		_recursive = recursive;
		return t->accept(this);
	}

protected:
	Formula* visit(PredForm* pf);
	Formula* visit(EqChainForm* ef);
};

#endif /* GRAPHAGGREGATES_HPP_ */
