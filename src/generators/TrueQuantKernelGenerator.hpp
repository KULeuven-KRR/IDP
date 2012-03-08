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
#include "structure/structure.hpp"

/**
 * Generate all y such that such that ?x phi(x,y) is true.
 * (y might be input or output)
 * Given are:
 * 	* A generator which returns all tuples (x,y) such that phi(x,y) is true.
 * He keeps a set of all y he has already generated; to avoid doubles...
 */

class TrueQuantKernelGenerator: public InstGenerator {
private:
	InstGenerator* _subBddTrueGenerator;
	set<ElementTuple, Compare<ElementTuple> > _alreadySeen;
	vector<const DomElemContainer*> _outvars;
	bool _reset;

	ElementTuple getDomainElements() {
		auto tuple = vector<const DomainElement*>(_outvars.size());
		for (int i = 0; i < _outvars.size(); ++i) {
			tuple[i] = _outvars[i]->get();
		}
		return tuple;
	}

public:
	TrueQuantKernelGenerator(InstGenerator* subBddTrueGenerator, vector<const DomElemContainer*> outvars)
			: _subBddTrueGenerator(subBddTrueGenerator), _outvars(outvars), _alreadySeen() {
	}

	// FIXME reimplemnt clone
	TrueQuantKernelGenerator* clone() const {
		throw notyetimplemented("Cloning TrueQuantKernelGenerators.");
	}

	void reset() {
		_reset = true;
		_alreadySeen.clear();
		_subBddTrueGenerator->begin();
		if (_subBddTrueGenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}

	void next() {
		if (_reset) {
			_reset = false;
		} else {
			_subBddTrueGenerator->operator ++();
		}
		for (; not _subBddTrueGenerator->isAtEnd(); _subBddTrueGenerator->operator ++()) {
			auto elements = getDomainElements();
			if (_alreadySeen.find(elements) == _alreadySeen.cend()) {
				_alreadySeen.insert(elements);
				return;
			}
		}
		if (_subBddTrueGenerator->isAtEnd()) {
			notifyAtEnd();
		}
	}
	virtual void put(std::ostream& stream) {
		pushtab();
		stream << "all true instances of: " << nt() << toString(_subBddTrueGenerator);
		poptab();
	}

};

#endif /* TRUEQUANTKERNELGENERATOR_HPP_ */
