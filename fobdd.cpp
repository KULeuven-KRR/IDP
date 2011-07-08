/************************************
	fobdd.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cassert>
#include <typeinfo>
#include <cmath>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
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
	unsigned int category = (bdd->containsDeBruijnIndex(1)) ? DEBRUIJNCATEGORY : STANDARDCATEGORY;
	return newOrder(category);
}

void FOBDDManager::moveUp(const FOBDDKernel* kernel) {
	unsigned int cat = kernel->category();
	if(cat != TRUEFALSECATEGORY) {
		unsigned int nr = kernel->number();
		if(nr != 0) {
			--nr;
			const FOBDDKernel* pkernel = _kernels[cat][nr];
			moveDown(pkernel);
		}
	}
}

void FOBDDManager::moveDown(const FOBDDKernel* kernel) {
	unsigned int cat = kernel->category();
	if(cat != TRUEFALSECATEGORY) {
		unsigned int nr = kernel->number();
		vector<const FOBDD*> falseerase;
		vector<const FOBDD*> trueerase;
		if(_kernels[cat].find(nr+1) != _kernels[cat].end()) {
			const FOBDDKernel* nextkernel = _kernels[cat][nr+1];
			const MBDDMBDDBDD& bdds = _bddtable[kernel];
			for(MBDDMBDDBDD::const_iterator it = bdds.begin(); it != bdds.end(); ++it) {
				for(MBDDBDD::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
					FOBDD* bdd = jt->second;
					bool swapfalse = (nextkernel == it->first->kernel());
					bool swaptrue = (nextkernel == jt->first->kernel());
					if(swapfalse || swaptrue) {
						falseerase.push_back(it->first);
						trueerase.push_back(jt->first);
					}
					if(swapfalse) {
						if(swaptrue) {
							const FOBDD* newfalse = getBDD(kernel,jt->first->falsebranch(),it->first->falsebranch()); 
							const FOBDD* newtrue = getBDD(kernel,jt->first->truebranch(),it->first->truebranch());
							bdd->replacefalse(newfalse);
							bdd->replacetrue(newtrue);
							bdd->replacekernel(nextkernel);
							_bddtable[nextkernel][newfalse][newtrue] = bdd;
						}
						else {
							const FOBDD* newfalse = getBDD(kernel,jt->first,it->first->falsebranch()); 
							const FOBDD* newtrue = getBDD(kernel,jt->first,it->first->truebranch());
							bdd->replacefalse(newfalse);
							bdd->replacetrue(newtrue);
							bdd->replacekernel(nextkernel);
							_bddtable[nextkernel][newfalse][newtrue] = bdd;
						}
					}
					else if(swaptrue) {
						const FOBDD* newfalse = getBDD(kernel,jt->first->falsebranch(),it->first); 
						const FOBDD* newtrue = getBDD(kernel,jt->first->truebranch(),it->first);
						bdd->replacefalse(newfalse);
						bdd->replacetrue(newtrue);
						bdd->replacekernel(nextkernel);
						_bddtable[nextkernel][newfalse][newtrue] = bdd;
					}
				}
			}
			for(unsigned int n = 0; n < falseerase.size(); ++n) {
				_bddtable[kernel][falseerase[n]].erase(trueerase[n]);
				if(_bddtable[kernel][falseerase[n]].empty()) _bddtable[kernel].erase(falseerase[n]);
			}
			FOBDDKernel* tkernel = _kernels[cat][nr];
			FOBDDKernel* nkernel = _kernels[cat][nr+1];
			nkernel->replacenumber(nr);
			tkernel->replacenumber(nr+1);
			_kernels[cat][nr] = nkernel;
			_kernels[cat][nr+1] = tkernel;
		}
	}
}

/****************
	Arguments
****************/

Sort* FOBDDVariable::sort() const {
	return _variable->sort();
}

