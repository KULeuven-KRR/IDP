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
	void reset();
	void next();
	virtual void put(std::ostream& stream) const;

};

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
	FalseQuantKernelGenerator(InstGenerator* universegenerator, InstChecker* bddtruechecker);
	FalseQuantKernelGenerator* clone() const;
	void reset();
	void next();
	virtual void put(std::ostream& stream) const;

};

#endif /* TRUEQUANTKERNELGENERATOR_HPP_ */
