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

#ifndef FOBDDATOMKERNEL_HPP_
#define FOBDDATOMKERNEL_HPP_

#include <vector>
#include "fobdds/FoBddKernel.hpp"

class PFSymbol;
class FOBDDTerm;
class FOBDDManager;

class FOBDDAtomKernel: public FOBDDKernel {
private:
	friend class FOBDDManager;

	PFSymbol* _symbol;
	AtomKernelType _type;
	std::vector<const FOBDDTerm*> _args;

	FOBDDAtomKernel(PFSymbol* symbol, AtomKernelType akt, const std::vector<const FOBDDTerm*>& args, const KernelOrder& order)
			: FOBDDKernel(order), _symbol(symbol), _type(akt), _args(args) {
	}

public:
	bool containsDeBruijnIndex(unsigned int index) const;

	PFSymbol* symbol() const {
		return _symbol;
	}
	AtomKernelType type() const {
		return _type;
	}
	const FOBDDTerm* args(unsigned int n) const {
		Assert( n<_args.size());
		return _args[n];
	}

	const std::vector<const FOBDDTerm*>& args() const {
		return _args;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDKernel* acceptchange(FOBDDVisitor*) const;

	virtual std::ostream& put(std::ostream& output) const;

};

#endif /* FOBDDATOMKERNEL_HPP_ */
