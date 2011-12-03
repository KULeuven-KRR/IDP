#ifndef REMOVEEQUATIONCHAINS_HPP_
#define REMOVEEQUATIONCHAINS_HPP_

#include <vector>
#include "visitors/TheoryMutatingVisitor.hpp"

class Vocabulary;

class SplitComparisonChains: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	Vocabulary* _vocab;
public:
	template<typename T>
	T execute(T t, Vocabulary* v = NULL){
		_vocab = v;
		return t->accept(this);
	}

protected:
	Formula* visit(EqChainForm*);
};

#endif /* REMOVEEQUATIONCHAINS_HPP_ */
