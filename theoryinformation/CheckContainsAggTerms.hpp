#ifndef CONTAINSAGGTERMS_HPP_
#define CONTAINSAGGTERMS_HPP_

#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

class CheckContainsAggTerms: public TheoryVisitor {
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
	void visit(const AggTerm*){
		_result = true;
	}
};

#endif /* CONTAINSAGGTERMS_HPP_ */
