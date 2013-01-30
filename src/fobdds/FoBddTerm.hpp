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

#ifndef FOBDDTERM_HPP_
#define FOBDDTERM_HPP_
#include <iostream>
#include "common.hpp"
#include "Assert.hpp"
class Sort;
class FOBDDVisitor;

/**
 * Class to represents terms in BDDs.  A term can be a domainterm, function term, a variable
 * or a DeBruyn index (which is in essence a variable)
 */
class FOBDDTerm {
private:
	uint id; // For the order;

protected:
	FOBDDTerm(uint id)
			: id(id) {

	}
public:
	uint getID() const {
		return id;
	}

	virtual ~FOBDDTerm() {
	}
	virtual bool containsDeBruijnIndex(unsigned int index) const = 0;
	bool containsFreeDeBruijnIndex() const {
		return containsDeBruijnIndex(0);
	}

	virtual void accept(FOBDDVisitor*) const = 0;
	virtual const FOBDDTerm* acceptchange(FOBDDVisitor*) const = 0;

	virtual Sort* sort() const = 0;
	virtual std::ostream& put(std::ostream& output) const=0;

	bool before(const FOBDDTerm* rhs) const {
		Assert(rhs != NULL);
		return getID() < rhs->getID();
	}
};

#endif /* FOBDDTERM_HPP_ */
