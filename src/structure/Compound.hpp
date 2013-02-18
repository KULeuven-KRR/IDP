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

#ifndef COMPOUND_HPP_
#define COMPOUND_HPP_

class Function;

#include "DomainElement.hpp"

/**
 *	The value of a domain element that consists of a function applied to domain elements.
 */
class Compound {
private:
	Function* _function;
	ElementTuple _arguments;

	Compound(Function* function, const ElementTuple& arguments);

public:
	~Compound();

	Function* function() const; //!< Returns the function of the compound
	const DomainElement* arg(unsigned int n) const; //!< Returns the n'th argument of the compound
	const ElementTuple& args() const {
		return _arguments;
	}

	std::ostream& put(std::ostream&) const;

	friend class DomainElementFactory;
};

#endif /* COMPOUND_HPP_ */
