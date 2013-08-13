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

#ifndef CONSTTERMEXTRACTOR_HPP_
#define CONSTTERMEXTRACTOR_HPP_

#include "IncludeComponents.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddVariable.hpp"

/**
 * Get the const term of the first func term with a const left hand side.
 *
 * TODO should only be called on certain parse trees, document this!
 */
class FirstConstMultTerm: public FOBDDVisitor {
private:
	const FOBDDDomainTerm* _result;
public:
	FirstConstMultTerm(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}
	const FOBDDDomainTerm* run(const FOBDDTerm* arg) {
		_result = _manager->getDomainTerm(arg->sort(), createDomElem(1));
		arg->accept(this);
		return _result;
	}
	void visit(const FOBDDFuncTerm* ft) {
		if (isBddDomainTerm(ft->args(0))) {
			Assert(not isBddDomainTerm(ft->args(1)));
			_result = castBddDomainTerm(ft->args(0));
		}
	}
};

#endif /* CONSTTERMEXTRACTOR_HPP_ */
