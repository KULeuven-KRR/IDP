/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FOBDDDOMAINTERM_HPP_
#define FOBDDDOMAINTERM_HPP_

#include "fobdds/FoBddTerm.hpp"

class DomainElement;
class FOBDDManager;

class FOBDDDomainTerm: public FOBDDTerm {
private:
	friend class FOBDDManager;

	Sort* _sort;
	const DomainElement* _value;

	FOBDDDomainTerm(Sort* sort, const DomainElement* value)
			: _sort(sort), _value(value) {
	}

public:
	bool containsDeBruijnIndex(unsigned int) const {
		return false;
	}

	Sort* sort() const {
		return _sort;
	}
	const DomainElement* value() const {
		return _value;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDTerm* acceptchange(FOBDDVisitor*) const;
	virtual std::ostream& put(std::ostream& output) const;

};

const FOBDDDomainTerm* add(FOBDDManager* manager, const FOBDDDomainTerm* d1, const FOBDDDomainTerm* d2);

#endif /* FOBDDDOMAINTERM_HPP_ */
