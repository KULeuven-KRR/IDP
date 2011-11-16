/************************************
	CheckFuncTerms.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CHECKFUNCTERMS_HPP_
#define CHECKFUNCTERMS_HPP_

#include "visitors/TheoryVisitor.hpp"

class FormulaFuncTermChecker: public TheoryVisitor {
private:
	bool _result;
	void visit(const FuncTerm*) {
		_result = true;
	}
public:
	bool containsFuncTerms(Formula* f) {
		_result = false;
		f->accept(this);
		return _result;
	}
};

#endif /* CHECKFUNCTERMS_HPP_ */
