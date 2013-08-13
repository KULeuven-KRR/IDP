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
		if (ft->func()->partial() || is(ft->func(), STDFUNC::DIVISION) || is(ft->func(), STDFUNC::MODULO)) {
			//TODO: division and modulo should be partial...
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
	ContainsPartialFunctions(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}
	bool check(const FOBDDTerm* arg) {
		_result = false;
		arg->accept(this);
		return _result;
	}
};

#endif /* BDDPARTIALCHECKER_HPP_ */
