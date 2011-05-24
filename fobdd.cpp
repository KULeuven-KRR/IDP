/************************************
	fobdd.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cassert>
#include <typeinfo>
#include <cmath>
#include <sstream>
#include "fobdd.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "common.hpp"
#include "structure.hpp"


using namespace std;

/*******************
	Kernel order
*******************/

static unsigned int STANDARDCATEGORY = 1;
static unsigned int DEBRUIJNCATEGORY = 2;
static unsigned int TRUEFALSECATEGORY = 3;

bool FOBDDKernel::operator<(const FOBDDKernel& k) const {
	if(_order._category < k._order._category) return true;
	else if(_order._category > k._order._category) return false;
	else return _order._number < k._order._number;
}

bool FOBDDKernel::operator>(const FOBDDKernel& k) const {
	if(_order._category > k._order._category) return true;
	else if(_order._category < k._order._category) return false;
	else return _order._number > k._order._number;
}

KernelOrder FOBDDManager::newOrder(unsigned int category) {
	KernelOrder order(category,_nextorder[category]);
	++_nextorder[category];
	return order;
}

KernelOrder FOBDDManager::newOrder(const vector<FOBDDArgument*>& args) {
	unsigned int category = STANDARDCATEGORY;
	for(unsigned int n = 0; n < args.size(); ++n) {
		if(args[n]->containsFreeDeBruijnIndex()) {
			category = DEBRUIJNCATEGORY;
			break;
		}
	}
	return newOrder(category);
}

KernelOrder FOBDDManager::newOrder(FOBDD* bdd) {
	unsigned int category = (bdd->containsFreeDeBruijnIndex()) ? DEBRUIJNCATEGORY : STANDARDCATEGORY;
	return newOrder(category);
}

/************************
	De Bruijn indices
************************/

bool FOBDDFuncTerm::containsDeBruijnIndex(unsigned int index) const {
	for(unsigned int n = 0; n < _args.size(); ++n) {
		if(_args[n]->containsDeBruijnIndex(index)) return true;
	}
	return false;
}

bool FOBDDAtomKernel::containsDeBruijnIndex(unsigned int index) const {
	for(unsigned int n = 0; n < _args.size(); ++n) {
		if(_args[n]->containsDeBruijnIndex(index)) return true;
	}
	return false;
}

bool FOBDDQuantKernel::containsDeBruijnIndex(unsigned int index) const {
	return _bdd->containsDeBruijnIndex(index+1);
}

bool FOBDD::containsDeBruijnIndex(unsigned int index) const {
	if(_kernel->containsDeBruijnIndex(index)) return true;
	else if(_falsebranch && _falsebranch->containsDeBruijnIndex(index)) return true;
	else if(_truebranch && _truebranch->containsDeBruijnIndex(index)) return true;
	else return false;
}

FOBDD* FOBDDManager::bump(FOBDDVariable* var, FOBDD* bdd, unsigned int depth) {
	if(bdd == _truebdd || bdd == _falsebdd) return bdd;

	FOBDDKernel* newkernel = bump(var,bdd->kernel(),depth);
	FOBDD* newfalse = bump(var,bdd->falsebranch(),depth);
	FOBDD* newtrue = bump(var,bdd->truebranch(),depth);

	return ifthenelse(newkernel,newtrue,newfalse);
}

FOBDDKernel* FOBDDManager::bump(FOBDDVariable* var, FOBDDKernel* kernel, unsigned int depth) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		FOBDDAtomKernel* atomkernel = dynamic_cast<FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		vector<FOBDDArgument*> newargs(symbol->nrSorts());
		for(unsigned int n = 0; n < symbol->nrSorts(); ++n) 
			newargs[n] = bump(var,atomkernel->args(n),depth);
		return getAtomKernel(symbol,atomkernel->type(),newargs);
	}
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		FOBDDQuantKernel* quantkernel = dynamic_cast<FOBDDQuantKernel*>(kernel);
		FOBDD* newbdd = bump(var,quantkernel->bdd(),depth+1);
		return getQuantKernel(quantkernel->sort(),newbdd);
	}
}

FOBDDArgument* FOBDDManager::bump(FOBDDVariable* var, FOBDDArgument* arg, unsigned int depth) {
	if(arg == var) return getDeBruijnIndex(var->variable()->sort(),depth);
	else if(typeid(*arg) == typeid(FOBDDDeBruijnIndex)) {
		FOBDDDeBruijnIndex* dbr = dynamic_cast<FOBDDDeBruijnIndex*>(arg);
		if(depth <= dbr->index()) return getDeBruijnIndex(dbr->sort(),dbr->index()+1);
	}
	else if(typeid(*arg) == typeid(FOBDDFuncTerm)) {
		FOBDDFuncTerm* ft = dynamic_cast<FOBDDFuncTerm*>(arg);
		Function* func = ft->func();
		vector<FOBDDArgument*> newargs(func->arity());
		for(unsigned int n = 0; n < func->arity(); ++n) 
			newargs[n] = bump(var,ft->args(n),depth);
		return getFuncTerm(func,newargs);
	}

	return arg;
}

