/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef INVUNAGENERATOR_HPP_
#define INVUNAGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "structure/Universe.hpp"

class DomElemContainer;

class InverseUNAFuncGenerator: public InstGenerator {
private:
	std::vector<const DomElemContainer*> _outvars;
	std::vector<unsigned int> _outpos;
	Universe _universe;
	const DomElemContainer* _resvar;
	bool _reset;

#ifdef DEBUG
	std::vector<const DomElemContainer*> _invars;
	std::vector<unsigned int> _inpos;
#endif

public:
	InverseUNAFuncGenerator(const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ);
	InverseUNAFuncGenerator* clone() const;
	void reset();
	void next();
};

#endif /* INVUNAGENERATOR_HPP_ */
