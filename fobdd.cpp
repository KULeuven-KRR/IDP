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

extern int global_seed;

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

KernelOrder FOBDDManager::newOrder(const vector<const FOBDDArgument*>& args) {
	unsigned int category = STANDARDCATEGORY;
	for(unsigned int n = 0; n < args.size(); ++n) {
		if(args[n]->containsFreeDeBruijnIndex()) {
			category = DEBRUIJNCATEGORY;
			break;
		}
	}
	return newOrder(category);
}

KernelOrder FOBDDManager::newOrder(const FOBDD* bdd) {
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

class Bump : public FOBDDVisitor {
	private:
		unsigned int			_depth;
		const FOBDDVariable*	_variable;
	public:
		Bump(FOBDDManager* manager, const FOBDDVariable* variable, unsigned int depth) :
			FOBDDVisitor(manager), _depth(depth), _variable(variable) { }

		const FOBDDQuantKernel*	change(const FOBDDQuantKernel* kernel) {
			++_depth;
			FOBDD* nbdd = FOBDDVisitor::change(kernel->bdd());
			--_depth;
			return _manager->getQuantKernel(kernel->sort(),nbdd);
		}

		const FOBDDArgument* change(const FOBDDVariable* var) {
			if(var == _variable) return _manager->getDeBruijnIndex(var->variable()->sort(),_depth);
			else return var;
		}

		const FOBDDArgument* change(const FOBDDDeBruijnIndex* dbr) {
			if(_depth <= dbr->index()) return _manager->getDeBruijnIndex(dbr->sort(),dbr->index()+1);
			else return dbr;
		}

};

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

const FOBDD* FOBDDManager::getBDD(const FOBDDKernel* kernel,const FOBDD* truebranch,const FOBDD* falsebranch) {
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

FOBDD* FOBDDManager::addBDD(const FOBDDKernel* kernel,const FOBDD* truebranch,const FOBDD* falsebranch) {
	FOBDD* newbdd = new FOBDD(kernel,truebranch,falsebranch);
	_bddtable[kernel][falsebranch][truebranch] = newbdd;
	return newbdd;
}

const FOBDDAtomKernel* FOBDDManager::getAtomKernel(PFSymbol* symbol,AtomKernelType akt, const vector<const FOBDDArgument*>& args) {
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

FOBDDAtomKernel* FOBDDManager::addAtomKernel(PFSymbol* symbol,AtomKernelType akt, const vector<const FOBDDArgument*>& args) {
	FOBDDAtomKernel* newkernel = new FOBDDAtomKernel(symbol,akt,args,newOrder(args));
	_atomkerneltable[symbol][akt][args] = newkernel;
	return newkernel;
}

const FOBDDQuantKernel* FOBDDManager::getQuantKernel(Sort* sort,const FOBDD* bdd) {
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

FOBDDQuantKernel* FOBDDManager::addQuantKernel(Sort* sort,const FOBDD* bdd) {
	FOBDDQuantKernel* newkernel = new FOBDDQuantKernel(sort,bdd,newOrder(bdd));
	_quantkerneltable[sort][bdd] = newkernel;
	return newkernel;
}

const FOBDDVariable* FOBDDManager::getVariable(Variable* var) {
	// Lookup
	VariableTable::const_iterator it = _variabletable.find(var);
	if(it != _variabletable.end()) {
		return it->second;
	}

	// Lookup failed, create a new variable
	return addVariable(var);
}

set<const FOBDDVariable*> FOBDDManager::getVariables(const set<Variable*>& vars) {
	set<const FOBDDVariable*> bddvars;
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

const FOBDDDeBruijnIndex* FOBDDManager::getDeBruijnIndex(Sort* sort, unsigned int index) {
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

const FOBDDFuncTerm* FOBDDManager::getFuncTerm(Function* func, const vector<const FOBDDArgument*>& args) {
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

FOBDDFuncTerm* FOBDDManager::addFuncTerm(Function* func, const vector<const FOBDDArgument*>& args) {
	FOBDDFuncTerm* newarg = new FOBDDFuncTerm(func,args);
	_functermtable[func][args] = newarg;
	return newarg;
}

const FOBDDDomainTerm* FOBDDManager::getDomainTerm(Sort* sort, const DomainElement* value) {
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

const FOBDD* FOBDDManager::negation(const FOBDD* bdd) {
	// TODO dynamic programming
	
	// Base cases
	if(bdd == _truebdd) return _falsebdd;
	if(bdd == _falsebdd) return _truebdd;

	// Recursive case
	const FOBDD* falsebranch = negation(bdd->falsebranch());
	const FOBDD* truebranch = negation(bdd->truebranch());
	return getBDD(bdd->kernel(),truebranch,falsebranch);
}

const FOBDD* FOBDDManager::conjunction(const FOBDD* bdd1, const FOBDD* bdd2) {
	// TODO dynamic programming
	
	// Base cases
	if(bdd1 == _falsebdd || bdd2 == _falsebdd) return _falsebdd;
	if(bdd1 == _truebdd) return bdd2;
	if(bdd2 == _truebdd) return bdd1;
	if(bdd1 == bdd2) return bdd1;

	// Recursive case
	if(*(bdd1->kernel()) < *(bdd2->kernel())) {
		const FOBDD* falsebranch = conjunction(bdd1->falsebranch(),bdd2);
		const FOBDD* truebranch = conjunction(bdd1->truebranch(),bdd2);
		return getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
	else if(*(bdd1->kernel()) > *(bdd2->kernel())) {
		const FOBDD* falsebranch = conjunction(bdd1,bdd2->falsebranch());
		const FOBDD* truebranch = conjunction(bdd1,bdd2->truebranch());
		return getBDD(bdd2->kernel(),truebranch,falsebranch);
	}
	else {
		assert(bdd1->kernel() == bdd2->kernel());
		const FOBDD* falsebranch = conjunction(bdd1->falsebranch(),bdd2->falsebranch());
		const FOBDD* truebranch = conjunction(bdd1->truebranch(),bdd2->truebranch());
		return getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
}

const FOBDD* FOBDDManager::disjunction(const FOBDD* bdd1, const FOBDD* bdd2) {
	// TODO dynamic programming
	
	// Base cases
	if(bdd1 == _truebdd || bdd2 == _truebdd) return _truebdd;
	if(bdd1 == _falsebdd) return bdd2;
	if(bdd2 == _falsebdd) return bdd1;
	if(bdd1 == bdd2) return bdd1;

	// Recursive case
	if(*(bdd1->kernel()) < *(bdd2->kernel())) {
		const FOBDD* falsebranch = disjunction(bdd1->falsebranch(),bdd2);
		const FOBDD* truebranch = disjunction(bdd1->truebranch(),bdd2);
		return getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
	else if(*(bdd1->kernel()) > *(bdd2->kernel())) {
		const FOBDD* falsebranch = disjunction(bdd1,bdd2->falsebranch());
		const FOBDD* truebranch = disjunction(bdd1,bdd2->truebranch());
		return getBDD(bdd2->kernel(),truebranch,falsebranch);
	}
	else {
		assert(bdd1->kernel() == bdd2->kernel());
		const FOBDD* falsebranch = disjunction(bdd1->falsebranch(),bdd2->falsebranch());
		const FOBDD* truebranch = disjunction(bdd1->truebranch(),bdd2->truebranch());
		return getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
}

const FOBDD* FOBDDManager::ifthenelse(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	// TODO dynamic programming
	const FOBDDKernel* truekernel = truebranch->kernel();
	const FOBDDKernel* falsekernel = falsebranch->kernel();

	if(*kernel < *truekernel) {
		if(*kernel < *falsekernel) {
			return getBDD(kernel,truebranch,falsebranch);
		}
		else if(kernel == falsekernel) {
			return getBDD(kernel,truebranch,falsebranch->falsebranch());
		}
		else {
			assert(*kernel > *falsekernel);
			const FOBDD* newtrue = ifthenelse(kernel,truebranch,falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel,truebranch,falsebranch->falsebranch());
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
			const FOBDD* newtrue = ifthenelse(kernel,truebranch,falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel,truebranch,falsebranch->falsebranch());
			return getBDD(falsekernel,newtrue,newfalse);
		}
	}
	else {
		assert(*kernel > *truekernel);
		if(*kernel < *falsekernel || kernel == falsekernel || *truekernel < *falsekernel) {
			const FOBDD* newtrue = ifthenelse(kernel,truebranch->truebranch(),falsebranch);
			const FOBDD* newfalse = ifthenelse(kernel,truebranch->falsebranch(),falsebranch);
			return getBDD(truekernel,newtrue,newfalse);
		}
		else if(truekernel == falsekernel) {
			const FOBDD* newtrue = ifthenelse(kernel,truebranch->truebranch(),falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel,truebranch->falsebranch(),falsebranch->falsebranch());
			return getBDD(truekernel,newtrue,newfalse);
		}
		else {
			assert(*falsekernel < *truekernel);
			const FOBDD* newtrue = ifthenelse(kernel,truebranch,falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel,truebranch,falsebranch->falsebranch());
			return getBDD(falsekernel,newtrue,newfalse);
		}
	}
}

const FOBDD* FOBDDManager::univquantify(const FOBDDVariable* var, const FOBDD* bdd) {
	// TODO dynamic programming
	const FOBDD* negatedbdd = negation(bdd);
	const FOBDD* quantbdd = existsquantify(var,negatedbdd);
	return negation(quantbdd);
}

const FOBDD* FOBDDManager::univquantify(const set<const FOBDDVariable*>& qvars, const FOBDD* bdd) {
	const FOBDD* negatedbdd = negation(bdd);
	const FOBDD* quantbdd = existsquantify(qvars,negatedbdd);
	return negation(quantbdd);
}

const FOBDD* FOBDDManager::existsquantify(const FOBDDVariable* var, const FOBDD* bdd) {
	// TODO dynamic programming
	Bump b(this,var,0);
	const FOBDD* bumped = b.FOBDDVisitor::change(bdd);
	return quantify(var->variable()->sort(),bumped);
}

const FOBDD* FOBDDManager::existsquantify(const set<const FOBDDVariable*>& qvars, const FOBDD* bdd) {
	const FOBDD* result = bdd;
	for(set<const FOBDDVariable*>::const_iterator it = qvars.begin(); it != qvars.end(); ++it) {
		result = existsquantify(*it,result);
	}
	return result;
}

const FOBDD* FOBDDManager::quantify(Sort* sort, const FOBDD* bdd) {
	// base case
	if(bdd == _truebdd || bdd == _falsebdd) {
		// FIXME take empty sorts into account!
		return bdd;
	}
	
	// Recursive case
	if(bdd->kernel()->category() == STANDARDCATEGORY) {
		const FOBDD* newfalse = quantify(sort,bdd->falsebranch());
		const FOBDD* newtrue = quantify(sort,bdd->truebranch());
		const FOBDD* result = ifthenelse(bdd->kernel(),newtrue,newfalse);
		return result;
	}
	else {
		const FOBDDQuantKernel* kernel = getQuantKernel(sort,bdd);
		return getBDD(kernel,_truebdd,_falsebdd);
	}
}

class Substitute : public FOBDDVisitor {
	private:
		map<const FOBDDVariable*, const FOBDDVariable*>	_mvv;
	public:
		Substitute(FOBDDManager* manager, const map<const FOBDDVariable*, const FOBDDVariable*>	mvv) :
			FOBDDVisitor(manager), _mvv(mvv) { }

		const FOBDDVariable*	change(FOBDDVariable* v) {
			map<const FOBDDVariable*,const FOBDDVariable*>::const_iterator it = _mvv.find(v);
			if(it != _mvv.end()) return it->second;
			else return v;
		}
};

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd,const map<const FOBDDVariable*,const FOBDDVariable*>& mvv) {
	Substitute s(this,mvv);
	return s.FOBDDVisitor::change(bdd);
}

int FOBDDManager::longestbranch(const FOBDDKernel* kernel) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) return 1;
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* qk = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		return longestbranch(qk->bdd()) + 1;
	}
}

int FOBDDManager::longestbranch(const FOBDD* bdd) {
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
	vector<const FOBDDArgument*> args(ft->function()->arity());
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
	vector<const FOBDDArgument*> args(pf->symbol()->nrSorts());
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
		const FOBDD* temp = _manager->truebdd();
		for(vector<Formula*>::const_iterator it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
			(*it)->accept(this);
			temp = _manager->conjunction(temp,_bdd);
		}
		_bdd = temp;
	}
	else {
		const FOBDD* temp = _manager->falsebdd();
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
	const FOBDD* qbdd = _bdd;
	for(set<Variable*>::const_iterator it = qf->quantvars().begin(); it != qf->quantvars().end(); ++it) {
		const FOBDDVariable* qvar = _manager->getVariable(*it);
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

ostream& FOBDDManager::put(ostream& output, const FOBDD* bdd, unsigned int spaces) const {
	if(bdd == _truebdd) {
		printtabs(output,spaces);
		output << "true\n";
	}
	else if(bdd == _falsebdd) {
		printtabs(output,spaces);
		output << "false\n";
	}
	else {
		put(output,bdd->kernel(),spaces);
		printtabs(output,spaces+3);
		output << "FALSE BRANCH:\n";
		put(output,bdd->falsebranch(),spaces+6);
		printtabs(output,spaces+3);
		output << "TRUE BRANCH:\n";
		put(output,bdd->truebranch(),spaces+6);
	}
	return output;
}

ostream& FOBDDManager::put(ostream& output, const FOBDDKernel* kernel, unsigned int spaces) const {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		printtabs(output,spaces);
		output << *symbol;
		if(atomkernel->type() == AKT_CF) output << "<cf>";
		else if(atomkernel->type() == AKT_CT) output << "<ct>";
		if(typeid(*symbol) == typeid(Predicate)) {
			if(symbol->nrSorts()) {
				output << "(";
				put(output,atomkernel->args(0));
				for(unsigned int n = 1; n < symbol->nrSorts(); ++n) {
					output << ",";
					put(output,atomkernel->args(n));
				}
				output << ")";
			}
		}
		else {
			if(symbol->nrSorts() > 1) {
				output << "(";
				put(output,atomkernel->args(0));
				for(unsigned int n = 1; n < symbol->nrSorts()-1; ++n) {
					output << ",";
					put(output,atomkernel->args(n));
				}
				output << ")";
			}
			output << " = ";
			put(output,atomkernel->args(symbol->nrSorts()-1));
		}
	}
	else if(typeid(*kernel) == typeid(FOBDDQuantKernel)) {
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		printtabs(output,spaces);
		output << "EXISTS(" << *(quantkernel->sort()) << ") {\n";
		put(output,quantkernel->bdd(),spaces+3);
		printtabs(output,spaces);
		output << "}";
	}
	else {
		assert(false);
	}
	output << '\n';
	return output;
}

ostream& FOBDDManager::put(ostream& output, const FOBDDArgument* arg) const {
	if(typeid(*arg) == typeid(FOBDDVariable)) {
		const FOBDDVariable* var = dynamic_cast<const FOBDDVariable*>(arg);
		var->variable()->put(output);
	}
	else if(typeid(*arg) == typeid(FOBDDDeBruijnIndex)) {
		const FOBDDDeBruijnIndex* dbr = dynamic_cast<const FOBDDDeBruijnIndex*>(arg);
		output << "<" << dbr->index() << ">[" << *(dbr->sort()) << "]";
	}
	else if(typeid(*arg) == typeid(FOBDDFuncTerm)) {
		const FOBDDFuncTerm* ft = dynamic_cast<const FOBDDFuncTerm*>(arg);
		Function* f = ft->func();
		output << *f;
		if(f->arity()) {
			output << "(";
			put(output,ft->args(0));
			for(unsigned int n = 1; n < f->arity(); ++n) {
				output << ",";
				put(output,ft->args(n));
			}
			output << ")";
		}
	}
	else if(typeid(*arg) == typeid(FOBDDDomainTerm)) {
		const FOBDDDomainTerm* dt = dynamic_cast<const FOBDDDomainTerm*>(arg);
		output << *(dt->value()) << "[" << *(dt->sort()) << "]";
	}
	else {
		assert(false);
	}
	return output;
}

/*****************
	Estimators
*****************/

/**
 * Returns true iff the bdd contains the variable
 */
bool FOBDDManager::contains(const FOBDD* bdd, const FOBDDVariable* v) {
	if(bdd == _truebdd || bdd == _falsebdd) return false;
	else {
		if(contains(bdd->kernel(),v)) return true;
		else if(contains(bdd->truebranch(),v)) return true;
		else if(contains(bdd->falsebranch(),v)) return true;
		else return false;
	}
}

/**
 * Returns true iff the kernel contains the variable
 */
bool FOBDDManager::contains(const FOBDDKernel* kernel, const FOBDDVariable* v) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		for(unsigned int n = 0; n < atomkernel->symbol()->sorts().size(); ++n) {
			if(contains(atomkernel->args(n),v)) return true;
		}
		return false;
	}
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		return contains(quantkernel->bdd(),v);
	}
}

/**
 * Returns true iff the argument contains the variable
 */
bool FOBDDManager::contains(const FOBDDArgument* arg, const FOBDDVariable* v) {
	if(typeid(*arg) == typeid(FOBDDVariable)) return arg == v;
	else if(typeid(*arg) == typeid(FOBDDFuncTerm)) {
		const FOBDDFuncTerm* farg = dynamic_cast<const FOBDDFuncTerm*>(arg);
		for(unsigned int n = 0; n < farg->func()->arity(); ++n) {
			if(contains(farg->args(n),v)) return true;
		}
		return false;
	}
	else return false;
}

/**
 * Returns true iff the kernel contains the variable
 */
bool FOBDDManager::contains(const FOBDDKernel* kernel, Variable* v) {
	const FOBDDVariable* var = getVariable(v);
	return contains(kernel,var);
}

/**
 * Returns the product of the sizes of the interpretations of the sorts of the given variables and indices in the given structure
 */
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
 * Returns all paths in the given bdd that end in the node 'false'
 * Each path is represented by a vector of pairs of booleans and kernels.
 * The kernels are the succesive nodes in the path, 
 * the booleans indicate whether the path continues via the false or true branch.
 */
vector<vector<pair<bool,const FOBDDKernel*> > > FOBDDManager::pathsToFalse(const FOBDD* bdd) {
	vector<vector<pair<bool,const FOBDDKernel*> > > result;
	if(bdd == _falsebdd) {
		result.push_back(vector<pair<bool,const FOBDDKernel*> >(0));
	}
	else if(bdd != _truebdd) {
		vector<vector<pair<bool,const FOBDDKernel*> > > falsepaths = pathsToFalse(bdd->falsebranch());
		vector<vector<pair<bool,const FOBDDKernel*> > > truepaths = pathsToFalse(bdd->truebranch());
		for(vector<vector<pair<bool,const FOBDDKernel*> > >::const_iterator it = falsepaths.begin(); it != falsepaths.end(); ++it) {
			result.push_back(vector<pair<bool,const FOBDDKernel*> >(1,pair<bool,const FOBDDKernel*>(false,bdd->kernel())));
			for(vector<pair<bool,const FOBDDKernel*> >::const_iterator jt = it->begin(); jt != it->end(); ++jt) {
				result.back().push_back(*jt);
			}
		}
		for(vector<vector<pair<bool,const FOBDDKernel*> > >::const_iterator it = truepaths.begin(); it != truepaths.end(); ++it) {
			result.push_back(vector<pair<bool,const FOBDDKernel*> >(1,pair<bool,const FOBDDKernel*>(true,bdd->kernel())));
			for(vector<pair<bool,const FOBDDKernel*> >::const_iterator jt = it->begin(); jt != it->end(); ++jt) {
				result.back().push_back(*jt);
			}
		}
	}
	return result;
}

/**
 * Return all kernels of the given bdd that occur outside the scope of quantifiers
 */
set<const FOBDDKernel*> FOBDDManager::nonnestedkernels(const FOBDD* bdd) {
	set<const FOBDDKernel*> result;
	if(bdd != _truebdd && bdd != _falsebdd) {
		set<const FOBDDKernel*> falsekernels = nonnestedkernels(bdd->falsebranch());
		set<const FOBDDKernel*> truekernels = nonnestedkernels(bdd->truebranch());
		result.insert(falsekernels.begin(),falsekernels.end());
		result.insert(truekernels.begin(),truekernels.end());
		result.insert(bdd->kernel());
	}
	return result;
}

/**
 * Return a mapping from the non-nested kernels of the given bdd to their estimated number of answers with
 * respect to the given variables and indices
 */
map<const FOBDDKernel*,double> FOBDDManager::kernelAnswers(const FOBDD* bdd, const set<Variable*>& vars, const set<const FOBDDDeBruijnIndex*> indices, AbstractStructure* structure) {
	map<const FOBDDKernel*,double> result;
	set<const FOBDDKernel*> kernels = nonnestedkernels(bdd);
	for(set<const FOBDDKernel*>::const_iterator it = kernels.begin(); it != kernels.end(); ++it) {
		result[*it] = estimatedNrAnswers(*it,vars,indices,structure);
	}
	return result;
}

/**
 * Class to obtain all variables of a bdd
 */
class VariableCollector : public FOBDDVisitor {
	private:
		set<const FOBDDVariable*>	_result;
	public:
		VariableCollector(FOBDDManager* m) : FOBDDVisitor(m) { }
		void visit(const FOBDDVariable* v) { _result.insert(v);	}
		const set<const FOBDDVariable*>&	result()	{ return _result;	}
};

/**
 * Returns all variables that occur in the given bdd
 */
set<const FOBDDVariable*> FOBDDManager::variables(const FOBDD* bdd) {
	VariableCollector vc(this);
	vc.FOBDDVisitor::visit(bdd);
	return vc.result();
}

/**
 * Returns all variables that occur in the given kernel
 */
set<const FOBDDVariable*> FOBDDManager::variables(const FOBDDKernel* kernel) {
	VariableCollector vc(this);
	kernel->accept(&vc);
	return vc.result();
}

/**
 * Class to obtain all unquantified DeBruijnIndices of a bdd
 */
class DeBruijnCollector : public FOBDDVisitor {
	private:
		set<const FOBDDDeBruijnIndex*>	_result;
		unsigned int					_minimaldepth;
	public:
		DeBruijnCollector(FOBDDManager* m) : FOBDDVisitor(m), _minimaldepth(0) { }
		void visit(const FOBDDQuantKernel* kernel) {
			++_minimaldepth;
			FOBDDVisitor::visit(kernel->bdd());
			--_minimaldepth;
		}
		void visit(const FOBDDDeBruijnIndex* index) {
			if(index->index() >= _minimaldepth) {
				const FOBDDDeBruijnIndex* i = _manager->getDeBruijnIndex(index->sort(),index->index() - _minimaldepth);
				_result.insert(i);
			}
		}
		const set<const FOBDDDeBruijnIndex*>&	result()	const { return _result;	}

};

/**
 * Returns all De Bruijn indices that occur in the given bdd. 
 */
set<const FOBDDDeBruijnIndex*> FOBDDManager::indices(const FOBDD* bdd) {
	DeBruijnCollector dbc(this);
	dbc.FOBDDVisitor::visit(bdd);
	return dbc.result();
}

/**
 * Returns all variables that occur in the given kernel
 */
set<const FOBDDDeBruijnIndex*> FOBDDManager::indices(const FOBDDKernel* kernel) {
	DeBruijnCollector dbc(this);
	kernel->accept(&dbc);
	return dbc.result();
}

/**
 * Returns the product of the size of the domains of all variables and non-quantified indices in the
 * given kernel, except those among the given variables and indices
 */
double FOBDDManager::univSize(const FOBDDKernel* kernel, const set<Variable*>& vars, const set<const FOBDDDeBruijnIndex*> indices, AbstractStructure* structure) {
	double maxdouble = numeric_limits<double>::max();
	set<const FOBDDVariable*> kernelvars = variables(kernel);
	set<const FOBDDDeBruijnIndex*> kernelindices = FOBDDManager::indices(kernel);
	for(set<Variable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
		const FOBDDVariable* v = getVariable(*it);
		kernelvars.erase(v);
	}
	for(set<const FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
		kernelindices.erase(*it);
	}
	double result = 1;
	for(set<const FOBDDVariable*>::const_iterator it = kernelvars.begin(); it != kernelvars.end(); ++it) {
		tablesize ts = structure->inter((*it)->variable()->sort())->size();
		if(ts.first) result = result * double(ts.second);
		else return maxdouble;
	}
	for(set<const FOBDDDeBruijnIndex*>::const_iterator it = kernelindices.begin(); it != kernelindices.end(); ++it) {
		tablesize ts = structure->inter((*it)->sort())->size();
		if(ts.first) result = result * double(ts.second);
		else return maxdouble;
	}
}

map<const FOBDDKernel*,double> FOBDDManager::kernelUnivs(const FOBDD* bdd, const set<Variable*>& vars, const set<const FOBDDDeBruijnIndex*> indices, AbstractStructure* structure) {
	map<const FOBDDKernel*,double> result;
	set<const FOBDDKernel*> kernels = nonnestedkernels(bdd);
	for(set<const FOBDDKernel*>::const_iterator it = kernels.begin(); it != kernels.end(); ++it) {
		result[*it] = univSize(*it,vars,indices,structure);
	}
	return result;
}

double FOBDDManager::estimatedChance(const FOBDDKernel* kernel, AbstractStructure* structure) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		double chance = 0;
		PFSymbol* symbol = atomkernel->symbol();
		PredInter* pinter;
		if(typeid(*symbol) == typeid(Predicate)) pinter = structure->inter(dynamic_cast<Predicate*>(symbol));
		else pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphinter();
		const PredTable* pt = atomkernel->type() == AKT_CF ? pinter->cf() : pinter->ct();
		tablesize symbolsize = pt->size();
		tablesize univsize = pt->universe().size();
		if(symbolsize.first) {
			if(univsize.first && univsize.second != 0) {
				assert(symbolsize.second <= univsize.second);
				chance = double(symbolsize.second) / double(univsize.second);
			}
			else chance = 0;
		}
		else {
			// TODO better estimators possible?
			if(univsize.first) chance = 0.5;
			else {
				if(typeid(*(pt->interntable())) == typeid(FullInternalPredTable)) chance = 1;
				else chance = 0;
			}
		}
		return chance;
	}
	else {	// case of a quantification kernel
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);

		// get the table of the sort of the quantified variable 
		SortTable* quantsorttable = structure->inter(quantkernel->sort());
		tablesize quanttablesize = quantsorttable->size();

		// some simple checks
		int quantsize = 0;
		if(quanttablesize.first) {
			if(quanttablesize.second == 0) return 0;	// if the sort is empty, the kernel cannot be true
			else quantsize = quanttablesize.second;
		}
		else {
			if(!quantsorttable->approxfinite()) {
				// if the sort is infinite, the kernel is true if the chance of the bdd is nonzero.
				double bddchance = estimatedChance(quantkernel->bdd(),structure);
				return bddchance == 0 ? 0 : 1;
			}
			else {
				// TODO TODO TODO
				assert(false);
				return 0.5;
			}
		}

		// collect the paths that lead to node 'false'
		vector<vector<pair<bool,const FOBDDKernel*> > > paths = pathsToFalse(quantkernel->bdd());

		// collect all kernels and their estimated number of answers
		set<const FOBDDDeBruijnIndex*> idx; idx.insert(getDeBruijnIndex(quantkernel->sort(),0));
		map<const FOBDDKernel*,double> subkernels = kernelAnswers(quantkernel->bdd(),set<Variable*>(),idx,structure);
		map<const FOBDDKernel*,double> subunivs = kernelUnivs(quantkernel->bdd(),set<Variable*>(),idx,structure);

		srand(global_seed);
		double sum = 0;		// stores the sum of the chances obtained by each experiment
		int sumcount = 0;	// stores the number of succesfull experiments
		for(unsigned int experiment = 0; experiment < 10; ++experiment) {	// do 10 experiments
			// An experiment consists of trying to reach N times node 'false', 
			// where N is the size of the domain of the quantified variable.
			
			map<const FOBDDKernel*,double> dynsubkernels = subkernels;
			bool fail = false;

			double chance = 1;
			for(int element = 0; element < quantsize; ++element) {
				// Compute possibility of each path
				vector<double> cumulative_pathsposs;
				double cumulative_chance = 0;
				for(unsigned int pathnr = 0; pathnr < paths.size(); ++pathnr) {
					double currchance = 1;
					for(unsigned int nodenr = 0; nodenr < paths[pathnr].size(); ++nodenr) {
						if(paths[pathnr][nodenr].first) 
							currchance = 
								currchance * dynsubkernels[paths[pathnr][nodenr].second] / double(quantsize - element);
						else 
							currchance = 
								currchance * (quantsize - element - dynsubkernels[paths[pathnr][nodenr].second]) / double(quantsize - element);
					}
					cumulative_chance += currchance;
					cumulative_pathsposs.push_back(cumulative_chance);
				}
				assert(cumulative_chance <= 1);
				if(cumulative_chance > 0) {	// there is a possible path to false
					chance = chance * cumulative_chance;

					// randomly choose a path
					double toss = double(rand()) / double(RAND_MAX) * cumulative_chance;
					unsigned int chosenpathnr = lower_bound(cumulative_pathsposs.begin(),cumulative_pathsposs.end(),toss) - cumulative_pathsposs.begin();
					for(unsigned int nodenr = 0; nodenr < paths[chosenpathnr].size(); ++nodenr) {
						if(paths[chosenpathnr][nodenr].first) 
							dynsubkernels[paths[chosenpathnr][nodenr].second] += -(1.0 / subunivs[paths[chosenpathnr][nodenr].second]);
					}
				}
				else {	// the experiment failed 
					fail = true;
					break;
				}
			}

			if(!fail) {
				sum += chance;
				++sumcount;
			}
		}

		if(sum == 0) {	// no experiment succeeded 
			return 1;
		}
		else {	// at least one experiment succeeded: take average of all succesfull experiments
			return 1 - (sum / double(sumcount));
		}
	}
}

double FOBDDManager::estimatedChance(const FOBDD* bdd, AbstractStructure* structure) {
	if(bdd == _falsebdd) return 0;
	else if(bdd == _truebdd) return 1;
	else {
		double kernchance = estimatedChance(bdd->kernel(),structure);
		double falsechance = estimatedChance(bdd->falsebranch(),structure);
		double truechance = estimatedChance(bdd->truebranch(),structure);
		return (kernchance * truechance) + ((1-kernchance) * falsechance);
	}
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | kernel } in the given structure 
 */
double FOBDDManager::estimatedNrAnswers(const FOBDDKernel* kernel, const set<Variable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	// TODO: improve this if functional dependency is known
	int maxint = numeric_limits<int>::max();
	double maxdouble = numeric_limits<double>::max();
	double kernelchance = estimatedChance(kernel,structure);
	int univanswers = univNrAnswers(vars,indices,structure);
	if(univanswers == maxint) {
		return (kernelchance > 0 ? maxdouble : 0);
	}
	else return kernelchance * univanswers;
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | bdd } in the given structure 
 */
double FOBDDManager::estimatedNrAnswers(const FOBDD* bdd, const set<Variable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	int maxint = numeric_limits<int>::max();
	double maxdouble = numeric_limits<double>::max();
	double bddchance = estimatedChance(bdd,structure);
	int univanswers = univNrAnswers(vars,indices,structure);
	if(univanswers == maxint) {
		return (bddchance > 0 ? maxdouble : 0);
	}
	else return bddchance * univanswers;
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

/**************
	Visitor
**************/

void FOBDDAtomKernel::accept(FOBDDVisitor* v)		const { v->visit(this);	}
void FOBDDQuantKernel::accept(FOBDDVisitor* v)		const { v->visit(this);	}
void FOBDDVariable::accept(FOBDDVisitor* v)			const { v->visit(this);	}
void FOBDDDeBruijnIndex::accept(FOBDDVisitor* v)	const { v->visit(this);	}
void FOBDDDomainTerm::accept(FOBDDVisitor* v)		const { v->visit(this);	}
void FOBDDFuncTerm::accept(FOBDDVisitor* v)			const { v->visit(this);	}

FOBDDKernel*	FOBDDAtomKernel::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}
FOBDDKernel*	FOBDDQuantKernel::acceptchange(FOBDDVisitor* v)	const { return v->change(this);	}
FOBDDArgument*	FOBDDVariable::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}
FOBDDArgument*	FOBDDDeBruijnIndex::acceptchange(FOBDDVisitor* v)	const { return v->change(this);	}
FOBDDArgument*	FOBDDDomainTerm::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}
FOBDDArgument*	FOBDDFuncTerm::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}

