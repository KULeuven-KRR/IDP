#ifndef CONTAINMENTCHECKER_HPP_
#define CONTAINMENTCHECKER_HPP_

#include "TheoryVisitor.hpp"

class PFSymbol;

class CheckContainment: public TheoryVisitor {
private:
	const PFSymbol* _symbol;
	bool _result;

public:
	CheckContainment(const PFSymbol* s) :
			TheoryVisitor(), _symbol(s), _result(false) {
	}

	bool containsSymbol(const Formula* f);

	void visit(const PredForm* pf);

	void visit(const FuncTerm* ft);
};

#endif /* CONTAINMENTCHECKER_HPP_ */
