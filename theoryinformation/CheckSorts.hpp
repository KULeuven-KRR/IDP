#ifndef CHECKSORTS_HPP_
#define CHECKSORTS_HPP_

#include "TheoryVisitor.hpp"

class Vocabulary;

class CheckSorts: public TheoryVisitor {
private:
	Vocabulary* _vocab;

public:
	CheckSorts(Formula* f, Vocabulary* v);
	CheckSorts(Term* t, Vocabulary* v);
	CheckSorts(Definition* d, Vocabulary* v);
	CheckSorts(FixpDef* d, Vocabulary* v);

	virtual ~CheckSorts() {}

	void visit(const PredForm*);
	void visit(const EqChainForm*);
	void visit(const FuncTerm*);
	void visit(const AggTerm*);

};

#endif /* CHECKSORTS_HPP_ */
