/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
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
#include "theory/TheoryUtils.hpp"

using namespace std;

// TODO why clone the formula and not clone the term?
const FOBDD* FOBDDFactory::turnIntoBdd(const Formula* f) {
	auto cf = f->cloneKeepVars();
	cf = FormulaUtils::unnestPartialTerms(cf, Context::POSITIVE);
	cf->accept(this);
	//cf->recursiveDelete();
	//FIXME variables from the cloned cf are used in the bdd, and they are deleted when using recursive delete. What should be the solution? Use variables from f?
	//Possible solution: cf->recursiveDeleteKeepVars();
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
	_term = _manager->getAggTerm(function, _set);
}
void FOBDDFactory::visit(const EnumSetExpr* se) {
	int size = se->subformulas().size();
	Assert(size == se->subterms().size());
	std::vector<const FOBDD*> subforms(size);
	std::vector<const FOBDDTerm*> subterms(size);
	for (int i = 0; i < size; i++) {
		se->subformulas()[i]->accept(this);
		subforms[i] = _bdd;
		se->subterms()[i]->accept(this);
		subterms[i] = _term;
	}
	_set = _manager->getEnumSetExpr(subforms, subterms, se->sort());
}
void FOBDDFactory::visit(const QuantSetExpr* se) {
	se->subformulas()[0]->accept(this);
	auto formula = _bdd;
	se->subterms()[0]->accept(this);
	auto term = _term;
	std::vector<const FOBDDVariable*> variables(se->quantVars().size());
	int i = 0;
	for (auto it = se->quantVars().cbegin(); it != se->quantVars().cend(); it.operator ++(), i++) {
		variables[i] = _manager->getVariable((*it));
	}
	_set = _manager->getQuantSetExpr(variables, formula, term, se->sort());
}
/**
 * If it is a predicate, we have to check if we are working with a bounded version of a parent predicate,
 * if so, set the relevant kerneltype and inversion.
 */
void checkIfBoundedPredicate(PFSymbol*& symbol, AtomKernelType& akt, bool& invert) {
	if (sametypeid<Predicate>(*symbol)) {
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
	for (auto it = qf->quantVars().cbegin(); it != qf->quantVars().cend(); ++it) {
		const FOBDDVariable* qvar = _manager->getVariable(*it);
		if (qf->isUniv()) {
			_bdd = _manager->univquantify(qvar, _bdd);
		} else {
			_bdd = _manager->existsquantify(qvar, _bdd);
		}
	}
	if (isNeg(qf->sign())) {
		_bdd = _manager->negation(_bdd);
	}
}

void FOBDDFactory::visit(const EqChainForm* ef) {
	auto efclone = ef->cloneKeepVars(); //We are not allowed to change the vars, since the manager keeps a vars->bddvars mapping.
	auto f = FormulaUtils::splitComparisonChains(efclone, _vocabulary);
	f->accept(this);
	// f->recursiveDelete(); TODO deletes variables also!
}

void FOBDDFactory::visit(const AggForm* af) {
#ifndef NDEBUG
	if (af->left()->type() != TermType::TT_DOM && af->left()->type() != TermType::TT_VAR) {
		throw notyetimplemented("Creating a bdd for unnested aggregate formulas has not yet been implemented.");
	}
#endif
	auto invert = isNeg(af->sign());
	af->left()->accept(this);
	auto left = _term;
	af->right()->accept(this);
	_kernel = _manager->getAggKernel(left, af->comp(), _term);
	if (invert) {
		_bdd = _manager->ifthenelse(_kernel, _manager->falsebdd(), _manager->truebdd());
	} else {
		_bdd = _manager->ifthenelse(_kernel, _manager->truebdd(), _manager->falsebdd());
	}
}
