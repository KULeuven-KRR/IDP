#pragma once

#include "visitors/TheoryMutatingVisitor.hpp"

#include <set>
#include <map>
#include "vocabulary/vocabulary.hpp"

class CardConstrToFO: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	int _maxbound;
public:
	template<typename T>
	T execute(T t, int maxbound) {
		_maxbound = maxbound;
		return t->accept(this);
	}
protected:
	Formula* visit(AggForm*);
	Formula* visit(EqChainForm* ef);
	Formula* visit(PredForm* pf);

private:
	QuantForm* solveGreater(int, AggTerm*);
	QuantForm* solveLesser(int, AggTerm*);
};