/******************
	BDD manager
******************/

FOBDDManager::FOBDDManager() {

	_nextorder[TRUEFALSECATEGORY] = 0;
	_nextorder[STANDARDCATEGORY] = 0;
	_nextorder[DEBRUIJNCATEGORY] = 0;

	KernelOrder ktrue = newOrder(TRUEFALSECATEGORY);
	KernelOrder kfalse = newOrder(TRUEFALSECATEGORY);
	FOBDDKernel* truekernel = new FOBDDKernel(ktrue);
	FOBDDKernel* falsekernel = new FOBDDKernel(kfalse);
	_truebdd = new FOBDD(truekernel,0,0);
	_falsebdd = new FOBDD(falsekernel,0,0);

}

FOBDD* FOBDDManager::getBDD(FOBDDKernel* kernel,FOBDD* truebranch,FOBDD* falsebranch) {
	// Simplification
	// TODO: extra tests (see old grounder)
	if(falsebranch == truebranch) return falsebranch;

	// Lookup
	BDDTable::const_iterator it = _bddtable.find(kernel);
	if(it != _bddtable.end()) {
		MBDDMBDDBDD::const_iterator jt = it->second.find(falsebranch);
		if(jt != it->second.end()) {
			MBDDBDD::const_iterator kt = jt->second.find(truebranch);
			if(kt != jt->second.end()) {
				return kt->second;
			}
		}
	}
	
	// Lookup failed, create a new bdd
	return addBDD(kernel,truebranch,falsebranch);

}

FOBDD* FOBDDManager::addBDD(FOBDDKernel* kernel,FOBDD* truebranch,FOBDD* falsebranch) {
	FOBDD* newbdd = new FOBDD(kernel,truebranch,falsebranch);
	_bddtable[kernel][falsebranch][truebranch] = newbdd;
	return newbdd;
}

FOBDDAtomKernel* FOBDDManager::getAtomKernel(PFSymbol* symbol,AtomKernelType akt, const vector<FOBDDArgument*>& args) {
	// TODO: simplification

	// Lookup
	AtomKernelTable::const_iterator it = _atomkerneltable.find(symbol);
	if(it != _atomkerneltable.end()) {
		MAKTMVAGAK::const_iterator jt = it->second.find(akt);
		if(jt != it->second.end()) {
			MVAGAK::const_iterator kt = jt->second.find(args);
			if(kt != jt->second.end())
				return kt->second;
		}
	}

	// Lookup failed, create a new atom kernel
	return addAtomKernel(symbol,akt,args);
}

FOBDDAtomKernel* FOBDDManager::addAtomKernel(PFSymbol* symbol,AtomKernelType akt, const vector<FOBDDArgument*>& args) {
	FOBDDAtomKernel* newkernel = new FOBDDAtomKernel(symbol,akt,args,newOrder(args));
	_atomkerneltable[symbol][akt][args] = newkernel;
	return newkernel;
}

FOBDDQuantKernel* FOBDDManager::getQuantKernel(Sort* sort,FOBDD* bdd) {
	// TODO: simplification

	// Lookup
	QuantKernelTable::const_iterator it = _quantkerneltable.find(sort);
	if(it != _quantkerneltable.end()) {
		MBDDQK::const_iterator jt = it->second.find(bdd);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}

	// Lookup failed, create a new quantified kernel
	return addQuantKernel(sort,bdd);
}

FOBDDQuantKernel* FOBDDManager::addQuantKernel(Sort* sort,FOBDD* bdd) {
	FOBDDQuantKernel* newkernel = new FOBDDQuantKernel(sort,bdd,newOrder(bdd));
	_quantkerneltable[sort][bdd] = newkernel;
	return newkernel;
}

FOBDDVariable* FOBDDManager::getVariable(Variable* var) {
	// Lookup
	VariableTable::const_iterator it = _variabletable.find(var);
	if(it != _variabletable.end()) {
		return it->second;
	}

	// Lookup failed, create a new variable
	return addVariable(var);
}

set<FOBDDVariable*> FOBDDManager::getVariables(const set<Variable*>& vars) {
	set<FOBDDVariable*> bddvars;
	for(set<Variable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
		bddvars.insert(getVariable(*it));
	}
	return bddvars;
}

FOBDDVariable* FOBDDManager::addVariable(Variable* var) {
	FOBDDVariable* newvariable = new FOBDDVariable(var);
	_variabletable[var] = newvariable;
	return newvariable;
}

