#ifndef CONTAINSFUNCTERMS_HPP_
#define CONTAINSFUNCTERMS_HPP_

#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

class CheckContainsFuncTerms: public TheoryVisitor {
	VISITORFRIENDS()
private:
	bool _result;

public:
	bool execute(const Formula* f){
		_result = false;
		f->accept(this);
		return _result;
	}

protected:
	void visit(const FuncTerm*){
		_result = true;
	}
};

#endif /* CONTAINSFUNCTERMS_HPP_ */
