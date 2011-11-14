/************************************
  	SplitComparisonChains.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef REMOVEEQUATIONCHAINS_HPP_
#define REMOVEEQUATIONCHAINS_HPP_

#include <vector>
#include "visitors/TheoryMutatingVisitor.hpp"

class Vocabulary;

class SplitComparisonChains: public TheoryMutatingVisitor {
private:
	Vocabulary* _vocab;
public:
	SplitComparisonChains() :
			TheoryMutatingVisitor(), _vocab(0) {
	}
	SplitComparisonChains(Vocabulary* v) :
			TheoryMutatingVisitor(), _vocab(v) {
	}

	Formula* visit(EqChainForm*);
};

#endif /* REMOVEEQUATIONCHAINS_HPP_ */
