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

#ifndef TERMEXTRACTOR_HPP_
#define TERMEXTRACTOR_HPP_

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
 * Collects all subterms of a given term which are reachable from root only by functerms of the provided type.
 */
class TermCollector: public FOBDDVisitor {
private:
	std::vector<const FOBDDTerm*> _terms;
	std::string _funcname;
public:
	TermCollector(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m), _funcname("") {
	}

	const std::vector<const FOBDDTerm*>& getTerms(const FOBDDTerm* arg, const std::string& funcname) {
		_funcname = funcname;
		_terms.clear();
		arg->accept(this);
		return _terms;
	}

	void visit(const FOBDDDomainTerm* domterm) {
		_terms.push_back(domterm);
	}

	void visit(const FOBDDAggTerm* aggterm) {
		_terms.push_back(aggterm);
	}

	void visit(const FOBDDDeBruijnIndex* dbrterm) {
		_terms.push_back(dbrterm);
	}

	void visit(const FOBDDVariable* varterm) {
		_terms.push_back(varterm);
	}

	void visit(const FOBDDFuncTerm* functerm) {
		if (functerm->func()->name() == _funcname) {
			for (auto i = functerm->args().cbegin(); i < functerm->args().cend(); ++i) {
				(*i)->accept(this);
			}
		} else {
			_terms.push_back(functerm);
		}
	}
};

#endif /* TERMEXTRACTOR_HPP_ */