FOBDDDeBruijnIndex* FOBDDManager::getDeBruijnIndex(Sort* sort, unsigned int index) {
	// Lookup
	DeBruijnIndexTable::const_iterator it = _debruijntable.find(sort);
	if(it != _debruijntable.end()) {
		MUIDB::const_iterator jt = it->second.find(index);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}
	// Lookup failed, create a new De Bruijn index
	return addDeBruijnIndex(sort,index);
}

FOBDDDeBruijnIndex* FOBDDManager::addDeBruijnIndex(Sort* sort, unsigned int index) {
	FOBDDDeBruijnIndex* newindex = new FOBDDDeBruijnIndex(sort,index);
	_debruijntable[sort][index] = newindex;
	return newindex;
}

FOBDDFuncTerm* FOBDDManager::getFuncTerm(Function* func, const vector<FOBDDArgument*>& args) {
	// TODO Simplification
	
	// Lookup
	FuncTermTable::const_iterator it = _functermtable.find(func);
	if(it != _functermtable.end()) {
		MVAFT::const_iterator jt = it->second.find(args);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}
	// Lookup failed, create a new funcion term
	return addFuncTerm(func,args);
}

FOBDDFuncTerm* FOBDDManager::addFuncTerm(Function* func, const vector<FOBDDArgument*>& args) {
	FOBDDFuncTerm* newarg = new FOBDDFuncTerm(func,args);
	_functermtable[func][args] = newarg;
	return newarg;
}

FOBDDDomainTerm* FOBDDManager::getDomainTerm(Sort* sort, const DomainElement* value) {
	// Lookup
	DomainTermTable::const_iterator it = _domaintermtable.find(sort);
	if(it != _domaintermtable.end()) {
		MTEDT::const_iterator jt = it->second.find(value);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}
	// Lookup failed, create a new funcion term
	return addDomainTerm(sort,value);
}

FOBDDDomainTerm* FOBDDManager::addDomainTerm(Sort* sort, const DomainElement* value) {
	FOBDDDomainTerm* newdt = new FOBDDDomainTerm(sort,value);
	_domaintermtable[sort][value] = newdt;
	return newdt;
}

/*************************
	Logical operations
*************************/

FOBDD* FOBDDManager::negation(FOBDD* bdd) {
	// TODO dynamic programming
	
	// Base cases
	if(bdd == _truebdd) return _falsebdd;
	if(bdd == _falsebdd) return _truebdd;

	// Recursive case
	FOBDD* falsebranch = negation(bdd->falsebranch());
	FOBDD* truebranch = negation(bdd->truebranch());
	return getBDD(bdd->kernel(),truebranch,falsebranch);
}

