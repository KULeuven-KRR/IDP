/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef FOBDDINDEX_HPP_
#define FOBDDINDEX_HPP_

#include "fobdds/FoBddTerm.hpp"

class FOBDDManager;

class FOBDDDeBruijnIndex: public FOBDDArgument {
private:
	friend class FOBDDManager;

	Sort* _sort;
	unsigned int _index;

	FOBDDDeBruijnIndex(Sort* sort, unsigned int index) :
			_sort(sort), _index(index) {
	}

public:
	bool containsDeBruijnIndex(unsigned int index) const {
		return _index == index;
	}

	Sort* sort() const {
		return _sort;
	}
	unsigned int index() const {
		return _index;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDArgument* acceptchange(FOBDDVisitor*) const;
};


#endif /* FOBDDINDEX_HPP_ */
