#ifndef CONTAINMSFUNCTERMS_HPP_
#define CONTAINMSFUNCTERMS_HPP_

#include "TheoryVisitor.hpp"

class PFSymbol;

class CheckContainsFuncTerms: public TheoryVisitor {
private:
	bool _result;

public:
	CheckContainsFuncTerms() :
			TheoryVisitor(), _result(false) {
	}

	bool containsFuncTerms(const Formula* f){
		f->accept(this);
		return _result;
	}

	void visit(const FuncTerm*){
		_result = true;
	}
};

#endif /* CONTAINMSFUNCTERMS_HPP_ */