FOBDD* FOBDDManager::conjunction(FOBDD* bdd1, FOBDD* bdd2) {
	// TODO dynamic programming
	
	// Base cases
	if(bdd1 == _falsebdd || bdd2 == _falsebdd) return _falsebdd;
	if(bdd1 == _truebdd) return bdd2;
	if(bdd2 == _truebdd) return bdd1;
	if(bdd1 == bdd2) return bdd1;

	// Recursive case
	if(*(bdd1->kernel()) < *(bdd2->kernel())) {
		FOBDD* falsebranch = conjunction(bdd1->falsebranch(),bdd2);
		FOBDD* truebranch = conjunction(bdd1->truebranch(),bdd2);
		return getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
	else if(*(bdd1->kernel()) > *(bdd2->kernel())) {
		FOBDD* falsebranch = conjunction(bdd1,bdd2->falsebranch());
		FOBDD* truebranch = conjunction(bdd1,bdd2->truebranch());
		return getBDD(bdd2->kernel(),truebranch,falsebranch);
	}
	else {
		assert(bdd1->kernel() == bdd2->kernel());
		FOBDD* falsebranch = conjunction(bdd1->falsebranch(),bdd2->falsebranch());
		FOBDD* truebranch = conjunction(bdd1->truebranch(),bdd2->truebranch());
		return getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
}

FOBDD* FOBDDManager::disjunction(FOBDD* bdd1, FOBDD* bdd2) {
	// TODO dynamic programming
	
	// Base cases
	if(bdd1 == _truebdd || bdd2 == _truebdd) return _truebdd;
	if(bdd1 == _falsebdd) return bdd2;
	if(bdd2 == _falsebdd) return bdd1;
	if(bdd1 == bdd2) return bdd1;

	// Recursive case
	if(*(bdd1->kernel()) < *(bdd2->kernel())) {
		FOBDD* falsebranch = disjunction(bdd1->falsebranch(),bdd2);
		FOBDD* truebranch = disjunction(bdd1->truebranch(),bdd2);
		return getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
	else if(*(bdd1->kernel()) > *(bdd2->kernel())) {
		FOBDD* falsebranch = disjunction(bdd1,bdd2->falsebranch());
		FOBDD* truebranch = disjunction(bdd1,bdd2->truebranch());
		return getBDD(bdd2->kernel(),truebranch,falsebranch);
	}
	else {
		assert(bdd1->kernel() == bdd2->kernel());
		FOBDD* falsebranch = disjunction(bdd1->falsebranch(),bdd2->falsebranch());
		FOBDD* truebranch = disjunction(bdd1->truebranch(),bdd2->truebranch());
		return getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
}

FOBDD* FOBDDManager::ifthenelse(FOBDDKernel* kernel, FOBDD* truebranch, FOBDD* falsebranch) {
	// TODO dynamic programming
	FOBDDKernel* truekernel = truebranch->kernel();
	FOBDDKernel* falsekernel = falsebranch->kernel();

	if(*kernel < *truekernel) {
		if(*kernel < *falsekernel) {
			return getBDD(kernel,truebranch,falsebranch);
		}
		else if(kernel == falsekernel) {
			return getBDD(kernel,truebranch,falsebranch->falsebranch());
		}
		else {
			assert(*kernel > *falsekernel);
			FOBDD* newtrue = ifthenelse(kernel,truebranch,falsebranch->truebranch());
			FOBDD* newfalse = ifthenelse(kernel,truebranch,falsebranch->falsebranch());
			return getBDD(falsekernel,newtrue,newfalse);
		}
	}
	else if(kernel == truekernel) {
		if(*kernel < *falsekernel) {
			return getBDD(kernel,truebranch->truebranch(),falsebranch);
		}
		else if(kernel == falsekernel) {
			return getBDD(kernel,truebranch->truebranch(),falsebranch->falsebranch());
		}
		else {
			assert(*kernel > *falsekernel);
			FOBDD* newtrue = ifthenelse(kernel,truebranch,falsebranch->truebranch());
			FOBDD* newfalse = ifthenelse(kernel,truebranch,falsebranch->falsebranch());
			return getBDD(falsekernel,newtrue,newfalse);
		}
	}
	else {
		assert(*kernel > *truekernel);
		if(*kernel < *falsekernel || kernel == falsekernel || *truekernel < *falsekernel) {
			FOBDD* newtrue = ifthenelse(kernel,truebranch->truebranch(),falsebranch);
			FOBDD* newfalse = ifthenelse(kernel,truebranch->falsebranch(),falsebranch);
			return getBDD(truekernel,newtrue,newfalse);
		}
		else if(truekernel == falsekernel) {
			FOBDD* newtrue = ifthenelse(kernel,truebranch->truebranch(),falsebranch->truebranch());
			FOBDD* newfalse = ifthenelse(kernel,truebranch->falsebranch(),falsebranch->falsebranch());
			return getBDD(truekernel,newtrue,newfalse);
		}
		else {
			assert(*falsekernel < *truekernel);
			FOBDD* newtrue = ifthenelse(kernel,truebranch,falsebranch->truebranch());
			FOBDD* newfalse = ifthenelse(kernel,truebranch,falsebranch->falsebranch());
			return getBDD(falsekernel,newtrue,newfalse);
		}
	}
}

FOBDD* FOBDDManager::univquantify(FOBDDVariable* var, FOBDD* bdd) {
	// TODO dynamic programming
	FOBDD* negatedbdd = negation(bdd);
	FOBDD* quantbdd = existsquantify(var,negatedbdd);
	return negation(quantbdd);
}

FOBDD* FOBDDManager::univquantify(const set<FOBDDVariable*>& qvars, FOBDD* bdd) {
	FOBDD* negatedbdd = negation(bdd);
	FOBDD* quantbdd = existsquantify(qvars,negatedbdd);
	return negation(quantbdd);
}

FOBDD* FOBDDManager::existsquantify(FOBDDVariable* var, FOBDD* bdd) {
	// TODO dynamic programming
	FOBDD* bumped = bump(var,bdd);
	return quantify(var->variable()->sort(),bumped);
}

FOBDD* FOBDDManager::existsquantify(const set<FOBDDVariable*>& qvars, FOBDD* bdd) {
	FOBDD* result = bdd;
	for(set<FOBDDVariable*>::const_iterator it = qvars.begin(); it != qvars.end(); ++it) {
		result = existsquantify(*it,result);
	}
	return result;
}

FOBDD* FOBDDManager::quantify(Sort* sort, FOBDD* bdd) {
	// base case
	if(bdd == _truebdd || bdd == _falsebdd) {
		// FIXME take empty sorts into account!
		return bdd;
	}
	
	// Recursive case
	if(bdd->kernel()->category() == STANDARDCATEGORY) {
		FOBDD* newfalse = quantify(sort,bdd->falsebranch());
		FOBDD* newtrue = quantify(sort,bdd->truebranch());
		FOBDD* result = ifthenelse(bdd->kernel(),newtrue,newfalse);
		return result;
	}
	else {
		FOBDDQuantKernel* kernel = getQuantKernel(sort,bdd);
		return getBDD(kernel,_truebdd,_falsebdd);
	}
}

FOBDD* FOBDDManager::substitute(FOBDD* bdd,const map<FOBDDVariable*,FOBDDVariable*>& mvv) {
	if(bdd == _truebdd || bdd == _falsebdd) return bdd;
	else {
		FOBDD* newfalse = substitute(bdd->falsebranch(),mvv);
		FOBDD* newtrue = substitute(bdd->truebranch(),mvv);
		FOBDDKernel* newkernel = substitute(bdd->kernel(),mvv);
		FOBDD* result = ifthenelse(newkernel,newtrue,newfalse);
		return result;
	}
}

FOBDDKernel* FOBDDManager::substitute(FOBDDKernel* kernel,const map<FOBDDVariable*,FOBDDVariable*>& mvv) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		FOBDDAtomKernel* atomkernel = dynamic_cast<FOBDDAtomKernel*>(kernel);
		vector<FOBDDArgument*> newargs;
		for(unsigned int n = 0; n < atomkernel->symbol()->nrSorts(); ++n) {
			FOBDDArgument* newarg = substitute(atomkernel->args(n),mvv);
			newargs.push_back(newarg);
		}
		return getAtomKernel(atomkernel->symbol(),atomkernel->type(),newargs);
	}
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		FOBDDQuantKernel* quantkernel = dynamic_cast<FOBDDQuantKernel*>(kernel);
		FOBDD* newbdd = substitute(quantkernel->bdd(),mvv);
		return getQuantKernel(quantkernel->sort(),newbdd);
	}
}

FOBDDArgument* FOBDDManager::substitute(FOBDDArgument* arg,const map<FOBDDVariable*,FOBDDVariable*>& mvv) {
	if(typeid(*arg) == typeid(FOBDDVariable)) {
		FOBDDVariable* variable = dynamic_cast<FOBDDVariable*>(arg);
		map<FOBDDVariable*,FOBDDVariable*>::const_iterator it = mvv.find(variable);
		if(it != mvv.end()) return it->second;
		else return arg;
	}
	else if(typeid(*arg) == typeid(FOBDDFuncTerm)) {
		FOBDDFuncTerm* functerm = dynamic_cast<FOBDDFuncTerm*>(arg);
		vector<FOBDDArgument*> newargs;
		for(unsigned int n = 0; n < functerm->func()->arity(); ++n) {
			FOBDDArgument* newarg = substitute(functerm->args(n),mvv);
			newargs.push_back(newarg);
		}
		return getFuncTerm(functerm->func(),newargs);
	}
	else return arg;
}

int FOBDDManager::longestbranch(FOBDDKernel* kernel) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) return 1;
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		FOBDDQuantKernel* qk = dynamic_cast<FOBDDQuantKernel*>(kernel);
		return longestbranch(qk->bdd()) + 1;
	}
}

