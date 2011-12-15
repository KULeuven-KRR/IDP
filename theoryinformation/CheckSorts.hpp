#ifndef CHECKSORTS_HPP_
#define CHECKSORTS_HPP_

#include "visitors/TheoryVisitor.hpp"

class Vocabulary;

class CheckSorts: public TheoryVisitor {
	VISITORFRIENDS()
private:
	Vocabulary* _vocab;

public:
	template<typename T>
	void execute(T f, Vocabulary* v){
		_vocab = v;
		f->accept(this);
	}

protected:
	void visit(const PredForm*);
	void visit(const EqChainForm*);
	void visit(const FuncTerm*);
	void visit(const AggTerm*);
};

#endif /* CHECKSORTS_HPP_ */
