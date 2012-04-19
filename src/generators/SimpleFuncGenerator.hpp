/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef SIMPLEFUNCGENERATOR_HPP_
#define SIMPLEFUNCGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "structure/Universe.hpp"

class FuncTable;
class DomElemContainer;
class DomainElement;

/**
 * Generator for a function when the range is output and there is an input/output pattern for the domain.
 */
class SimpleFuncGenerator: public InstGenerator {
private:
	const FuncTable* _functable;
	InstGenerator* _univgen;
	std::vector<const DomElemContainer*> _outvars;
	std::vector<unsigned int> _outpos;

	const DomElemContainer* _rangevar;
	bool _reset;

	std::vector<const DomElemContainer*> _vars;
	ElementTuple _currenttuple;

	std::vector<const DomElemContainer*> _invars;
	std::vector<unsigned int> _inpos;

	Universe _universe;

public:
	// NOTE: DOES NOT take ownership of table
	SimpleFuncGenerator(const FuncTable* ft, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ,
			const std::vector<unsigned int>& firstocc);
	~SimpleFuncGenerator();
	SimpleFuncGenerator* clone() const;
	void setVarsAgain();
	void reset();
	void next();
	virtual void put(std::ostream& stream);};

#endif /* SIMPLEFUNCGENERATOR_HPP_ */
