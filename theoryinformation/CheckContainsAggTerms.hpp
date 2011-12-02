#ifndef CONTAINSAGGTERMS_HPP_
#define CONTAINSAGGTERMS_HPP_

#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

class CheckContainsAggTerms: public TheoryVisitor {
private:
	bool _result;

public:
	CheckContainsAggTerms() :
			TheoryVisitor(), _result(false) {
	}

	bool containsAggTerms(const Formula* f){
		f->accept(this);
		return _result;
	}

	void visit(const AggTerm*){
		_result = true;
	}
};

#endif /* CONTAINSAGGTERMS_HPP_ */
