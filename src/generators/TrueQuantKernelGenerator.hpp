/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef TRUEQUANTKERNELGENERATOR_HPP_
#define TRUEQUANTKERNELGENERATOR_HPP_

#include <set>
#include "InstGenerator.hpp"

/**
 * Generate all y such that such that ?x phi(x,y) is true.
 * (y might be input or output)
 * Given are:
 * 	* A generator for all x (INCLUDED IN UNIVERSE!)
 * 	* A generator for all output y
 * 	* A checker which returns true if phi(x,y) is true.
 */
//TODO: for the moment, the quantVarGenerator is not included.  This has as consequence that the generate might generate some things more than once.
//This might cause troubles when we are working with aggregates for example.
//However, naively running over all x and checking every time might not be the most efficient solution!
class TrueQuantKernelGenerator: public InstGenerator {
private:
	//InstGenerator* _quantVarGenerator;
	InstGenerator* _universeGenerator;
	InstChecker* _quantKernelTrueChecker;
	bool _reset;

public:
	TrueQuantKernelGenerator(/*InstGenerator* quantVarGenerator,*/ InstGenerator* universegenerator, InstChecker* bddtruechecker) :
			/*_quantVarGenerator(quantVarGenerator),*/ _universeGenerator(universegenerator), _quantKernelTrueChecker(bddtruechecker) {
	}

	// FIXME reimplemnt clone
	TrueQuantKernelGenerator* clone() const {
		throw notyetimplemented("Cloning TrueQuantKernelGenerators.");
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

		for (; not _universeGenerator->isAtEnd() && not _quantKernelTrueChecker->check() ; _universeGenerator->operator ++()) {
			//for(_quantVarGenerator->begin();not _quantVarGenerator->isAtEnd();_quantVarGenerator->operator ++()){
			//}
		}
		if (_universeGenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}
	virtual void put(std::ostream& stream) {
		pushtab();
		stream << "all true instances of: " << nt() << toString(_quantKernelTrueChecker);
		poptab();
	}

};

#endif /* TRUEQUANTKERNELGENERATOR_HPP_ */
