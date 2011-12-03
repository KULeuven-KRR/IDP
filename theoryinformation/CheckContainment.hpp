#ifndef CONTAINMENTCHECKER_HPP_
#define CONTAINMENTCHECKER_HPP_

#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

class CheckContainment: public TheoryVisitor {
	VISITORFRIENDS()
private:
	const PFSymbol* _symbol;
	bool _result;

public:
	bool execute(const PFSymbol* s, const Formula* f);

protected:
	void visit(const PredForm* pf);
	void visit(const FuncTerm* ft);
};

#endif /* CONTAINMENTCHECKER_HPP_ */
