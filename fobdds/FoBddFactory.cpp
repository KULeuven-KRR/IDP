#include <cassert>

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
#include "theorytransformations/Utils.hpp"

using namespace std;

const FOBDD* FOBDDFactory::run(const Formula* f) {
	Formula* cf = f->clone();
	cf = FormulaUtils::unnestPartialTerms(cf, Context::POSITIVE);
	cf->accept(this);
	cf->recursiveDelete();
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
	_argument = _manager->getDomainTerm(dt->sort(),dt->value());
}

void FOBDDFactory::visit(const FuncTerm* ft) {
	vector<const FOBDDArgument*> args(ft->function()->arity());
	for(size_t n = 0; n < args.size(); ++n) {
		ft->subterms()[n]->accept(this);
		args[n] = _argument;
	}
	_argument = _manager->getFuncTerm(ft->function(),args);
}

void FOBDDFactory::visit(const AggTerm*) {
	// TODO
	assert(false);
}

void FOBDDFactory::visit(const PredForm* pf) {
	vector<const FOBDDArgument*> args(pf->symbol()->nrSorts());
	for(size_t n = 0; n < args.size(); ++n) {
		pf->subterms()[n]->accept(this);
		args[n] = _argument;
	}
	AtomKernelType akt = AtomKernelType::AKT_TWOVALUED;
	bool notinverse = true;
	PFSymbol* symbol = pf->symbol();
	if(sametypeid<Predicate>(*symbol)) {
		Predicate* predicate = dynamic_cast<Predicate*>(symbol);
		if(predicate->type() != ST_NONE) {
			switch(predicate->type()) {
				case ST_CF: akt = AtomKernelType::AKT_CF; break;
				case ST_CT: akt = AtomKernelType::AKT_CT; break;
				case ST_PF: akt = AtomKernelType::AKT_CT; notinverse = false; break;
				case ST_PT: akt = AtomKernelType::AKT_CF; notinverse = false; break;
				case ST_NONE: assert(false); break;
			}
			symbol = predicate->parent();
		}
	}
	_kernel = _manager->getAtomKernel(pf->symbol(),akt,args);
	bool invert = (notinverse && pf->sign()==SIGN::NEG) || (not notinverse && pf->sign()==SIGN::POS);
	if(invert) { _bdd = _manager->getBDD(_kernel,_manager->truebdd(),_manager->falsebdd()); }
	else { _bdd = _manager->getBDD(_kernel,_manager->falsebdd(),_manager->truebdd()); }
}

void FOBDDFactory::visit(const BoolForm* bf) {
	if(bf->conj()) {
		const FOBDD* temp = _manager->truebdd();
		for(auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
			(*it)->accept(this);
			temp = _manager->conjunction(temp,_bdd);
		}
		_bdd = temp;
	}
	else {
		const FOBDD* temp = _manager->falsebdd();
		for(auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
			(*it)->accept(this);
			temp = _manager->disjunction(temp,_bdd);
		}
		_bdd = temp;
	}
	_bdd = isPos(bf->sign())? _bdd : _manager->negation(_bdd);
}

void FOBDDFactory::visit(const QuantForm* qf) {
	qf->subformula()->accept(this);
	const FOBDD* qbdd = _bdd;
	for(auto it = qf->quantVars().cbegin(); it != qf->quantVars().cend(); ++it) {
		const FOBDDVariable* qvar = _manager->getVariable(*it);
		if(qf->isUniv()){
			qbdd = _manager->univquantify(qvar,qbdd);
		} else {
			qbdd = _manager->existsquantify(qvar,qbdd);
		}
	}
	_bdd = isPos(qf->sign()) ? qbdd : _manager->negation(qbdd);
}

void FOBDDFactory::visit(const EqChainForm* ef) {
	EqChainForm* efclone = ef->clone();
	Formula* f = FormulaUtils::splitComparisonChains(efclone,_vocabulary);
	f->accept(this);
	f->recursiveDelete();
}

void FOBDDFactory::visit(const AggForm*) {
	// TODO
	assert(false);
}