Sort* FOBDDFuncTerm::sort() const {
	return _function->outsort();
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
			const FOBDD* nbdd = FOBDDVisitor::change(kernel->bdd());
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

class FOBDDCopy : public FOBDDVisitor {
	private:
		FOBDDManager*			_originalmanager;
		FOBDDManager*			_copymanager;
		const FOBDDKernel*		_kernel;
		const FOBDDArgument*	_argument;
	public:
		FOBDDCopy(FOBDDManager* orig, FOBDDManager* copy) : FOBDDVisitor(orig), _originalmanager(orig), _copymanager(copy) { }

		void visit(const FOBDDVariable* var) {
			_argument = _copymanager->getVariable(var->variable());
		}

		void visit(const FOBDDDeBruijnIndex* index) {
			_argument = _copymanager->getDeBruijnIndex(index->sort(),index->index());
		}

		void visit(const FOBDDDomainTerm* term) {
			_argument = _copymanager->getDomainTerm(term->sort(),term->value());
		}

		void visit(const FOBDDFuncTerm* term) {
			vector<const FOBDDArgument*> newargs;
			for(vector<const FOBDDArgument*>::const_iterator it = term->args().begin(); it != term->args().end(); ++it) {
				newargs.push_back(copy(*it));
			}
			_argument = _copymanager->getFuncTerm(term->func(),newargs);
		}

		void visit(const FOBDDQuantKernel* kernel) {
			const FOBDD* newbdd = copy(kernel->bdd());
			_kernel = _copymanager->getQuantKernel(kernel->sort(),newbdd);
		}

		void visit(const FOBDDAtomKernel* kernel) {
			vector<const FOBDDArgument*> newargs;
			for(vector<const FOBDDArgument*>::const_iterator it = kernel->args().begin(); it != kernel->args().end(); ++it) {
				newargs.push_back(copy(*it));
			}
			_kernel = _copymanager->getAtomKernel(kernel->symbol(),kernel->type(),newargs);
		}

		const FOBDDArgument* copy(const FOBDDArgument* arg) {
			arg->accept(this);
			return _argument;
		}

		const FOBDDKernel* copy(const FOBDDKernel* kernel) {
			kernel->accept(this);
			return _kernel;
		}

		const FOBDD* copy(const FOBDD* bdd) {
			if(bdd == _originalmanager->truebdd()) return _copymanager->truebdd();
			else if(bdd == _originalmanager->falsebdd()) return _copymanager->falsebdd();
			else {
				const FOBDD* falsebranch = copy(bdd->falsebranch());
				const FOBDD* truebranch = copy(bdd->truebranch());
				const FOBDDKernel* kernel = copy(bdd->kernel());
				return _copymanager->ifthenelse(kernel,truebranch,falsebranch);
			}
		}
};

const FOBDD* FOBDDManager::getBDD(const FOBDD* bdd, FOBDDManager* manager) {
	FOBDDCopy copier(manager,this);
	const FOBDD* res = copier.copy(bdd);
	return res;
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
	_kernels[newkernel->category()][newkernel->number()] = newkernel;
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
	_kernels[newkernel->category()][newkernel->number()] = newkernel;
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
	// TODO: rewrite arithmetic terms!
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
	const FOBDD* q = quantify(var->variable()->sort(),bumped);
	return q;
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

		const FOBDDVariable* change(const FOBDDVariable* v) {
			map<const FOBDDVariable*,const FOBDDVariable*>::const_iterator it = _mvv.find(v);
			if(it != _mvv.end()) return it->second;
			else return v;
		}
};

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd,const map<const FOBDDVariable*,const FOBDDVariable*>& mvv) {
	Substitute s(this,mvv);
	return s.FOBDDVisitor::change(bdd);
}

class DomainTermSubstitute : public FOBDDVisitor {
	private:
		const FOBDDDomainTerm*	_domainterm;
		const FOBDDVariable*	_variable;
	public:
		DomainTermSubstitute(FOBDDManager* manager, const FOBDDDomainTerm* term, const FOBDDVariable* variable) :
			FOBDDVisitor(manager), _domainterm(term), _variable(variable) { }
		const FOBDDArgument* change(const FOBDDDomainTerm* dt) {
			if(dt == _domainterm) return _variable;
			else return dt;
		}
};

const FOBDDKernel* FOBDDManager::substitute(const FOBDDKernel* kernel,const FOBDDDomainTerm* term, const FOBDDVariable* variable) {
	DomainTermSubstitute s(this,term,variable);
	return kernel->acceptchange(&s);
}

