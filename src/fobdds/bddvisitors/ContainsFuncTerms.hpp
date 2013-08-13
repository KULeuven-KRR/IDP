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

#ifndef CONTAINSFUNCTERMS_HPP_
#define CONTAINSFUNCTERMS_HPP_

#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBdd.hpp"

/**
 * Checks whether the given term contains Functions
 */
class ContainsFuncTerms: public FOBDDVisitor {
private:
	bool _result;
	void visit(const FOBDDFuncTerm*) {
		_result = true;
		return;
	}

public:
	ContainsFuncTerms(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}
	template<typename Tree>
	bool check(const Tree* arg) {
		_result = false;
		arg->accept(this);
		return _result;
	}
};

#endif /* CONTAINSFUNCTERMS_HPP_ */
