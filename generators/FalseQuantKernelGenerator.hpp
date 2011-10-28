/************************************
	FalseQuantKernelGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FALSEQUANTKERNELGENERATOR_HPP_
#define FALSEQUANTKERNELGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

/**
 * Generate all x such that ?x phi(x) is false.
 * Given is a generator for the universe and a checker which returns true if phi(x) is true.
 */
class FalseQuantKernelGenerator : public InstGenerator {
private:
	InstGenerator*	universeGenerator;
	InstChecker*	quantKernelTrueChecker;

public:
	FalseQuantKernelGenerator(InstGenerator* universegenerator, InstChecker* bddtruechecker)
			: universeGenerator(universegenerator), quantKernelTrueChecker(bddtruechecker){ }

	bool check() const{
		return not quantKernelTrueChecker->check();
	}

	void reset(){
		universeGenerator->reset();
		if(universeGenerator->isAtEnd()){
			notifyAtEnd();
		}
	}

	void next(){
		universeGenerator->next();
		while(not universeGenerator->isAtEnd() && quantKernelTrueChecker->check()){
			universeGenerator->next();
		}
		if(universeGenerator->isAtEnd()){
			notifyAtEnd();
		}
	}
};

#endif /* FALSEQUANTKERNELGENERATOR_HPP_ */