class IndexSubstitute : public FOBDDVisitor {
	private:
		const FOBDDDeBruijnIndex*	_index;
		const FOBDDVariable*		_variable;
	public:
		IndexSubstitute(FOBDDManager* manager, const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable) :
			FOBDDVisitor(manager), _index(index), _variable(variable) { }
		const FOBDDArgument* change(const FOBDDDeBruijnIndex* i) {
			if(i == _index) return _variable;
			else return i;
		}
		const FOBDDKernel* change(const FOBDDQuantKernel* k) {
			_index = _manager->getDeBruijnIndex(_index->sort(),_index->index()+1);
			const FOBDD* nbdd = FOBDDVisitor::change(k->bdd());
			_index = _manager->getDeBruijnIndex(_index->sort(),_index->index()-1);
			return _manager->getQuantKernel(k->sort(),nbdd);
		}
};

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd,const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable) {
	IndexSubstitute s(this,index,variable);
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

/*****************
	Arithmetic
*****************/

class ArithChecker : public FOBDDVisitor {
	private:
		bool _result;
	public: 
		ArithChecker(FOBDDManager* m) : FOBDDVisitor(m) { }

		bool check(const FOBDDKernel* k)	{ _result = true; k->accept(this); return _result;	}
		bool check(const FOBDDArgument* a)	{ _result = true; a->accept(this); return _result;	}

		void visit(const FOBDDAtomKernel* kernel) {
			for(vector<const FOBDDArgument*>::const_iterator it = kernel->args().begin(); it != kernel->args().end(); ++it) {
				if(_result) (*it)->accept(this);
			}
			_result = _result && Vocabulary::std()->contains(kernel->symbol());
		}

		void visit(const FOBDDQuantKernel* ) {
			_result = false;
		}

		void visit(const FOBDDVariable* variable) {
			_result = _result && SortUtils::isSubsort(variable->sort(),VocabularyUtils::floatsort());
		}

		void visit(const FOBDDDeBruijnIndex* index) {
			_result = _result && SortUtils::isSubsort(index->sort(),VocabularyUtils::floatsort());
		}

		void visit(const FOBDDDomainTerm* domainterm) {
			_result = _result && SortUtils::isSubsort(domainterm->sort(),VocabularyUtils::floatsort());
		}

		void visit(const FOBDDFuncTerm* functerm) {
			for(vector<const FOBDDArgument*>::const_iterator it = functerm->args().begin(); it != functerm->args().end(); ++it) {
				if(_result) (*it)->accept(this);
			}
			_result = _result && Vocabulary::std()->contains(functerm->func());
		}

};

bool FOBDDManager::isArithmetic(const FOBDDKernel* k) {
	ArithChecker ac(this);
	return ac.check(k);
}

bool FOBDDManager::isArithmetic(const FOBDDArgument* a) {
	ArithChecker ac(this);
	return ac.check(a);
}

/**
 * Class to move all terms in an equation to the left hand side
 */
class TermsToLeft : public FOBDDVisitor {
	public:
		TermsToLeft(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDAtomKernel* change(const FOBDDAtomKernel* atom) {
			const DomainElement* minus_one = DomainElementFactory::instance()->create(-1);
			const DomainElement* zero = DomainElementFactory::instance()->create(0);
			const FOBDDDomainTerm* minus_one_term = _manager->getDomainTerm(VocabularyUtils::intsort(),minus_one);
			if(typeid(*(atom->symbol())) == typeid(Function)) {
				// TODO
			}
			else {
				const string& predname = atom->symbol()->name();
				if(predname == "=/2" || predname == "</2" || predname == ">/2") {
					const FOBDDArgument* rhs = atom->args(1);
					if(SortUtils::isSubsort(rhs->sort(),VocabularyUtils::floatsort())) {
						const FOBDDDomainTerm* zero_term = _manager->getDomainTerm(rhs->sort(),zero);
						if(rhs != zero_term) {
							const FOBDDArgument* lhs = atom->args(0);
							Sort* plussort = SortUtils::resolve(lhs->sort(),rhs->sort());
							if(plussort) {
								Function* plus = Vocabulary::std()->func("+/2");
								vector<Sort*> plussorts(2,plussort);
								plus = plus->disambiguate(plussorts,0);
								assert(plus);
								vector<const FOBDDArgument*> newlhsargs;
								newlhsargs.push_back(lhs);
								Function* times = Vocabulary::std()->func("*/2");
								vector<Sort*> timessorts(2,SortUtils::resolve(rhs->sort(),minus_one_term->sort()));
								times = times->disambiguate(timessorts,0);
								vector<const FOBDDArgument*> timesargs;
								timesargs.push_back(minus_one_term);
								timesargs.push_back(rhs);
								const FOBDDFuncTerm* timesterm = _manager->getFuncTerm(times,timesargs);
								newlhsargs.push_back(timesterm);
								const FOBDDFuncTerm* newlhs = _manager->getFuncTerm(plus,newlhsargs);
								vector<const FOBDDArgument*> newatomargs;
								newatomargs.push_back(newlhs);
								newatomargs.push_back(zero_term);
								atom = _manager->getAtomKernel(atom->symbol(),atom->type(),newatomargs);
							}
						}
					}
				}
			}
			return atom;
		}
};

/**
 * Class to exhaustively distribute addition with respect to multiplication in a FOBDDFuncTerm
 */
class Distributivity : public FOBDDVisitor {
	public:
		Distributivity(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
			if(functerm->func()->name() == "*/2") {
				const FOBDDArgument* leftterm = functerm->args(0);
				const FOBDDArgument* rightterm = functerm->args(1);
				if(typeid(*leftterm) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* leftfuncterm = dynamic_cast<const FOBDDFuncTerm*>(leftterm);
					if(leftfuncterm->func()->name() == "+/2") {
						vector<const FOBDDArgument*> newleftargs;
						newleftargs.push_back(leftfuncterm->args(0));
						newleftargs.push_back(rightterm);
						vector<const FOBDDArgument*> newrightargs;
						newrightargs.push_back(leftfuncterm->args(1));
						newrightargs.push_back(rightterm);
						const FOBDDFuncTerm* newleft = _manager->getFuncTerm(functerm->func(),newleftargs);
						const FOBDDFuncTerm* newright = _manager->getFuncTerm(functerm->func(),newrightargs);
						vector<const FOBDDArgument*> newargs;
						newargs.push_back(newleft);
						newargs.push_back(newright);
						const FOBDDFuncTerm* newterm = _manager->getFuncTerm(leftfuncterm->func(),newargs);
						return newterm->acceptchange(this);
					}
				}
				if(typeid(*rightterm) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* rightfuncterm = dynamic_cast<const FOBDDFuncTerm*>(rightterm);
					if(rightfuncterm->func()->name() == "+/2") {
						vector<const FOBDDArgument*> newleftargs;
						newleftargs.push_back(rightfuncterm->args(0));
						newleftargs.push_back(leftterm);
						vector<const FOBDDArgument*> newrightargs;
						newrightargs.push_back(rightfuncterm->args(1));
						newrightargs.push_back(leftterm);
						const FOBDDFuncTerm* newleft = _manager->getFuncTerm(functerm->func(),newleftargs);
						const FOBDDFuncTerm* newright = _manager->getFuncTerm(functerm->func(),newrightargs);
						vector<const FOBDDArgument*> newargs;
						newargs.push_back(newleft);
						newargs.push_back(newright);
						const FOBDDFuncTerm* newterm = _manager->getFuncTerm(rightfuncterm->func(),newargs);
						return newterm->acceptchange(this);
					}
				}
			}
			return FOBDDVisitor::change(functerm);
		}
};

const FOBDDAtomKernel* FOBDDManager::solve(const FOBDDKernel* kernel, const FOBDDVariable* var) {
	// TODO
	return 0;
}

const FOBDDAtomKernel* FOBDDManager::solve(const FOBDDKernel* kernel, const FOBDDDeBruijnIndex* index) {
	// TODO
	return 0;
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
int univNrAnswers(const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	int maxint = numeric_limits<int>::max();
	vector<SortTable*> vst; 
	for(set<const FOBDDVariable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) 
		vst.push_back(structure->inter((*it)->variable()->sort()));
	for(set<const FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it)
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
 * Return all kernels of the given bdd
 */
set<const FOBDDKernel*> FOBDDManager::allkernels(const FOBDD* bdd) {
	set<const FOBDDKernel*> result;
	if(bdd != _truebdd && bdd != _falsebdd) {
		set<const FOBDDKernel*> falsekernels = allkernels(bdd->falsebranch());
		set<const FOBDDKernel*> truekernels = allkernels(bdd->truebranch());
		result.insert(falsekernels.begin(),falsekernels.end());
		result.insert(truekernels.begin(),truekernels.end());
		result.insert(bdd->kernel());
		if(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel)) {
			set<const FOBDDKernel*> kernelkernels = allkernels(dynamic_cast<const FOBDDQuantKernel*>(bdd->kernel())->bdd());
			result.insert(kernelkernels.begin(),kernelkernels.end());
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
 * Return a mapping from the non-nested kernels of the given bdd to their estimated number of answers
 */
map<const FOBDDKernel*,double> FOBDDManager::kernelAnswers(const FOBDD* bdd, AbstractStructure* structure) {
	map<const FOBDDKernel*,double> result;
	set<const FOBDDKernel*> kernels = nonnestedkernels(bdd);
	for(set<const FOBDDKernel*>::const_iterator it = kernels.begin(); it != kernels.end(); ++it) {
		set<const FOBDDVariable*> vars = variables(*it);
		set<const FOBDDDeBruijnIndex*> indices = FOBDDManager::indices(*it);
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

map<const FOBDDKernel*,double> FOBDDManager::kernelUnivs(const FOBDD* bdd, AbstractStructure* structure) {
	map<const FOBDDKernel*,double> result;
	set<const FOBDDKernel*> kernels = nonnestedkernels(bdd);
	for(set<const FOBDDKernel*>::const_iterator it = kernels.begin(); it != kernels.end(); ++it) {
		set<const FOBDDVariable*> vars = variables(*it);
		set<const FOBDDDeBruijnIndex*> indices = FOBDDManager::indices(*it);
		result[*it] = univNrAnswers(vars,indices,structure);
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
		double univsize = 1;
		for(vector<const FOBDDArgument*>::const_iterator it = atomkernel->args().begin(); it != atomkernel->args().end(); ++it) {
			tablesize argsize = structure->inter((*it)->sort())->size();
			if(argsize.first) univsize = univsize * argsize.second;
			else {
				univsize = numeric_limits<double>::max();
				break;
			}
		}
		if(symbolsize.first) {
			if(univsize < numeric_limits<double>::max()) {
				chance = double(symbolsize.second) / univsize;
				if(chance > 1) chance = 1;
			}
			else chance = 0;
		}
		else {
			// TODO better estimators possible?
			if(univsize < numeric_limits<double>::max()) chance = 0.5;
			else chance = 0;
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
		map<const FOBDDKernel*,double> subkernels = kernelAnswers(quantkernel->bdd(),structure);
		map<const FOBDDKernel*,double> subunivs = kernelUnivs(quantkernel->bdd(),structure);

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
						double nodeunivsize = subunivs[paths[pathnr][nodenr].second];
						if(paths[pathnr][nodenr].first) 
							currchance = 
								currchance * dynsubkernels[paths[pathnr][nodenr].second] / double(nodeunivsize - element);
						else 
							currchance = 
								currchance * (nodeunivsize - element - dynsubkernels[paths[pathnr][nodenr].second]) / double(nodeunivsize - element);
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
							dynsubkernels[paths[chosenpathnr][nodenr].second] += -(1.0);
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
double FOBDDManager::estimatedNrAnswers(const FOBDDKernel* kernel, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
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
double FOBDDManager::estimatedNrAnswers(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	int maxint = numeric_limits<int>::max();
	double maxdouble = numeric_limits<double>::max();
	double bddchance = estimatedChance(bdd,structure);
	int univanswers = univNrAnswers(vars,indices,structure);
	if(univanswers == maxint) {
		return (bddchance > 0 ? maxdouble : 0);
	}
	else return bddchance * univanswers;
}

/**********************
	Cost estimators
**********************/

class TableCostEstimator : public StructureVisitor {
	private:
		const PredTable*	_table;
		vector<bool>		_pattern;	//!< _pattern[n] == true iff the n'th column is an input column
		double				_result;
	public:

		double run(const PredTable* t, const vector<bool>& p) {
			_table = t;
			_pattern = p;
			t->interntable()->accept(this);
			return _result;
		}

		void visit(const ProcInternalPredTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double sz = 1;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(!_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts.first) {
						if(sz * ts.second < maxdouble) {
							sz = sz * ts.second;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			// NOTE: We assume that evaluation of a lua function has cost 5
			// Can be adapted if this turns out to be unrealistic
			if(5 * sz < maxdouble) _result = 5 * sz;				
			else _result = maxdouble;
		}
		
		void visit(const BDDInternalPredTable*) {
			// TODO
		}

		void visit(const FullInternalPredTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double sz = 1;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(!_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts.first) {
						if(sz * ts.second < maxdouble) {
							sz = sz * ts.second;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			_result = sz;
		}

		void visit(const FuncInternalPredTable* t) {
			t->table()->interntable()->accept(this);
		}

		void visit(const UnionInternalPredTable* ) {
			// TODO
		}

		void visit(const UnionInternalSortTable* ) {
			// TODO
		}

		void visit(const EnumeratedInternalSortTable* ) {
			if(_pattern[0]) {
				_result = log(double(_table->size().second)) / log(2);
			}
			else {
				_result = _table->size().second;
			}
		}

		void visit(const IntRangeInternalSortTable* ) {
			if(_pattern[0]) _result = 1;
			else _result = _table->size().second;
		}

		void visit(const EnumeratedInternalPredTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double inputunivsize = 1;
			int inputsize = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) {
					++inputsize;
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts.first) {
						if(inputunivsize * ts.second < maxdouble) {
							inputunivsize = inputunivsize * ts.second;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			tablesize ts = _table->size();
			double lookupsize = inputunivsize < ts.second ? inputunivsize : ts.second;
			if(inputsize * log(lookupsize) / log(2) < maxdouble) {
				double lookuptime = inputsize * log(lookupsize) / log(2);
				double iteratetime = ts.second / inputunivsize;
				if(lookuptime + iteratetime < maxdouble) _result = lookuptime + iteratetime;
				else _result = maxdouble;
			}
			else _result = maxdouble;
		}

		void visit(const EqualInternalPredTable* ) {
			if(_pattern[0]) _result = 1;
			else if(_pattern[1]) _result = 1;
			else {
				tablesize ts = _table->universe().tables()[0]->size();
				if(ts.first) _result = ts.second;
				else _result = numeric_limits<double>::max();
			}
		}

		void visit(const StrLessInternalPredTable* ) {
			if(_pattern[0]) {
				if(_pattern[1]) _result = 1;
				else {
					tablesize ts = _table->universe().tables()[0]->size();
					if(ts.first) _result = ts.second / double(2);
					else _result = numeric_limits<double>::max();
				}
			}
			else if(_pattern[1]) {
				tablesize ts = _table->universe().tables()[0]->size();
				if(ts.first) _result = ts.second / double(2);
				else _result = numeric_limits<double>::max();
			}
			else {
				tablesize ts = _table->size();
				if(ts.first) _result = ts.second;
				else _result = numeric_limits<double>::max();
			}
		}

		void visit(const StrGreaterInternalPredTable* ) {
			if(_pattern[0]) {
				if(_pattern[1]) _result = 1;
				else {
					tablesize ts = _table->universe().tables()[0]->size();
					if(ts.first) _result = ts.second / double(2);
					else _result = numeric_limits<double>::max();
				}
			}
			else if(_pattern[1]) {
				tablesize ts = _table->universe().tables()[0]->size();
				if(ts.first) _result = ts.second / double(2);
				else _result = numeric_limits<double>::max();
			}
			else {
				tablesize ts = _table->size();
				if(ts.first) _result = ts.second;
				else _result = numeric_limits<double>::max();
			}
		}

		void visit(const InverseInternalPredTable* t) {
			if(typeid(*(t->table())) == typeid(InverseInternalPredTable)) {
				const InverseInternalPredTable* nt = dynamic_cast<const InverseInternalPredTable*>(t->table());
				nt->table()->accept(this);
			}
			else {
				double maxdouble = numeric_limits<double>::max();
				TableCostEstimator tce;
				PredTable npt(t->table(),_table->universe());
				double lookuptime = tce.run(&npt,vector<bool>(_pattern.size(),true));
				double sz = 1;
				for(unsigned int n = 0; n < _pattern.size(); ++n) {
					if(!_pattern[n]) {
						tablesize ts = _table->universe().tables()[n]->size();
						if(ts.first) {
							if(sz * ts.second < maxdouble) {
								sz = sz * ts.second;
							}
							else {
								_result = maxdouble;
								return;
							}
						}
						else {
							_result = maxdouble;
							return;
						}
					}
				}
				if(sz * lookuptime < maxdouble) _result = sz * lookuptime;
				else _result = maxdouble;
			}
		}

		void visit(const ProcInternalFuncTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double sz = 1;
			for(unsigned int n = 0; n < _pattern.size() - 1; ++n) {
				if(!_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts.first) {
						if(sz * ts.second < maxdouble) {
							sz = sz * ts.second;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			// NOTE: We assume that evaluation of a lua function has cost 5
			// Can be adapted if this turns out to be unrealistic
			if(5 * sz < maxdouble) _result = 5 * sz;				
			else _result = maxdouble;
		}

		void visit(const UNAInternalFuncTable* ) {
			if(_pattern.back()) {
				unsigned int patterncount = 0;
				for(unsigned int n = 0; n < _pattern.size() - 1; ++n) {
					if(_pattern[n]) ++patterncount;
				}
				_result = patterncount;
			}
			else {
				double maxdouble = numeric_limits<double>::max();
				double sz = 1;
				for(unsigned int n = 0; n < _pattern.size() - 1; ++n) {
					if(!_pattern[n]) {
						tablesize ts = _table->universe().tables()[n]->size();
						if(ts.first) {
							if(sz * ts.second < maxdouble) {
								sz = sz * ts.second;
							}
							else {
								_result = maxdouble;
								return;
							}
						}
						else {
							_result = maxdouble;
							return;
						}
					}
				}
				_result = sz;
			}
		}

		void visit(const EnumeratedInternalFuncTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double inputunivsize = 1;
			int inputsize = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) {
					++inputsize;
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts.first) {
						if(inputunivsize * ts.second < maxdouble) {
							inputunivsize = inputunivsize * ts.second;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			tablesize ts = _table->size();
			double lookupsize = inputunivsize < ts.second ? inputunivsize : ts.second;
			if(inputsize * log(lookupsize) / log(2) < maxdouble) {
				double lookuptime = inputsize * log(lookupsize) / log(2);
				double iteratetime = ts.second / inputunivsize;
				if(lookuptime + iteratetime < maxdouble) _result = lookuptime + iteratetime;
				else _result = maxdouble;
			}
			else _result = maxdouble;
		}

		void visit(const PlusInternalFuncTable* ) {
			unsigned int patterncount = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) ++patterncount;
			}
			if(patterncount >= 2) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const MinusInternalFuncTable* ) {
			unsigned int patterncount = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) ++patterncount;
			}
			if(patterncount >= 2) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const TimesInternalFuncTable* ) {
			unsigned int patterncount = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) ++patterncount;
			}
			if(patterncount >= 2) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const DivInternalFuncTable* ) {
			unsigned int patterncount = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) ++patterncount;
			}
			if(patterncount >= 2) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const AbsInternalFuncTable* ) {
			if(_pattern[0]) _result = 1;
			else if(_pattern[1]) _result = 2;
			else _result = numeric_limits<double>::max();
		}

		void visit(const UminInternalFuncTable* ) {
			if(_pattern[0] || _pattern[1]) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const ExpInternalFuncTable* ) {
			if(_pattern[0] && _pattern[1]) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const ModInternalFuncTable* ) {
			if(_pattern[0] && _pattern[1]) _result = 1;
			else _result = numeric_limits<double>::max();
		}
};

double FOBDDManager::estimatedCostAll(bool sign, const FOBDDKernel* kernel, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	double maxdouble = numeric_limits<double>::max();
	if(isArithmetic(kernel)) {
		vector<double> varunivsizes;
		vector<double> indexunivsizes;
		vector<const FOBDDVariable*> varsvector;
		vector<const FOBDDDeBruijnIndex*> indicesvector;
		unsigned int nrinfinite = 0;
		const FOBDDVariable* infinitevar = 0;
		const FOBDDDeBruijnIndex* infiniteindex = 0;
		for(set<const FOBDDVariable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
			varsvector.push_back(*it);
			SortTable* st = structure->inter((*it)->sort());
			tablesize stsize = st->size();
			if(stsize.first) varunivsizes.push_back(double(stsize.second));
			else { varunivsizes.push_back(maxdouble); ++nrinfinite; if(!infinitevar) infinitevar = *it; }
		}
		for(set<const FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
			indicesvector.push_back(*it);
			SortTable* st = structure->inter((*it)->sort());
			tablesize stsize = st->size();
			if(stsize.first) indexunivsizes.push_back(double(stsize.second));
			else { indexunivsizes.push_back(maxdouble); ++nrinfinite; if(!infiniteindex) infiniteindex = *it; }
		}
		if(nrinfinite > 1) {
			return maxdouble;
		}
		else if(nrinfinite == 1) {
			if(infinitevar) {
				if(!solve(kernel,infinitevar)) return maxdouble;
			}
			else {
				assert(infiniteindex);
				if(!solve(kernel,infiniteindex)) return maxdouble;
			}
			double result = 1;
			for(unsigned int n = 0; n < varsvector.size(); ++n) {
				if(varsvector[n] != infinitevar) {
					result = (result * varunivsizes[n] < maxdouble) ? (result * varunivsizes[n]) : maxdouble;
				}
			}
			for(unsigned int n = 0; n < indicesvector.size(); ++n) {
				if(indicesvector[n] != infiniteindex) {
					result = (result * indexunivsizes[n] < maxdouble) ? (result * indexunivsizes[n]) : maxdouble;
				}
			}
			return result;
		}
		else {
			double maxresult = 1;
			for(vector<double>::const_iterator it = varunivsizes.begin(); it != varunivsizes.end(); ++it) {
				maxresult = (maxresult * (*it) < maxdouble) ? (maxresult * (*it)) : maxdouble;
			}
			for(vector<double>::const_iterator it = indexunivsizes.begin(); it != indexunivsizes.end(); ++it) {
				maxresult = (maxresult * (*it) < maxdouble) ? (maxresult * (*it)) : maxdouble;
			}
			if(maxresult < maxdouble) {
				double bestresult = maxresult;
				for(unsigned int n = 0; n < varsvector.size(); ++n) {
					if(solve(kernel,varsvector[n])) {
						double currresult = maxresult / varunivsizes[n];
						if(currresult < bestresult) bestresult = currresult;
					}
				}
				for(unsigned int n = 0; n < indicesvector.size(); ++n) {
					if(solve(kernel,indicesvector[n])) {
						double currresult = maxresult / indexunivsizes[n];
						if(currresult < bestresult) bestresult = currresult;
					}
				}
				return bestresult;
			}
			else return maxdouble;
		}
	}
	else if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		PredInter* pinter;
		if(typeid(*symbol) == typeid(Predicate)) pinter = structure->inter(dynamic_cast<Predicate*>(symbol));
		else pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphinter();
		const PredTable* pt;
		if(sign) {
			if(atomkernel->type() == AKT_CF) pt = pinter->cf();
			else pt = pinter->ct();
		}
		else {
			if(atomkernel->type() == AKT_CF) pt = pinter->pt();
			else pt = pinter->pf();
		}

		vector<bool> pattern;
		for(vector<const FOBDDArgument*>::const_iterator it = atomkernel->args().begin(); it != atomkernel->args().end(); ++it) {
			bool input = true;
			for(set<const FOBDDVariable*>::const_iterator jt = vars.begin(); jt != vars.end(); ++jt) {
				if(contains(*it,*jt)) {
					input = false;
					break;
				}
			}
			if(input) {
				for(set<const FOBDDDeBruijnIndex*>::const_iterator jt = indices.begin(); jt != indices.end(); ++jt) {
					if((*it)->containsDeBruijnIndex((*jt)->index())) {
						input = false;
						break;
					}
				}
			}
			pattern.push_back(input);
		}
		
		TableCostEstimator tce;
		double result = tce.run(pt,pattern);
		return result;
	}
	else {
		// NOTE: implement a better estimator if backjumping on bdds is implemented
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		set<const FOBDDDeBruijnIndex*> newindices;
		for(set<const FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
			newindices.insert(getDeBruijnIndex((*it)->sort(),(*it)->index()+1));
		}
		newindices.insert(getDeBruijnIndex(quantkernel->sort(),0));
		double result = estimatedCostAll(quantkernel->bdd(),vars,newindices,structure);
		return result;
	}
}

double FOBDDManager::estimatedCostAll(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	int maxint = numeric_limits<int>::max();
	double maxdouble = numeric_limits<double>::max();
	if(bdd == _truebdd) {
		int univsize = univNrAnswers(vars,indices,structure);
		if(univsize == maxint) return maxdouble;
		else return double(univsize);
	}
	else if(bdd == _falsebdd) {
		return 1;
	}
	else {
		// split variables
		set<const FOBDDVariable*> kernelvars = variables(bdd->kernel());
		set<const FOBDDDeBruijnIndex*> kernelindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*> bddvars;
		set<const FOBDDDeBruijnIndex*> bddindices;
		for(set<const FOBDDVariable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
			if(kernelvars.find(*it) == kernelvars.end()) bddvars.insert(*it);
		}
		for(set<const FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
			if(kernelindices.find(*it) == kernelindices.end()) bddindices.insert(*it);
		}
		set<const FOBDDVariable*> removevars;
		set<const FOBDDDeBruijnIndex*> removeindices;
		for(set<const FOBDDVariable*>::const_iterator it = kernelvars.begin(); it != kernelvars.end(); ++it) {
			if(vars.find(*it) == vars.end()) removevars.insert(*it);
		}
		for(set<const FOBDDDeBruijnIndex*>::const_iterator it = kernelindices.begin(); it != kernelindices.end(); ++it) {
			if(indices.find(*it) == indices.end()) removeindices.insert(*it);
		}
		for(set<const FOBDDVariable*>::const_iterator it = removevars.begin(); it != removevars.end(); ++it) {
			kernelvars.erase(*it);
		}
		for(set<const FOBDDDeBruijnIndex*>::const_iterator it = removeindices.begin(); it != removeindices.end(); ++it) {
			kernelindices.erase(*it);
		}

		// recursive case
		if(bdd->falsebranch() == _falsebdd) {
			double kernelcost = estimatedCostAll(true,bdd->kernel(),kernelvars,kernelindices,structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(),kernelvars,kernelindices,structure);
			double truecost = estimatedCostAll(bdd->truebranch(),bddvars,bddindices,structure);
			if(kernelcost < maxdouble && 
			   kernelans < maxdouble && 
			   truecost < maxdouble &&
			   kernelcost + (kernelans * truecost) < maxdouble) {
				return kernelcost + (kernelans * truecost);
			}
			else return maxdouble;
		}
		else if(bdd->truebranch() == _falsebdd) {
			double kernelcost = estimatedCostAll(false,bdd->kernel(),kernelvars,kernelindices,structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(),kernelvars,kernelindices,structure);
			int kernelunivsize = univNrAnswers(kernelvars,kernelindices,structure);
			double invkernans = (kernelunivsize == maxint) ? maxdouble : double(kernelunivsize) - kernelans;
			double falsecost = estimatedCostAll(bdd->falsebranch(),bddvars,bddindices,structure);
			if( kernelcost + (invkernans * falsecost) < maxdouble) {
				return kernelcost + (invkernans * falsecost);
			}
			else return maxdouble;
		}
		else {
			int kernelunivsize = univNrAnswers(kernelvars,kernelindices,structure);
			set<const FOBDDVariable*> emptyvars;
			set<const FOBDDDeBruijnIndex*> emptyindices;
			double kernelcost = estimatedCostAll(true,bdd->kernel(),emptyvars,emptyindices,structure);
			double truecost = estimatedCostAll(bdd->truebranch(),bddvars,bddindices,structure);
			double falsecost = estimatedCostAll(bdd->falsebranch(),bddvars,bddindices,structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(),kernelvars,kernelindices,structure);
			if(kernelunivsize == maxint) return maxdouble;
			else {
				if((double(kernelunivsize) * kernelcost) + (double(kernelans) * truecost) + ((kernelunivsize - kernelans) * falsecost) < maxdouble) {
					return (double(kernelunivsize) * kernelcost) + (double(kernelans) * truecost) + ((kernelunivsize - kernelans) * falsecost);
				}
				else return maxdouble;
			}
		}
	}
}

void FOBDDManager::optimizequery(const FOBDD* query, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	if(query != _truebdd && query != _falsebdd) {
		set<const FOBDDKernel*> kernels = allkernels(query);
		for(set<const FOBDDKernel*>::const_iterator it = kernels.begin(); it != kernels.end(); ++it) {
			double bestscore = estimatedCostAll(query,vars,indices,structure);
			int bestposition = 0;
			// move upward
			while((*it)->number() != 0) {
				moveUp(*it);
				double currscore = estimatedCostAll(query,vars,indices,structure);
				if(currscore < bestscore) {
					bestscore = currscore;
					bestposition = 0;
				}
				else bestposition += 1;
			}
			// move downward
			while((*it)->number() < _kernels[(*it)->category()].size()-1) {
				moveDown(*it);
				double currscore = estimatedCostAll(query,vars,indices,structure);
				if(currscore < bestscore) {
					bestscore = currscore;
					bestposition = 0;
				}
				else bestposition += -1;
			}
			// move to best position
			if(bestposition < 0) {
				for(int n = 0; n > bestposition; --n) moveUp(*it);
			}
			else if(bestposition > 0) {
				for(int n = 0; n < bestposition; ++n) moveDown(*it);
			}
		}
	}
}


/**************
	Visitor
**************/

void FOBDDAtomKernel::accept(FOBDDVisitor* v)		const { v->visit(this);	}
void FOBDDQuantKernel::accept(FOBDDVisitor* v)		const { v->visit(this);	}
void FOBDDVariable::accept(FOBDDVisitor* v)			const { v->visit(this);	}
void FOBDDDeBruijnIndex::accept(FOBDDVisitor* v)	const { v->visit(this);	}
void FOBDDDomainTerm::accept(FOBDDVisitor* v)		const { v->visit(this);	}
void FOBDDFuncTerm::accept(FOBDDVisitor* v)			const { v->visit(this);	}

const FOBDDKernel*		FOBDDAtomKernel::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}
const FOBDDKernel*		FOBDDQuantKernel::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}
const FOBDDArgument*	FOBDDVariable::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}
const FOBDDArgument*	FOBDDDeBruijnIndex::acceptchange(FOBDDVisitor* v)	const { return v->change(this);	}
const FOBDDArgument*	FOBDDDomainTerm::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}
const FOBDDArgument*	FOBDDFuncTerm::acceptchange(FOBDDVisitor* v)		const { return v->change(this);	}

void FOBDDVisitor::visit(const FOBDD* bdd) {
	if(bdd != _manager->truebdd() && bdd != _manager->falsebdd()) {
		bdd->kernel()->accept(this);
		visit(bdd->truebranch());
		visit(bdd->falsebranch());
	}
}

void FOBDDVisitor::visit(const FOBDDAtomKernel* kernel) {
	for(vector<const FOBDDArgument*>::const_iterator it = kernel->args().begin(); it != kernel->args().end(); ++it) {
		(*it)->accept(this);
	}
}

void FOBDDVisitor::visit(const FOBDDQuantKernel* kernel) {
	visit(kernel->bdd());
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
	for(vector<const FOBDDArgument*>::const_iterator it = term->args().begin(); it != term->args().end(); ++it) {
		(*it)->accept(this);
	}
}

const FOBDD* FOBDDVisitor::change(const FOBDD* bdd) {
	if(_manager->isTruebdd(bdd)) return _manager->truebdd();
	else if(_manager->isFalsebdd(bdd)) return _manager->falsebdd();
	else {
		const FOBDDKernel* nk = bdd->kernel()->acceptchange(this);
		const FOBDD* nt = change(bdd->truebranch());
		const FOBDD* nf = change(bdd->falsebranch());
		return _manager->ifthenelse(nk,nt,nf);
	}
}

const FOBDDKernel* FOBDDVisitor::change(const FOBDDAtomKernel* kernel) {
	vector<const FOBDDArgument*>	nargs;
	for(vector<const FOBDDArgument*>::const_iterator it = kernel->args().begin(); it != kernel->args().end(); ++it) {
		nargs.push_back((*it)->acceptchange(this));
	}
	return _manager->getAtomKernel(kernel->symbol(),kernel->type(),nargs);
}

const FOBDDKernel* FOBDDVisitor::change(const FOBDDQuantKernel* kernel) {
	const FOBDD* nbdd = change(kernel->bdd());
	return _manager->getQuantKernel(kernel->sort(),nbdd);
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
	vector<const FOBDDArgument*>	nargs;
	for(vector<const FOBDDArgument*>::const_iterator it = term->args().begin(); it != term->args().end(); ++it) {
		nargs.push_back((*it)->acceptchange(this));
	}
	return _manager->getFuncTerm(term->func(),nargs);
}
