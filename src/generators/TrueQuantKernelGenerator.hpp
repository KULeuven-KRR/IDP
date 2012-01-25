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
 * Generate all x such that ?x phi(x) is true.
 * Given is a generator which returns tuples for which phi(x) is false.
 */
class TrueQuantKernelGenerator: public InstGenerator {
private:
	InstGenerator* _quantgenerator;
public:
	TrueQuantKernelGenerator(InstGenerator* gen)
			: _quantgenerator(gen) {
	}

	// FIXME reimplement (deep clone)
	TrueQuantKernelGenerator* clone() const {
		return new TrueQuantKernelGenerator(*this);
	}

	bool check() const {
		return not _quantgenerator->check();
	}

	void reset() {
		_quantgenerator->begin();
		if (_quantgenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}

	void next() {
		while (not _quantgenerator->isAtEnd()) {
			_quantgenerator->operator ++();
		}
		if (_quantgenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}

	virtual void put(std::ostream& stream) {
		stream << tabs() << "generate: TrueQuantKernelGenerator = " << toString(_quantgenerator) << "\n";
	}
};

#endif /* TRUEQUANTKERNELGENERATOR_HPP_ */
