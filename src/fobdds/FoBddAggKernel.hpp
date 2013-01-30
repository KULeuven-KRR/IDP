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

#ifndef FOBDDAGGKERNEL_HPP_
#define FOBDDAGGKERNEL_HPP_

#include <vector>
#include "fobdds/FoBddKernel.hpp"

class PFSymbol;
class FOBDDTerm;
class FOBDDManager;
class FOBDDSetExpr;
class FOBDDAggTerm;

class FOBDDAggKernel: public FOBDDKernel {
private:
	friend class FOBDDManager;

	const FOBDDTerm* _left;
	CompType _comp;
	const FOBDDAggTerm* _right;

	FOBDDAggKernel(const FOBDDTerm* left, CompType comp, const FOBDDAggTerm* right, const KernelOrder& order)
			: FOBDDKernel(order), _left(left), _comp(comp), _right(right) {
	}
public:
	bool containsDeBruijnIndex(unsigned int index) const;

	const FOBDDTerm* left() const {
		return _left;
	}
	CompType comp() const {
		return _comp;
	}
	const FOBDDAggTerm* right() const {
		return _right;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDKernel* acceptchange(FOBDDVisitor*) const;

	virtual std::ostream& put(std::ostream& output) const;

};

#endif /* FOBDDAGGKERNEL_HPP_ */
