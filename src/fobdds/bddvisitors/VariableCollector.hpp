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

#ifndef VARIABLECOLLECTOR_HPP_
#define VARIABLECOLLECTOR_HPP_

#include <set>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddKernel.hpp"

/**
 * Class to obtain all variables of a bdd.
 * NOTE: this does not collect indices.
 */

class VariableCollector: public FOBDDVisitor {
private:
	std::set<const FOBDDVariable*, CompareBDDVars> _result;
public:
	VariableCollector(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}
	void visit(const FOBDDVariable* v) {
		_result.insert(v);
	}

	template<typename Node>
	const std::set<const FOBDDVariable*, CompareBDDVars>& getVariables(const Node* arg) {
		_result.clear();
		arg->accept(this);
		return _result;
	}
};

#endif /* VARIABLECOLLECTOR_HPP_ */
