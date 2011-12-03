#ifndef CHECKFUNCTERMS_HPP_
#define CHECKFUNCTERMS_HPP_

#include "visitors/TheoryVisitor.hpp"

class FormulaFuncTermChecker: public TheoryVisitor {
	VISITORFRIENDS()
private:
	bool _result;
protected:
	void visit(const FuncTerm*) {
		_result = true;
	}
public:
	bool execute(Formula* f) {
		_result = false;
		f->accept(this);
		return _result;
	}
};

#endif /* CHECKFUNCTERMS_HPP_ */
