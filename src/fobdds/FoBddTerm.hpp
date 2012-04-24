/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef FOBDDTERM_HPP_
#define FOBDDTERM_HPP_
#include <iostream>
class Sort;
class FOBDDVisitor;

/**
 * Class to represents terms in BDDs.  A term can be a domainterm, function term, a variable
 * or a DeBruyn index (which is in essence a variable)
 */
class FOBDDTerm {
public:
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

};

#endif /* FOBDDTERM_HPP_ */
