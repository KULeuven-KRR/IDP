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

#ifndef FOBDDFUNCTERM_HPP_
#define FOBDDFUNCTERM_HPP_

#include <vector>
#include "fobdds/FoBddTerm.hpp"

class Function;

class FOBDDFuncTerm: public FOBDDTerm {
private:
	friend class FOBDDManager;

	Function* _function;
	std::vector<const FOBDDTerm*> _args;

	FOBDDFuncTerm(uint id, Function* func, const std::vector<const FOBDDTerm*>& args)
			: FOBDDTerm(id), _function(func), _args(args) {
	}

public:
	bool containsDeBruijnIndex(unsigned int index) const;

	Function* func() const {
		return _function;
	}
	const FOBDDTerm* args(unsigned int n) const {
		return _args[n];
	}
	const std::vector<const FOBDDTerm*>& args() const {
		return _args;
	}
	Sort* sort() const;

	void accept(FOBDDVisitor*) const;
	const FOBDDTerm* acceptchange(FOBDDVisitor*) const;

	virtual std::ostream& put(std::ostream& output) const;

};

#endif /* FOBDDFUNCTERM_HPP_ */
