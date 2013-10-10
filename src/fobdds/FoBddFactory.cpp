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

#include <algorithm>

#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddAtomKernel.hpp"
#include "IncludeComponents.hpp"
#include "fobdds/FoBddSetExpr.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;

const FOBDD* FOBDDFactory::turnIntoBdd(const Formula* f, Structure* s) {
	auto cf = f->cloneKeepVars();
	cf = FormulaUtils::unnestPartialTerms(cf, s, _vocabulary);
	cf->accept(this);
	cf->recursiveDeleteKeepVars();
	return _bdd;
}

const FOBDDTerm* FOBDDFactory::turnIntoBdd(const Term* t) {
	// FIXME: move partial functions in aggregates that occur in t
	t->accept(this);
	return _term;
}

void FOBDDFactory::visit(const VarTerm* vt) {
	_term = _manager->getVariable(vt->var());
}

void FOBDDFactory::visit(const DomainTerm* dt) {
	_term = _manager->getDomainTerm(dt);
}

void FOBDDFactory::visit(const FuncTerm* ft) {
	vector<const FOBDDTerm*> args;
	for (auto i = ft->subterms().cbegin(); i < ft->subterms().cend(); ++i) {
		(*i)->accept(this);
		args.push_back(_term);
	}
	_term = _manager->getFuncTerm(ft->function(), args);
}

void FOBDDFactory::visit(const AggTerm* at) {
	auto function = at->function();
	at->set()->accept(this);
	_term = _manager->getAggTerm(function, _enumset);
}
void FOBDDFactory::visit(const EnumSetExpr* ese) {
	auto sets = ese->getSets();
	std::vector<const FOBDDQuantSetExpr*> bddsets;
	for (auto it = sets.cbegin(); it != sets.cend(); it++) {
		(*it)->accept(this);
		bddsets.push_back(_quantset);
	}
	_enumset = _manager->getEnumSetExpr(bddsets, ese->sort());
}
void FOBDDFactory::visit(const QuantSetExpr* se) {
	se->getCondition()->accept(this);
	auto formula = _bdd;
	se->getTerm()->accept(this);
	auto term = _term;
	std::vector<const FOBDDVariable*> variables(se->quantVars().size());
	int i = 0;
	for (auto it = se->quantVars().cbegin(); it != se->quantVars().cend(); it.operator ++(), i++) {
		variables[i] = _manager->getVariable((*it));
	}
	_quantset = _manager->setquantify(variables, formula, term, se->sort());
}
/**
 * If it is a predicate, we have to check if we are working with a bounded version of a parent predicate,
 * if so, set the relevant kerneltype and inversion.
 */
void checkIfBoundedPredicate(PFSymbol*& symbol, AtomKernelType& akt, bool& invert) {
	if (isa<Predicate>(*symbol)) {
		auto predicate = dynamic_cast<Predicate*>(symbol);
		switch (predicate->type()) {
		case ST_CF:
			akt = AtomKernelType::AKT_CF;
			break;
		case ST_CT:
			akt = AtomKernelType::AKT_CT;
			break;
		case ST_PF:
			akt = AtomKernelType::AKT_CT;
			invert = not invert;
			break;
		case ST_PT:
			akt = AtomKernelType::AKT_CF;
			invert = not invert;
			break;
		case ST_NONE:
			break;
		}
		if (predicate->type() != ST_NONE) {
			symbol = predicate->parent();
		}
	}
}

void FOBDDFactory::visit(const PredForm* pf) {
	vector<const FOBDDTerm*> args;
	for (auto i = pf->subterms().cbegin(); i < pf->subterms().cend(); ++i) {
		(*i)->accept(this);
		args.push_back(_term);
	}
	auto akt = AtomKernelType::AKT_TWOVALUED;
	auto invert = isNeg(pf->sign());
	auto symbol = pf->symbol();
	Assert(symbol != NULL);

	checkIfBoundedPredicate(symbol, akt, invert);

	_kernel = _manager->getAtomKernel(symbol, akt, args);
	if (invert) {
		_bdd = _manager->ifthenelse(_kernel, _manager->falsebdd(), _manager->truebdd());
	} else {
		_bdd = _manager->ifthenelse(_kernel, _manager->truebdd(), _manager->falsebdd());
	}
}

void FOBDDFactory::visit(const BoolForm* bf) {
	if (bf->conj()) {
		auto temp = _manager->truebdd();
		for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
			(*it)->accept(this);
			temp = _manager->conjunction(temp, _bdd);
		}
		_bdd = temp;
	} else {
		auto temp = _manager->falsebdd();
		for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
			(*it)->accept(this);
			temp = _manager->disjunction(temp, _bdd);
		}
		_bdd = temp;
	}
	if (isNeg(bf->sign())) {
		_bdd = _manager->negation(_bdd);
	}
}

void FOBDDFactory::visit(const EquivForm* ef) {
	auto left = ef->left();
	auto right = ef->right();
	left->accept(this);
	auto leftbdd = _bdd;
	right->accept(this);
	auto rightbdd = _bdd;
	auto both = _manager->conjunction(leftbdd, rightbdd);
	auto none = _manager->conjunction(_manager->negation(leftbdd), _manager->negation(rightbdd));
	_bdd = _manager->disjunction(both, none);
	if (isNeg(ef->sign())) {
		_bdd = _manager->negation(_bdd);
	}
}

void FOBDDFactory::visit(const QuantForm* qf) {
	qf->subformula()->accept(this);
	auto fobddvars = _manager->getVariables(qf->quantVars());
	if (qf->isUniv()) {
		_bdd = _manager->univquantify(fobddvars, _bdd);
	} else {
		_bdd = _manager->existsquantify(fobddvars, _bdd);
	}

	if (isNeg(qf->sign())) {
		_bdd = _manager->negation(_bdd);
	}
}

void FOBDDFactory::visit(const EqChainForm* ef) {
	Formula* efclone = ef->cloneKeepVars(); //We are not allowed to change the vars, since the manager keeps a vars->bddvars mapping.
	efclone = FormulaUtils::splitComparisonChains(efclone, _vocabulary);
	efclone->accept(this);
	efclone->recursiveDeleteKeepVars();
}

void FOBDDFactory::visit(const AggForm* af) {
#ifndef NDEBUG
	if (af->getBound()->type() != TermType::DOM && af->getBound()->type() != TermType::VAR) {
		throw notyetimplemented("Creating a bdd for nested aggregate formulas");
	}
#endif
	auto invert = isNeg(af->sign());
	af->getBound()->accept(this);
	auto left = _term;
	af->getAggTerm()->accept(this);
	_kernel = _manager->getAggKernel(left, af->comp(), _term);
	if (invert) {
		_bdd = _manager->ifthenelse(_kernel, _manager->falsebdd(), _manager->truebdd());
	} else {
		_bdd = _manager->ifthenelse(_kernel, _manager->truebdd(), _manager->falsebdd());
	}
}
