/************************************
	FoBddFactory.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cassert>
#include <iostream>

#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddAtomKernel.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "utils/TheoryUtils.hpp"

using namespace std;

// TODO why clone the formula and not clone the term?
// FIXME should not CLONE in fobdd factory, because variables get copied and the bddmanager keeps an internel mapping
const FOBDD* FOBDDFactory::run(const Formula* f) {
//	Formula* cf = f->clone();
//	cf = FormulaUtils::unnestPartialTerms(cf, Context::POSITIVE);
	f->accept(this);
	//cf->recursiveDelete(); FIXME variables from the cloned cf are used in the bdd, and they are deleted when using recursive delete. What should be the solution? Use variables from f?
	return _bdd;
}

const FOBDDArgument* FOBDDFactory::run(const Term* t) {
	// FIXME: move partial functions in aggregates that occur in t
	t->accept(this);
	return _argument;
}

void FOBDDFactory::visit(const VarTerm* vt) {
	_argument = _manager->getVariable(vt->var());
}

void FOBDDFactory::visit(const DomainTerm* dt) {
	_argument = _manager->getDomainTerm(dt->sort(), dt->value());
}

void FOBDDFactory::visit(const FuncTerm* ft) {
	vector<const FOBDDArgument*> args(ft->function()->arity());
	for (size_t n = 0; n < args.size(); ++n) {
		ft->subterms()[n]->accept(this);
		args[n] = _argument;
	}
	_argument = _manager->getFuncTerm(ft->function(), args);
}

void FOBDDFactory::visit(const AggTerm*) {
	// TODO
	assert(false);
}

void FOBDDFactory::visit(const PredForm* pf) {
	vector<const FOBDDArgument*> args(pf->symbol()->nrSorts());
	for (size_t n = 0; n < args.size(); ++n) {
		pf->subterms()[n]->accept(this);
		args[n] = _argument;
	}
	AtomKernelType akt = AtomKernelType::AKT_TWOVALUED;
	bool inverse = false;
	auto symbol = pf->symbol();
	if (sametypeid<Predicate>(*symbol)) {
		auto predicate = dynamic_cast<Predicate*>(symbol);
		if (predicate->type() != ST_NONE) {
			switch (predicate->type()) {
			case ST_CF:
				akt = AtomKernelType::AKT_CF;
				break;
			case ST_CT:
				akt = AtomKernelType::AKT_CT;
				break;
			case ST_PF:
				akt = AtomKernelType::AKT_CT;
				inverse = false;
				break;
			case ST_PT:
				akt = AtomKernelType::AKT_CF;
				inverse = false;
				break;
			case ST_NONE:
				assert(false);
				break;
			}
			symbol = predicate->parent();
		}
	}
	_kernel = _manager->getAtomKernel(pf->symbol(), akt, args);
	bool invert = (not inverse && pf->sign() == SIGN::NEG) || (inverse && pf->sign() == SIGN::POS);
	if (invert) {
		_bdd = _manager->getBDD(_kernel, _manager->falsebdd(), _manager->truebdd());
	} else {
		_bdd = _manager->getBDD(_kernel, _manager->truebdd(), _manager->falsebdd());
	}
}

void FOBDDFactory::visit(const BoolForm* bf) {
	if (bf->conj()) {
		const FOBDD* temp = _manager->truebdd();
		for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
			(*it)->accept(this);
			temp = _manager->conjunction(temp, _bdd);
		}
		_bdd = temp;
	} else {
		const FOBDD* temp = _manager->falsebdd();
		for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
			(*it)->accept(this);
			temp = _manager->disjunction(temp, _bdd);
		}
		_bdd = temp;
	}
	if(isNeg(bf->sign())){
		_bdd = _manager->negation(_bdd);
	}
}

void FOBDDFactory::visit(const EquivForm* bf) {
	// TODO
	assert(false);
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
	if(isNeg(qf->sign())){
		_bdd = _manager->negation(_bdd);
	}
}

void FOBDDFactory::visit(const EqChainForm* ef) {
	assert(false);
	// FIXME cannot clone
/*	EqChainForm* efclone = ef->clone();
	Formula* f = FormulaUtils::splitComparisonChains(efclone, _vocabulary);
	f->accept(this);
	f->recursiveDelete();*/
}

void FOBDDFactory::visit(const AggForm*) {
	// TODO
	assert(false);
}
