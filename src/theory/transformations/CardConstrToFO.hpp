#pragma once

#include "visitors/TheoryMutatingVisitor.hpp"

#include <set>
#include <map>
#include "vocabulary/vocabulary.hpp"

class CardConstrToFO: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	int _maxVarsToIntroduce;
public:
	template<typename T>
	T execute(T t, int maxVarsToIntroduce) {
		_maxVarsToIntroduce = maxVarsToIntroduce;
		return t->accept(this);
	}
protected:
	Formula* visit(AggForm*);
	Formula* visit(EqChainForm* ef);
	Formula* visit(PredForm* pf);

private:
	QuantForm* solveGreater(size_t, AggTerm*);
	QuantForm* solveLesser(size_t, AggTerm*);
};
