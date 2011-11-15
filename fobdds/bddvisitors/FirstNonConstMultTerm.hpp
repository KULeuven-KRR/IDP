/************************************
	FirstNonConstMultTerm.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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
	const FOBDDArgument* _result;
public:
	FirstNonConstMultTerm() :
			FOBDDVisitor(NULL) {
	}

	const FOBDDArgument* run(const FOBDDArgument* arg) {
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
