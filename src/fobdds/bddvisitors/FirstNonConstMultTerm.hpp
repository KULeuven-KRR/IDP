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

#pragma once

#include "IncludeComponents.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddAggTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddVariable.hpp"

/**
 * Return first term which is not a multiplication with a const left hand side.
 */
class FirstNonConstMultTerm: public FOBDDVisitor {
private:
	const FOBDDTerm* _result;
public:
	FirstNonConstMultTerm(shared_ptr<FOBDDManager> manager)
			: FOBDDVisitor(manager), _result(NULL) {
	}

	const FOBDDTerm* run(const FOBDDTerm* arg) {
		_result = 0;
		arg->accept(this);
		return _result;
	}

	void visit(const FOBDDDomainTerm* dt) {
		_result = dt;
	}
	void visit(const FOBDDVariable* vt) {
		_result = vt;
	}
	void visit(const FOBDDDeBruijnIndex* i) {
		_result = i;
	}
	void visit(const FOBDDAggTerm* at) {
		_result = at;
	}
	void visit(const FOBDDFuncTerm* ft) {
		if (isMultiplication(ft) && isa<FOBDDDomainTerm>(*(ft->args(0)))) {
			ft->args(1)->accept(this);
		} else {
			_result = ft;
		}
	}
};
