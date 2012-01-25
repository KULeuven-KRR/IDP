/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "FoBddVisitor.hpp"

#include "FoBddVisitor.hpp"
#include "FoBddManager.hpp"
#include "FoBddFuncTerm.hpp"
#include "FoBddDomainTerm.hpp"
#include "FoBddVariable.hpp"
#include "FoBddIndex.hpp"
#include "FoBddAtomKernel.hpp"
#include "FoBddQuantKernel.hpp"
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
	vector<const FOBDDArgument*> nargs;
	for (auto it = kernel->args().cbegin(); it != kernel->args().cend(); ++it) {
		nargs.push_back((*it)->acceptchange(this));
	}
	return _manager->getAtomKernel(kernel->symbol(), kernel->type(), nargs);
}

const FOBDDKernel* FOBDDVisitor::change(const FOBDDQuantKernel* kernel) {
	const FOBDD* nbdd = change(kernel->bdd());
	return _manager->getQuantKernel(kernel->sort(), nbdd);
}

const FOBDDArgument* FOBDDVisitor::change(const FOBDDVariable* variable) {
	return variable;
}

const FOBDDArgument* FOBDDVisitor::change(const FOBDDDeBruijnIndex* index) {
	return index;
}

const FOBDDArgument* FOBDDVisitor::change(const FOBDDDomainTerm* term) {
	return term;
}

const FOBDDArgument* FOBDDVisitor::change(const FOBDDFuncTerm* term) {
	vector<const FOBDDArgument*> args;
	for (auto it = term->args().cbegin(); it != term->args().cend(); ++it) {
		args.push_back((*it)->acceptchange(this));
	}
	return _manager->getFuncTerm(term->func(), args);
}