int FOBDDManager::longestbranch(FOBDD* bdd) {
	if(bdd == _truebdd || bdd == _falsebdd) return 1;
	else {
		int kernellength = longestbranch(bdd->kernel());
		int truelength = longestbranch(bdd->truebranch()) + kernellength;
		int falselength = longestbranch(bdd->falsebranch()) + kernellength;
		return (truelength > falselength ? truelength : falselength);
	}
}

/********************
	FOBDD Factory
********************/

void FOBDDFactory::visit(const VarTerm* vt) {
	_argument = _manager->getVariable(vt->var());
}

void FOBDDFactory::visit(const DomainTerm* dt) {
	_argument = _manager->getDomainTerm(dt->sort(),dt->value());
}

void FOBDDFactory::visit(const FuncTerm* ft) {
	vector<FOBDDArgument*> args(ft->function()->arity());
	for(unsigned int n = 0; n < args.size(); ++n) {
		ft->subterms()[n]->accept(this);
		args[n] = _argument;
	}
	_argument = _manager->getFuncTerm(ft->function(),args);
}

void FOBDDFactory::visit(const AggTerm* ) {
	// TODO
	assert(false);
}

void FOBDDFactory::visit(const PredForm* pf) {
	vector<FOBDDArgument*> args(pf->symbol()->nrSorts());
	for(unsigned int n = 0; n < args.size(); ++n) {
		pf->subterms()[n]->accept(this);
		args[n] = _argument;
	}
	_kernel = _manager->getAtomKernel(pf->symbol(),AKT_TWOVAL,args);
	if(pf->sign()) _bdd = _manager->getBDD(_kernel,_manager->truebdd(),_manager->falsebdd());
	else  _bdd = _manager->getBDD(_kernel,_manager->falsebdd(),_manager->truebdd());
}

