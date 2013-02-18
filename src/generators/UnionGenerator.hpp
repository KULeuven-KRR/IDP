/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef UNIONQUANTKERNELGENERATOR_HPP_
#define UNIONQUANTKERNELGENERATOR_HPP_

#include "InstGenerator.hpp"

/**
 * Generate all x such that one of the subgenerators succeeds.
 * The checkers should satisfy the condition that they check iff the corresponding generator would generate the current tuple
 * (thus, they are the same as the corresponding generator, but with all input variables)
 */
class UnionGenerator: public InstGenerator {
private:
	std::vector<InstGenerator*> _generators;
	std::vector<InstChecker*> _checkers;
	bool _reset;
	unsigned int _current;

public:
	UnionGenerator(const std::vector<InstGenerator*>& generators, const std::vector<InstChecker*>& checkers);
	~UnionGenerator() {
		for (auto it = _generators.cbegin(); it != _generators.cend(); ++it) {
			delete (*it);
		}
		for (auto it = _checkers.cbegin(); it != _checkers.cend(); ++it) {
			delete (*it);
		}
	}
	UnionGenerator* clone() const;
	void reset();
	bool alreadySeen();
	void next();
	virtual void put(std::ostream& stream) const;

	virtual void internalSetVarsAgain();
};

#endif /* UNIONQUANTKERNELGENERATOR_HPP_ */
