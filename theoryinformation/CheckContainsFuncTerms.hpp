/************************************
  	CheckContainsFuncTerms.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CONTAINSFUNCTERMS_HPP_
#define CONTAINSFUNCTERMS_HPP_

#include "visitors/TheoryVisitor.hpp"

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

#endif /* CONTAINSFUNCTERMS_HPP_ */
