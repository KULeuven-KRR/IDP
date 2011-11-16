/************************************
  	CheckContainment.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CONTAINMENTCHECKER_HPP_
#define CONTAINMENTCHECKER_HPP_

#include "visitors/TheoryVisitor.hpp"

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
