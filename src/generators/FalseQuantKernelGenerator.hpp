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
//TODO: might be seen as a special case of a twochildgenerator...
class FalseQuantKernelGenerator: public InstGenerator {
private:
	InstGenerator* _universeGenerator;
	InstChecker* _quantKernelTrueChecker;
	bool _reset;

public:
	FalseQuantKernelGenerator(InstGenerator* universegenerator, InstChecker* bddtruechecker) :
			_universeGenerator(universegenerator), _quantKernelTrueChecker(bddtruechecker) {
	}

	// FIXME reimplemnt clone
	FalseQuantKernelGenerator* clone() const {
		throw notyetimplemented("Cloning FalseQuantKernelGenerators.");
	}

	void reset() {
		_reset = true;
		_universeGenerator->begin();
		if (_universeGenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}

	void next() {
		if (_reset) {
			_reset = false;
		} else {
			_universeGenerator->operator ++();
		}

		for (; not _universeGenerator->isAtEnd() && _quantKernelTrueChecker->check(); _universeGenerator->operator ++()) {
		}
		if (_universeGenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}
	virtual void put(std::ostream& stream) {
		pushtab();
		stream << "all false instances of: " << nt() << toString(_quantKernelTrueChecker);
		poptab();
	}

};

#endif /* FALSEQUANTKERNELGENERATOR_HPP_ */