void FOBDDFactory::visit(const BoolForm* bf) {
	if(bf->conj()) {
		FOBDD* temp = _manager->truebdd();
		for(vector<Formula*>::const_iterator it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
			(*it)->accept(this);
			temp = _manager->conjunction(temp,_bdd);
		}
		_bdd = temp;
	}
	else {
		FOBDD* temp = _manager->falsebdd();
		for(vector<Formula*>::const_iterator it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
			(*it)->accept(this);
			temp = _manager->disjunction(temp,_bdd);
		}
		_bdd = temp;
	}
	_bdd = bf->sign() ? _bdd : _manager->negation(_bdd);
}

void FOBDDFactory::visit(const QuantForm* qf) {
	qf->subf()->accept(this);
	FOBDD* qbdd = _bdd;
	for(set<Variable*>::const_iterator it = qf->quantvars().begin(); it != qf->quantvars().end(); ++it) {
		FOBDDVariable* qvar = _manager->getVariable(*it);
		if(qf->univ()) qbdd = _manager->univquantify(qvar,qbdd);
		else qbdd = _manager->existsquantify(qvar,qbdd);
	}
	_bdd = qf->sign() ? qbdd : _manager->negation(qbdd);
}

void FOBDDFactory::visit(const EqChainForm* ef) {
	EqChainForm* efclone = ef->clone();
	Formula* f = FormulaUtils::remove_eqchains(efclone,_vocabulary);
	f->accept(this);
	f->recursiveDelete();
}

void FOBDDFactory::visit(const AggForm* ) {
	// TODO
	assert(false);
}


/****************
	Debugging
****************/

string FOBDDManager::to_string(FOBDD* bdd, unsigned int spaces) const {
	stringstream sstr;
	printtabs(sstr,spaces);
	string str = sstr.str();
	if(bdd == _truebdd) str += "true\n";
	else if(bdd == _falsebdd) str += "false\n";
	else {
		str += to_string(bdd->kernel(),spaces);
		str += sstr.str() + string("FALSE BRANCH:\n");
		str += to_string(bdd->falsebranch(),spaces+3);
		str += sstr.str() + string("TRUE BRANCH:\n");
		str += to_string(bdd->truebranch(),spaces+3);
	}
	return str;
}

string FOBDDManager::to_string(FOBDDKernel* kernel, unsigned int spaces) const {
	stringstream sstr;
	printtabs(sstr,spaces);
	string str = sstr.str();
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		FOBDDAtomKernel* atomkernel = dynamic_cast<FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		str += symbol->to_string();
		if(atomkernel->type() == AKT_CF) str += "<cf>";
		else if(atomkernel->type() == AKT_CT) str += "<ct>";
		if(typeid(*symbol) == typeid(Predicate)) {
			if(symbol->nrSorts()) {
				str += "(" + to_string(atomkernel->args(0));
				for(unsigned int n = 1; n < symbol->nrSorts(); ++n) 
					str += "," + to_string(atomkernel->args(n));
				str += ")";
			}
		}
		else {
			if(symbol->nrSorts() > 1) {
				str += "(" + to_string(atomkernel->args(0));
				for(unsigned int n = 1; n < symbol->nrSorts()-1; ++n) 
					str += "," + to_string(atomkernel->args(n));
				str += ")";
			}
			str += " = " + to_string(atomkernel->args(symbol->nrSorts()-1));
		}
	}
	else if(typeid(*kernel) == typeid(FOBDDQuantKernel)) {
		FOBDDQuantKernel* quantkernel = dynamic_cast<FOBDDQuantKernel*>(kernel);
		str += "EXISTS(" + quantkernel->sort()->to_string() + ") {\n";
		str += to_string(quantkernel->bdd(),spaces+3);
		str += sstr.str() + "}";
	}
	else {
		assert(false);
	}
	return str + '\n';
}

string FOBDDManager::to_string(FOBDDArgument* arg) const {
	string str = "";
	if(typeid(*arg) == typeid(FOBDDVariable)) {
		FOBDDVariable* var = dynamic_cast<FOBDDVariable*>(arg);
		str += var->variable()->to_string();
	}
	else if(typeid(*arg) == typeid(FOBDDDeBruijnIndex)) {
		FOBDDDeBruijnIndex* dbr = dynamic_cast<FOBDDDeBruijnIndex*>(arg);
		str += "<" + itos(dbr->index()) + ">[" + dbr->sort()->to_string() + "]";
	}
	else if(typeid(*arg) == typeid(FOBDDFuncTerm)) {
		FOBDDFuncTerm* ft = dynamic_cast<FOBDDFuncTerm*>(arg);
		Function* f = ft->func();
		str += f->to_string();
		if(f->arity()) {
			str += "(" + to_string(ft->args(0));
			for(unsigned int n = 1; n < f->arity(); ++n) {
				str += "," + to_string(ft->args(n));
			}
			str += ")";
		}
	}
	else if(typeid(*arg) == typeid(FOBDDDomainTerm)) {
		FOBDDDomainTerm* dt = dynamic_cast<FOBDDDomainTerm*>(arg);
		str += dt->value()->to_string() + "[" + dt->sort()->to_string() + "]";
	}
	else {
		assert(false);
	}
	return str;
}

