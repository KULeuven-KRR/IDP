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

#ifndef SIMPLEFUNCGENERATOR_HPP_
#define SIMPLEFUNCGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "structure/Universe.hpp"

class FuncTable;
class DomElemContainer;
class DomainElement;

/**
 * Generator for a function when the co-domain is output and there is an input/output pattern for the domain.
 */
class SimpleFuncGenerator: public InstGenerator {
private:
	bool _reset;

	const FuncTable* _functable;
	InstGenerator* _univgen;
	Universe _universe;

	ElementTuple _currenttuple;

	std::vector<const DomElemContainer*> _vars; // Argument variables
	const DomElemContainer* _rangevar;			// Range variables

	std::vector<const DomElemContainer*> _outvars;  // Output variables
	std::vector<unsigned int> _outpos;				// Position of the associated domain elements in _currenttuple
	std::vector<const DomElemContainer*> _invars;	// Input variables
	std::vector<unsigned int> _inpos;				// Position of the associated domain elements in _currenttuple

public:
	// NOTE: DOES NOT take ownership of table
	SimpleFuncGenerator(const FuncTable* ft, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ,
			const std::vector<unsigned int>& firstocc);
	~SimpleFuncGenerator();
	SimpleFuncGenerator* clone() const;
	void internalSetVarsAgain();
	void reset();
	void next();
	virtual void put(std::ostream& stream) const;
};

#endif /* SIMPLEFUNCGENERATOR_HPP_ */