void FOBDDVisitor::visit(const FOBDD* bdd) {
	if(bdd != _manager->truebdd() && bdd != _manager->falsebdd()) {
		bdd->kernel()->accept(this);
		visit(bdd->truebranch());
		visit(bdd->falsebranch());
	}
}

void FOBDDVisitor::visit(const FOBDDAtomKernel* kernel) {
	for(vector<FOBDDArgument*>::const_iterator it = kernel->args().begin(); it != kernel->args().end(); ++it) {
		(*it)->accept(this);
	}
}

void FOBDDVisitor::visit(const FOBDDQuantKernel* kernel) {
	kernel->bdd()->accept(this);
}

void FOBDDVisitor::visit(const FOBDDVariable* ) {
	// do nothing
}

void FOBDDVisitor::visit(const FOBDDDeBruijnIndex* ) {
	// do nothing
}

void FOBDDVisitor::visit(const FOBDDDomainTerm* ) {
	// do nothing
}

void FOBDDVisitor::visit(const FOBDDFuncTerm* term) {
	for(vector<FOBDDArgument*>::const_iterator it = term->args().begin(); it != term->args().end(); ++it) {
		(*it)->accept(this);
	}
}

const FOBDD* FOBDDVisitor::change(const FOBDD* bdd) {
	if(_manager->isTruebdd(bdd)) return _manager->truebdd();
	else if(_manager->isFalsebdd(bdd)) return _manager->falsebdd();
	else {
		FOBDDKernel* nk = bdd->kernel()->acceptchange(this);
		FOBDD* nt = change(bdd->truebranch());
		FOBDD* nf = change(bdd->falsebranch());
		return manager->ifthenelse(nk,nt,nf);
	}
}

