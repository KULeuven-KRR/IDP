/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FALSEQUANTKERNELGENERATOR_HPP_
#define FALSEQUANTKERNELGENERATOR_HPP_

#include "InstGenerator.hpp"

/**
 * Generate all x such that ?x phi(x) is false.
 * Given is a generator for the universe and a checker which returns true if phi(x) is true.
 */
class FalseQuantKernelGenerator: public InstGenerator {
private:
	InstGenerator* universeGenerator;
	InstChecker* quantKernelTrueChecker;

public:
	FalseQuantKernelGenerator(InstGenerator* universegenerator, InstChecker* bddtruechecker)
			: universeGenerator(universegenerator), quantKernelTrueChecker(bddtruechecker) {
	}

	// FIXME reimplemnt clone
	FalseQuantKernelGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	bool check() const {
		return not quantKernelTrueChecker->check();
	}

	void reset() {
		universeGenerator->begin();
		if (universeGenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}

	void next() {
		universeGenerator->operator ++();
		while (not universeGenerator->isAtEnd() && quantKernelTrueChecker->check()) {
			universeGenerator->operator ++();
		}
		if (universeGenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}
	virtual void put(std::ostream& stream){
		pushtab();
		stream << "all false instances of: " << nt() << toString(quantKernelTrueChecker);
		poptab();
	}

};

#endif /* FALSEQUANTKERNELGENERATOR_HPP_ */