/*****************
	Estimators
*****************/

bool FOBDDManager::contains(FOBDD* bdd, FOBDDVariable* v) {
	if(bdd == _truebdd || bdd == _falsebdd) return false;
	else {
		if(contains(bdd->kernel(),v)) return true;
		else if(contains(bdd->truebranch(),v)) return true;
		else if(contains(bdd->falsebranch(),v)) return true;
		else return false;
	}
}

bool FOBDDManager::contains(FOBDDKernel* kernel, FOBDDVariable* v) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		FOBDDAtomKernel* atomkernel = dynamic_cast<FOBDDAtomKernel*>(kernel);
		for(unsigned int n = 0; n < atomkernel->symbol()->sorts().size(); ++n) {
			if(contains(atomkernel->args(n),v)) return true;
		}
		return false;
	}
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		FOBDDQuantKernel* quantkernel = dynamic_cast<FOBDDQuantKernel*>(kernel);
		return contains(quantkernel->bdd(),v);
	}
}

bool FOBDDManager::contains(FOBDDArgument* arg, FOBDDVariable* v) {
	if(typeid(*arg) == typeid(FOBDDVariable)) return arg == v;
	else if(typeid(*arg) == typeid(FOBDDFuncTerm)) {
		FOBDDFuncTerm* farg = dynamic_cast<FOBDDFuncTerm*>(arg);
		for(unsigned int n = 0; n < farg->func()->arity(); ++n) {
			if(contains(farg->args(n),v)) return true;
		}
		return false;
	}
	else return false;
}

bool FOBDDManager::contains(FOBDDKernel* kernel, Variable* v) {
	FOBDDVariable* var = getVariable(v);
	return contains(kernel,var);
}

int univNrAnswers(const set<Variable*>& vars, const set<FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	int maxint = numeric_limits<int>::max();
	vector<SortTable*> vst; 
	for(set<Variable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) 
		vst.push_back(structure->inter((*it)->sort()));
	for(set<FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it)
		vst.push_back(structure->inter((*it)->sort()));
	Universe univ(vst);
	tablesize univsize = univ.size();
	return (univsize.first ? univsize.second : maxint);
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | kernel } in the given structure 
 */
double FOBDDManager::estimatedNrAnswers(FOBDDKernel* kernel, const set<Variable*>& vars, const set<FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	int maxint = numeric_limits<int>::max();
	double maxdouble = numeric_limits<double>::max();
	double chance = 1;

	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		FOBDDAtomKernel* atomkernel = dynamic_cast<FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		PredInter* pinter;
		if(typeid(*symbol) == typeid(Predicate)) pinter = structure->inter(dynamic_cast<Predicate*>(symbol));
		else pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphinter();
		const PredTable* pt = atomkernel->type() == AKT_CF ? pinter->cf() : pinter->ct();
		tablesize symbolsize = pt->size();
		tablesize univsize = pt->universe().size();
		if(symbolsize.first) {
			if(univsize.first && univsize.second != 0) {
				chance = double(symbolsize.second) / double(univsize.second);
			}
			else chance = 0;
		}
		if(chance > 1) chance = 1;
	}
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		FOBDDQuantKernel* quantkernel = dynamic_cast<FOBDDQuantKernel*>(kernel);
		set<FOBDDDeBruijnIndex*> kernindices;
		kernindices.insert(getDeBruijnIndex(quantkernel->sort(),0));
		for(set<FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it) 
			kernindices.insert(getDeBruijnIndex((*it)->sort(),(*it)->index()+1));
		double quantans = estimatedNrAnswers(quantkernel->bdd(),vars,kernindices,structure);
		int univquantans = univNrAnswers(vars,kernindices,structure);
		double invquantans = 0;
		if(quantans < maxdouble) {
			if(univquantans == maxint) invquantans = maxdouble;
			else invquantans = double(univquantans) - double(quantans);
		}
		tablesize quantvarsize = structure->inter(quantkernel->sort())->size();
		if(quantvarsize.first) {
			double varsize = double(quantvarsize.second);
			if(varsize > invquantans + 1) chance = 1;
			else {
				double invchance = 1;
				for(double m = 0; m < varsize; ++m) {
					invchance = invchance * invquantans / univquantans;
					invquantans = invquantans - 1;
					univquantans = univquantans - 1;
				}
				chance = 1 - invchance;
			}
		}
		else {
			if(invquantans < univquantans) chance = 1;
			else chance = 0;
		}
	}

	int univsize = univNrAnswers(vars,indices,structure);
	if(univsize == maxint) return chance > 0 ? maxdouble : 0;
	else return double(univsize) * chance;
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | bdd } in the given structure 
 */