const FOBDDKernel* FOBDDVisitor::change(const FOBDDAtomKernel* kernel) {
	vector<FOBDDArgument*>	nargs;
	for(vector<FOBDDArgument*>::const_iterator it = kernel->args().begin(); it != kernel->args().end(); ++it) {
		nargs.push_back((*it)->acceptchange(this));
	}
	return manager->getAtomKernel(kernel->symbol(),kernel->type(),nargs);
}

const FOBDDKernel* FOBDDVisitor::change(const FOBDDQuantKernel* kernel) {
	FOBDD* nbdd = kernel->bdd()->change(this);
	return manager->getQuantKernel(kernel->sort(),nbdd);
}

const FOBDDArgument* FOBDDVisitor::visit(const FOBDDVariable* variable) {
	return variable;
}

const FOBDDArgument* FOBDDVisitor::visit(const FOBDDDeBruijnIndex* index) {
	return index;
}

const FOBDDArgument* FOBDDVisitor::visit(const FOBDDDomainTerm* term) {
	return term;
}

const FOBDDArgument* FOBDDVisitor::visit(const FOBDDFuncTerm* term) {
	vector<FOBDDArgument*>	nargs;
	for(vector<FOBDDArgument*>::const_iterator it = term->args().begin(); it != term->args().end(); ++it) {
		nargs.push_back((*it)->acceptchange(this));
	}
	return manager->getFuncTerm(term->function(),nargs);
}
