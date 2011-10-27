/************************************
	FalseQuantKernelGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FALSEQUANTKERNELGENERATOR_HPP_
#define FALSEQUANTKERNELGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

/**
 * Given a universe generator and a checker whether it is in the quantkernel, generate all tuples in the falsequantkernel.
 *
 * FIXME is this code what is meant? what does the FALSEquantkernelgen do here?
 */
class FalseQuantKernelGenerator : public InstGenerator {
private:
	InstGenerator*	universeGenerator;
	InstChecker*	inQuantKernelChecker;

public:
	FalseQuantKernelGenerator(InstGenerator* universegenerator, InstChecker* quantchecker)
			: universeGenerator(universegenerator), inQuantKernelChecker(quantchecker){ }

	void reset(){
		universeGenerator->reset();
		if(universeGenerator->isAtEnd()){
			notifyAtEnd();
		}
	}

	void next(){
		while(not inQuantKernelChecker->check()){
			universeGenerator->next();
		}
		if(universeGenerator->isAtEnd()){
			notifyAtEnd();
		}
	}
};

#endif /* FALSEQUANTKERNELGENERATOR_HPP_ */