double FOBDDManager::estimatedNrAnswers(FOBDD* bdd, const set<Variable*>& vars, const set<FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	int maxint = numeric_limits<int>::max();
	double maxdouble = numeric_limits<double>::max();
	if(bdd == _falsebdd) {
		return 0;
	}
	else if(bdd == _truebdd) {
		int res = univNrAnswers(vars,indices,structure);
		if(res == maxint) return maxdouble;
		else return double(res);
	}
	else {
		// split the variables among those that occur in the kernel and those that don't
		set<Variable*> kernvars;
		set<Variable*> othervars;
		for(set<Variable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
			if(contains(bdd->kernel(),*it)) kernvars.insert(*it);
			else othervars.insert(*it);
		}
		set<FOBDDDeBruijnIndex*> kernindices;
		set<FOBDDDeBruijnIndex*> otherindices;
		for(set<FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
			if(bdd->kernel()->containsDeBruijnIndex((*it)->index())) kernindices.insert(*it);
			else otherindices.insert(*it);
		}
	
		// get the number of answers to the kernels and branches
		double kernanswers = estimatedNrAnswers(bdd->kernel(),kernvars,kernindices,structure);
		double trueanswers = estimatedNrAnswers(bdd->truebranch(),othervars,otherindices,structure);
		double falseanswers = estimatedNrAnswers(bdd->falsebranch(),othervars,otherindices,structure);
		double allkernanswers = estimatedNrAnswers(_truebdd,kernvars,kernindices,structure);

		if(!(kernanswers < maxdouble)) return maxdouble;
		if(!(allkernanswers < maxdouble) && falseanswers > 0) return maxdouble;
		if(!(trueanswers < maxdouble) && kernanswers > 0) return maxdouble;
		if(!(falseanswers < maxdouble) && kernanswers < allkernanswers) return maxdouble;

		// compute the number of answers
		if(kernanswers * trueanswers + (allkernanswers - kernanswers) * falseanswers > maxdouble) return maxdouble;
		else return kernanswers * trueanswers + (allkernanswers - kernanswers) * falseanswers;
	}
}
/*
double FOBDDManager::estimatedCostOne(FOBDD* bdd, const set<Variable*>& vars, const set<FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	double costall = estimatedCostAll(bdd,vars,indices,structure);
	int nrans = double(estimatedNrAnswers(bdd,vars,indices,structure));
	if(nrans == numeric_limits<int>::max()) return numeric_limits<double>::max();
	else return costall / double(nrans);
}

double FOBDDManager::estimatedCostAll(FOBDD* bdd, const set<Variable*>& vars, const set<FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	double maxdouble = numeric_limits<double>::max();
	int maxint = numeric_limits<int>::max();
	if(bdd == _falsebdd) return 1;
	else if(bdd = _truebdd) return estimatedNrAnswers(bdd,vars,indices,structure);
	else {
		// split the variables among those that occur in the kernel and those that don't
		set<Variable*> kernvars;
		set<Variable*> othervars;
		for(set<Variable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
			if(contains(bdd->kernel(),*it)) kernvars.insert(*it);
			else othervars.insert(*it);
		}
		set<FOBDDDeBruijnIndex*> kernindices;
		set<FOBDDDeBruijnIndex*> otherindices;
		for(set<FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
			if(bdd->kernel()->containsDeBruijnIndex((*it)->index())) kernindices.insert(*it);
			else otherindices.insert(*it);
		}

		if(bdd->falsebranch() == _falsebdd) {
			double kerncost = estimatedTrueCostAll(bdd->kernel(),kernvars,kernindices,structure);
			int nrkernans = estimatedNrAnswers(bdd->kernel(),kernvars,kernindices,structure);
			double truecost = estimatedCostAll(bdd->truebranch(),othervars,otherindices,structure);
			if(kerncost < maxdouble && truecost < maxdouble && nrkernans != maxint) {
				if((kerncost * double(nrkernans)) > numeric_limits<double>::max()) return maxdouble;
				double branchcost = kerncost * double(nrkernans);
				if((branchcost + kerncost) > numeric_limits<double>::max()) return maxdouble;
				else return branchcost + kerncost;
			}
			else return maxdouble;
		}
		else if(bdd->truebranch() == _falsebdd) {
			double kerncost = estimatedFalseCostAll(bdd->kernel(),kernvars,kernindices,structure);
			int nrkernans = estimatedNrAnswers(bdd->kernel(),kernvars,kernindices,structure);
			int allkernanswers = estimatedNrAnswers(_truebdd,kernvars,kernindices,structure);
			double falsecost = estimatedCostAll(bdd->falsebranch(),othervars,otherindices,structure);
			// TODO
			if(kerncost < maxdouble && falsecost < maxdouble && invkernans != maint) {
				
			}
			else return maxdouble;
		}
		else {
			// TODO
		}
	}
}
*/
