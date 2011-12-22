/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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
const FOBDD* FOBDDFactory::turnIntoBdd(const Formula* f) {
	auto cf = f->cloneKeepVars();
	cf = FormulaUtils::unnestPartialTerms(cf, Context::POSITIVE);
	f->accept(this);
	//cf->recursiveDelete(); FIXME variables from the cloned cf are used in the bdd, and they are deleted when using recursive delete. What should be the solution? Use variables from f?
	return _bdd;
}

const FOBDDArgument* FOBDDFactory::turnIntoBdd(const Term* t) {
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
	vector<const FOBDDArgument*> args;
	for (auto i = ft->subterms().cbegin(); i < ft->subterms().cend(); ++i) {
		(*i)->accept(this);
		args.push_back(_argument);
	}
	_argument = _manager->getFuncTerm(ft->function(), args);
}

void FOBDDFactory::visit(const AggTerm*) {
	throw notyetimplemented("Creating a bdd for aggregate terms has not yet been implemented.");
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
			invert = true;
			break;
		case ST_PT:
			akt = AtomKernelType::AKT_CF;
			invert = true;
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
	vector<const FOBDDArgument*> args;
	for (auto i = pf->subterms().cbegin(); i < pf->subterms().cend(); ++i) {
		(*i)->accept(this);
		args.push_back(_argument);
	}
	auto akt = AtomKernelType::AKT_TWOVALUED;
	auto invert = isNeg(pf->sign());
	auto symbol = pf->symbol();

	checkIfBoundedPredicate(symbol, akt, invert);

	_kernel = _manager->getAtomKernel(symbol, akt, args);
	if (invert) {
		_bdd = _manager->getBDD(_kernel, _manager->falsebdd(), _manager->truebdd());
	} else {
		_bdd = _manager->getBDD(_kernel, _manager->truebdd(), _manager->falsebdd());
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

void FOBDDFactory::visit(const EquivForm*) {
	throw notyetimplemented("Creating a bdd for equivalences has not yet been implemented.");
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
	auto efclone = ef->clone();
	auto f = FormulaUtils::splitComparisonChains(efclone, _vocabulary);
	f->accept(this);
	// f->recursiveDelete(); TODO deletes variables also!
}

void FOBDDFactory::visit(const AggForm*) {
	throw notyetimplemented("Creating a bdd for aggregate formulas has not yet been implemented.");
}
