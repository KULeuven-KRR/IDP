#ifndef CHECKFUNCTERMS_HPP_
#define CHECKFUNCTERMS_HPP_

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
