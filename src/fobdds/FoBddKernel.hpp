/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FOBDDKERNEL_HPP_
#define FOBDDKERNEL_HPP_

#include "fobdds/FoBddUtils.hpp"
#include <utility> // for relational operators (namespace rel_ops)
using namespace std;
using namespace rel_ops;

class FOBDDVisitor;

class FOBDDKernel {
private:
	KernelOrder _order;
public:
	FOBDDKernel(const KernelOrder& order)
			: _order(order) {
	}
	virtual ~FOBDDKernel() {
	}

	bool containsFreeDeBruijnIndex() const {
		return containsDeBruijnIndex(0);
	}
	virtual bool containsDeBruijnIndex(unsigned int) const {
		return false;
	}
	unsigned int category() const {
		return _order._category;
	}
	unsigned int number() const {
		return _order._number;
	}

	void replacenumber(unsigned int n) {
		_order._number = n;
	}

	bool operator<(const FOBDDKernel& k) const {
		return(_order < k._order);
	}

	virtual void accept(FOBDDVisitor*) const {
	}
	virtual const FOBDDKernel* acceptchange(FOBDDVisitor*) const {
		return this;
	}
};

#endif /* FOBDDKERNEL_HPP_ */
