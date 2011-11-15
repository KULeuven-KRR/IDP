/************************************
	ContainsPartialFunctions.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BDDPARTIALCHECKER_HPP_
#define BDDPARTIALCHECKER_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"

/**
 * Checks whether the given term contains partial functions
 */
class ContainsPartialFunctions: public FOBDDVisitor {
private:
	bool _result;
	void visit(const FOBDDFuncTerm* ft) {
		if (ft->func()->partial()) {
			_result = true;
		} else {
			for (auto it = ft->args().cbegin(); it != ft->args().cend(); ++it) {
				(*it)->accept(this);
				if (_result) {
					break;
				}
			}
		}
	}

public:
	ContainsPartialFunctions(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}
	bool check(const FOBDDArgument* arg) {
		_result = false;
		arg->accept(this);
		return _result;
	}
};

#endif /* BDDPARTIALCHECKER_HPP_ */
