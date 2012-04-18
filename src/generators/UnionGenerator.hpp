/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef UNIONQUANTKERNELGENERATOR_HPP_
#define UNIONQUANTKERNELGENERATOR_HPP_

#include "InstGenerator.hpp"

/**
 * Generate all x such that one of the subgenerators succeeds.
 */
class UnionGenerator: public InstGenerator {
private:
	std::vector<InstGenerator*> _generators;
	bool _reset;
	unsigned int _current;

public:
	UnionGenerator(std::vector<InstGenerator*>& generators)
			: _generators(generators), _reset(false), _current(0) {
	}

	// FIXME reimplemnt clone
	UnionGenerator* clone() const {
		throw notyetimplemented("Cloning UnionGenerator.");
	}

	void reset() {
		_reset = true;
		_current = 0;
	}

	void next() {
		if (_reset) {
			_reset = false;
			if (_generators.size() == 0) {
				notifyAtEnd();
				return;
			} else {
				_generators[0]->begin();
			}
		} else {
			_generators[_current]->operator ++();
		}

		while (_generators[_current]->isAtEnd()) {
			_current++;
			if (_current < _generators.size()) {
				_generators[_current]->begin();
			} else {
				notifyAtEnd();
				return;
			}
		}
	}
	virtual void put(std::ostream& stream) {
		stream << "Union generator: union of ";
		pushtab();
		stream << nt();
		bool first = true;
		for (auto it = _generators.cbegin(); it != _generators.cend(); ++it) {
			if (not first) {
				stream << nt();
			} else {
				first = false;
			}
			stream << "*" << toString(*it);
		}
		poptab();
	}

};

#endif /* UNIONQUANTKERNELGENERATOR_HPP_ */
