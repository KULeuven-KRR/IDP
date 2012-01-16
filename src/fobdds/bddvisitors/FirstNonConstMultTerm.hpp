/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef NONCONSTTERMEXTRACTOR_HPP_
#define NONCONSTTERMEXTRACTOR_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddVariable.hpp"

#include "vocabulary.hpp"

/**
 * Return first term which is not a multiplication with a const left hand side.
 */
class FirstNonConstMultTerm: public FOBDDVisitor {
private:
	const FOBDDTerm* _result;
public:
	FirstNonConstMultTerm()
			: FOBDDVisitor(NULL) {
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
	void visit(const FOBDDDeBruijnIndex* dt) {
		_result = dt;
	}
	void visit(const FOBDDFuncTerm* ft) {
		if (isMultiplication(ft) && sametypeid<FOBDDDomainTerm>(*(ft->args(0)))) {
			ft->args(1)->accept(this);
		} else {
			_result = ft;
		}
	}
};

#endif /* NONCONSTTERMEXTRACTOR_HPP_ */
