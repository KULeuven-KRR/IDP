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

#ifndef COPY_HPP_
#define COPY_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddAggKernel.hpp"

class Copy: public FOBDDVisitor {
private:
	std::shared_ptr<FOBDDManager> _originalmanager;
	std::shared_ptr<FOBDDManager> _copymanager;
	const FOBDDKernel* _kernel;
	const FOBDDTerm* _argument;
	const FOBDDEnumSetExpr* _enumset;
	const FOBDDQuantSetExpr* _quantset;
public:
	Copy(std::shared_ptr<FOBDDManager> orig, std::shared_ptr<FOBDDManager> copy)
			: FOBDDVisitor(orig), _originalmanager(orig), _copymanager(copy) {
	}

	void visit(const FOBDDVariable* var) {
		_argument = _copymanager->getVariable(var->variable());
	}

	void visit(const FOBDDDeBruijnIndex* index) {
		_argument = _copymanager->getDeBruijnIndex(index->sort(), index->index());
	}

	void visit(const FOBDDDomainTerm* term) {
		_argument = _copymanager->getDomainTerm(term->sort(), term->value());
	}

	void visit(const FOBDDFuncTerm* term) {
		std::vector<const FOBDDTerm*> newargs;
		for (auto it = term->args().cbegin(); it != term->args().cend(); ++it) {
			newargs.push_back(copy(*it));
		}
		_argument = _copymanager->getFuncTerm(term->func(), newargs);
	}

	void visit(const FOBDDAggTerm* term) {
		auto newset = copy(term->setexpr());
		_argument = _copymanager->getAggTerm(term->aggfunction(), newset);
	}

	void visit(const FOBDDEnumSetExpr* set) {
		std::vector<const FOBDDQuantSetExpr*> subsets;
		for (int i = 0; i < set->size(); i++) {
			subsets.push_back(copy(set->subsets()[i]));
		}
		_enumset = _copymanager->getEnumSetExpr(subsets, set->sort());
	}

	void visit(const FOBDDQuantSetExpr* set) {
		auto subformula = copy(set->subformula());
		auto term = copy(set->subterm());
		_quantset = _copymanager->getQuantSetExpr(set->quantvarsorts(),subformula,term,set->sort());
	}

	void visit(const FOBDDQuantKernel* kernel) {
		auto newbdd = copy(kernel->bdd());
		_kernel = _copymanager->getQuantKernel(kernel->sort(), newbdd);
	}

	void visit(const FOBDDAggKernel* kernel) {
		auto newleft = copy(kernel->left());
		auto newright = copy(kernel->right());
		_kernel = _copymanager->getAggKernel(newleft, kernel->comp(), newright);
	}

	void visit(const FOBDDAtomKernel* kernel) {
		std::vector<const FOBDDTerm*> newargs;
		for (auto it = kernel->args().cbegin(); it != kernel->args().cend(); ++it) {
			newargs.push_back(copy(*it));
		}
		_kernel = _copymanager->getAtomKernel(kernel->symbol(), kernel->type(), newargs);
	}

	const FOBDDTerm* copy(const FOBDDTerm* arg) {
		arg->accept(this);
		return _argument;
	}

	const FOBDDKernel* copy(const FOBDDKernel* kernel) {
		kernel->accept(this);
		return _kernel;
	}

	const FOBDD* copy(const FOBDD* bdd) {
		if (bdd == _originalmanager->truebdd()) {
			return _copymanager->truebdd();
		} else if (bdd == _originalmanager->falsebdd()) {
			return _copymanager->falsebdd();
		} else {
			auto falsebranch = copy(bdd->falsebranch());
			auto truebranch = copy(bdd->truebranch());
			auto kernel = copy(bdd->kernel());
			return _copymanager->ifthenelse(kernel, truebranch, falsebranch);
		}
	}
	const FOBDD* copyTryMaintainOrder(const FOBDD* bdd) {
			if (bdd == _originalmanager->truebdd()) {
				return _copymanager->truebdd();
			} else if (bdd == _originalmanager->falsebdd()) {
				return _copymanager->falsebdd();
			} else {
				auto falsebranch = copyTryMaintainOrder(bdd->falsebranch());
				auto truebranch = copyTryMaintainOrder(bdd->truebranch());
				auto kernel = copy(bdd->kernel());
				return _copymanager->ifthenelseTryMaintainOrder(kernel, truebranch, falsebranch);
			}
		}

	const FOBDDEnumSetExpr* copy(const FOBDDEnumSetExpr* set) {
		set->accept(this);
		return _enumset;
	}

	const FOBDDQuantSetExpr* copy(const FOBDDQuantSetExpr* set) {
			set->accept(this);
			return _quantset;
		}
};

#endif /* COPY_HPP_ */
