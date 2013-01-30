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

#pragma once

#include "InstGenerator.hpp"
#include "structure/MainStructureComponents.hpp"

class DomElemContainer;

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
	std::set<ElementTuple, Compare<ElementTuple> > _alreadySeen;
	std::vector<const DomElemContainer*> _outvars;
	bool _reset;

	ElementTuple getDomainElements();

public:
	TrueQuantKernelGenerator(InstGenerator* subBddTrueGenerator, std::vector<const DomElemContainer*> outvars);
	TrueQuantKernelGenerator* clone() const;
	void internalSetVarsAgain();
	void reset();
	void next();
	virtual void put(std::ostream& stream) const;

};
