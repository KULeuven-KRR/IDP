/************************************
	CollectSameOperationTerms.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FLATMULT_HPP_
#define FLATMULT_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

/**
 *	Return a list of all terms which are currently in one long multiplication.
 *	@post: the list is in left-to-right order! (depth first visitation)
 *	If the list might only contain one elment which is not a domainelement, the neutral element is added at the start.
 */
template<typename Operation>
class CollectSameOperationTerms: public FOBDDVisitor {
private:
	std::vector<const FOBDDArgument*> _result;

	void avoidempty() {
		if (_result.empty()) {
			_result.push_back(_manager->getDomainTerm(VocabularyUtils::natsort(), Operation::getNeutralElement()));
		}
	}
	void visit(const FOBDDDomainTerm* dt) {
		_result.push_back(dt);
	}
	void visit(const FOBDDVariable* v) {
		avoidempty();
		_result.push_back(v);
	}
	void visit(const FOBDDDeBruijnIndex* i) {
		avoidempty();
		_result.push_back(i);
	}
	void visit(const FOBDDFuncTerm* ft) {
		if (ft->func()->name()==Operation::getFuncName()) {
			ft->args(0)->accept(this);
			_result.push_back(ft->args(1));
		} else {
			avoidempty();
			_result.push_back(ft);
		}
	}

public:
	CollectSameOperationTerms(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}

	std::vector<const FOBDDArgument*> getTerms(const FOBDDArgument* arg) {
		_result.clear();
		arg->accept(this);
		return _result;
	}
};

#endif /* FLATMULT_HPP_ */
