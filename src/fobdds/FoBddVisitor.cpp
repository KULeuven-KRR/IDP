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

#include "FoBddVisitor.hpp"

#include "FoBddVisitor.hpp"
#include "FoBddManager.hpp"
#include "FoBddFuncTerm.hpp"
#include "FoBddAggTerm.hpp"
#include "FoBddSetExpr.hpp"
#include "FoBddDomainTerm.hpp"
#include "FoBddVariable.hpp"
#include "FoBddIndex.hpp"
#include "FoBddAtomKernel.hpp"
#include "FoBddQuantKernel.hpp"
#include "FoBddAggKernel.hpp"
#include "FoBddUtils.hpp"
#include "FoBdd.hpp"

using namespace std;

void FOBDDVisitor::visit(const FOBDD* bdd) {
	if (bdd != _manager->truebdd() && bdd != _manager->falsebdd()) {
		bdd->kernel()->accept(this);
		visit(bdd->truebranch());
		visit(bdd->falsebranch());
	}
}

void FOBDDVisitor::visit(const FOBDDAtomKernel* kernel) {
	for (auto it = kernel->args().cbegin(); it != kernel->args().cend(); ++it) {
		(*it)->accept(this);
	}
}

void FOBDDVisitor::visit(const FOBDDQuantKernel* kernel) {
	visit(kernel->bdd());
}

void FOBDDVisitor::visit(const FOBDDAggKernel* kernel) {
	kernel->left()->accept(this);
	kernel->right()->accept(this);
}

void FOBDDVisitor::visit(const FOBDDVariable*) {
	// do nothing
}

void FOBDDVisitor::visit(const FOBDDDeBruijnIndex*) {
	// do nothing
}

void FOBDDVisitor::visit(const FOBDDDomainTerm*) {
	// do nothing
}

void FOBDDVisitor::visit(const FOBDDFuncTerm* term) {
	for (auto it = term->args().cbegin(); it != term->args().cend(); ++it) {
		(*it)->accept(this);
	}
}
void FOBDDVisitor::visit(const FOBDDAggTerm* term) {
	term->setexpr()->accept(this);
}

void FOBDDVisitor::visit(const FOBDDEnumSetExpr* set) {
	auto subsets = set->subsets();
	for (auto subs = subsets.cbegin(); subs != subsets.cend(); ++subs) {
		(*subs)->accept(this);
	}
}

void FOBDDVisitor::visit(const FOBDDQuantSetExpr* set) {
	set->subformula()->accept(this);
	set->subterm()->accept(this);
}

const FOBDD* FOBDDVisitor::change(const FOBDD* bdd) {
	if (_manager->isTruebdd(bdd))
		return _manager->truebdd();
	else if (_manager->isFalsebdd(bdd))
		return _manager->falsebdd();
	else {
		const FOBDDKernel* nk = bdd->kernel()->acceptchange(this);
		const FOBDD* nt = change(bdd->truebranch());
		const FOBDD* nf = change(bdd->falsebranch());
		return _manager->ifthenelse(nk, nt, nf);
	}
}

const FOBDDKernel* FOBDDVisitor::change(const FOBDDAtomKernel* kernel) {
	vector<const FOBDDTerm*> nargs;
	for (auto it = kernel->args().cbegin(); it != kernel->args().cend(); ++it) {
		nargs.push_back((*it)->acceptchange(this));
	}
	return _manager->getAtomKernel(kernel->symbol(), kernel->type(), nargs);
}

const FOBDDKernel* FOBDDVisitor::change(const FOBDDQuantKernel* kernel) {
	const FOBDD* nbdd = change(kernel->bdd());
	return _manager->getQuantKernel(kernel->sort(), nbdd);
}

const FOBDDKernel* FOBDDVisitor::change(const FOBDDAggKernel* kernel) {
	const FOBDDTerm* left = kernel->left()->acceptchange(this);
	const FOBDDTerm* right = kernel->right()->acceptchange(this);
	return _manager->getAggKernel(left, kernel->comp(), right);
}

const FOBDDTerm* FOBDDVisitor::change(const FOBDDVariable* variable) {
	return variable;
}

const FOBDDTerm* FOBDDVisitor::change(const FOBDDDeBruijnIndex* index) {
	return index;
}

const FOBDDTerm* FOBDDVisitor::change(const FOBDDDomainTerm* term) {
	return term;
}

const FOBDDTerm* FOBDDVisitor::change(const FOBDDFuncTerm* term) {
	vector<const FOBDDTerm*> args;
	for (auto it = term->args().cbegin(); it != term->args().cend(); ++it) {
		args.push_back((*it)->acceptchange(this));
	}
	return _manager->getFuncTerm(term->func(), args);
}

const FOBDDTerm* FOBDDVisitor::change(const FOBDDAggTerm* term) {
	auto set = term->setexpr()->acceptchange(this);
	return _manager->getAggTerm(term->aggfunction(), set);
}

const FOBDDQuantSetExpr* FOBDDVisitor::change(const FOBDDQuantSetExpr* set) {
	auto formula = change(set->subformula());
	auto term = set->subterm()->acceptchange(this);
	return _manager->getQuantSetExpr(set->quantvarsorts(), formula, term, set->sort());
}

const FOBDDEnumSetExpr* FOBDDVisitor::change(const FOBDDEnumSetExpr* set) {
	std::vector<const FOBDDQuantSetExpr*> subsets(set->size()); //!< The direct subformulas of the set expression
	size_t i = 0;
	for (auto subs = set->subsets().cbegin(); subs != set->subsets().cend(); subs++, i++) {
		subsets[i] = change(set->subsets()[i]);
	}
	return _manager->getEnumSetExpr(subsets, set->sort());
}
