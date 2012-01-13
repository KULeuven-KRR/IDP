/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef CONTAINSTERM_HPP_
#define CONTAINSTERM_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBdd.hpp"

/**
 * Checks whether the given term contains functions
 */
class ContainsTerm: public FOBDDVisitor {
private:
	bool _result;
	const FOBDDArgument* _arg;

public:
	ContainsTerm(FOBDDManager* m)
			: FOBDDVisitor(m) {
	}

	bool contains(const FOBDDArgument* super, const FOBDDArgument* arg) {
		_result = false;
		_arg = arg;
		super->accept(this);
		return _result;
	}

private:
	void visit(const FOBDDVariable* f) {
		if (f == _arg) {
			_result = true;
		}
	}

	void visit(const FOBDDDeBruijnIndex* f) {
		if (f == _arg) {
			_result = true;
		}
	}

	void visit(const FOBDDDomainTerm* f) {
		if (f == _arg) {
			_result = true;
		}
	}

	void visit(const FOBDDFuncTerm* f) {
		if (f == _arg) {
			_result = true;
			return;
		} else {
			for (auto it = f->args().cbegin(); it != f->args().cend(); ++it) {
				(*it)->accept(this);
				if (_result) {
					return;
				}
			}
		}
	}

};

#endif /* CONTAINSTERM_HPP_ */
