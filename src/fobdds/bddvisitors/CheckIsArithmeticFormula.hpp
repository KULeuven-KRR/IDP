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

#ifndef ARITHCHECKER_HPP_
#define ARITHCHECKER_HPP_

#include "IncludeComponents.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

/**
 * Checks whether the bdd contains no quantkernels, that each symbol is part of the std vocabulary and that all terms, variables and indices
 * are subsorts of floats.
 *
 * TODO: seems like a hack to check that the bdd represents an arithmetic formula
 */
class CheckIsArithmeticFormula: public FOBDDVisitor {
private:
	bool _result;

public:
	CheckIsArithmeticFormula(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}

	template<typename BddNode>
	bool isArithmeticFormula(const BddNode* k) {
		_result = true;
		k->accept(this);
		return _result;
	}

private:
	void visit(const FOBDDAtomKernel* kernel) {
		for (auto it = kernel->args().cbegin(); it != kernel->args().cend() && _result; ++it) {
			(*it)->accept(this);
		}
		_result = _result && Vocabulary::std()->contains(kernel->symbol());
	}

	void visit(const FOBDDQuantKernel*) {
		_result = false;
	}

	void visit(const FOBDDAggKernel*) {
		//TODO : is this right: is an AGGKERNEL an arithmetic formula???
		return; //If result was true, it still is true.
	}

	void visit(const FOBDDVariable* variable) {
		_result = _result && SortUtils::isSubsort(variable->sort(), get(STDSORT::FLOATSORT));
	}

	void visit(const FOBDDDeBruijnIndex* index) {
		_result = _result && SortUtils::isSubsort(index->sort(), get(STDSORT::FLOATSORT));
	}

	void visit(const FOBDDDomainTerm* domainterm) {
		_result = _result && SortUtils::isSubsort(domainterm->sort(), get(STDSORT::FLOATSORT));
	}

	void visit(const FOBDDFuncTerm* functerm) {
		for (auto it = functerm->args().cbegin(); it != functerm->args().cend() && _result; ++it) {
			(*it)->accept(this);
		}
		_result = _result && Vocabulary::std()->contains(functerm->func());
	}
	void visit(const FOBDDAggTerm*) {
		//TODO: right?
		return;
	}

};

#endif /* ARITHCHECKER_HPP_ */
