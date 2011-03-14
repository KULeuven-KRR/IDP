/************************************
	fobdd.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "fobdd.hpp"

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
	else if(_falsebranch->containsDeBruijnIndex(index)) return true;
	else if(_truebranch->containsDeBruijnIndex(index)) return true;
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
		return getAtomKernel(symbol,newargs);
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

FOBDDAtomKernel* FOBDDManager::getAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args) {
	// TODO: simplification

	// Lookup
	AtomKernelTable::const_iterator it = _atomkerneltable.find(symbol);
	if(it != _atomkerneltable.end()) {
		MVAGAK::const_iterator jt = it->second.find(args);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}

	// Lookup failed, create a new atom kernel
	return addAtomKernel(symbol,args);
}

FOBDDAtomKernel* FOBDDManager::addAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args) {
	FOBDDAtomKernel* newkernel = new FOBDDAtomKernel(symbol,args,newOrder(args));
	_atomkerneltable[symbol][args] = newkernel;
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

FOBDDDomainTerm* FOBDDManager::getDomainTerm(Sort* sort, TypedElement value) {
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

FOBDDDomainTerm* FOBDDManager::addDomainTerm(Sort* sort, TypedElement value) {
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

FOBDD* FOBDDManager::existsquantify(FOBDDVariable* var, FOBDD* bdd) {
	// TODO dynamic programming
	FOBDD* bumped = bump(var,bdd);
	return quantify(var->variable()->sort(),bumped);
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

/********************
	FOBDD Factory
********************/

void FOBDDFactory::visit(const VarTerm* vt) {
	_argument = _manager->getVariable(vt->var());
}

void FOBDDFactory::visit(const DomainTerm* dt) {
	TypedElement te;
	te._type = dt->type();
	te._element = dt->value();
	_argument = _manager->getDomainTerm(dt->sort(),te);
}

void FOBDDFactory::visit(const FuncTerm* ft) {
	vector<FOBDDArgument*> args(ft->func()->arity());
	for(unsigned int n = 0; n < args.size(); ++n) {
		ft->subterm(n)->accept(this);
		args[n] = _argument;
	}
	_argument = _manager->getFuncTerm(ft->func(),args);
}

void FOBDDFactory::visit(const AggTerm* ) {
	// TODO
	assert(false);
}

void FOBDDFactory::visit(const PredForm* pf) {
	vector<FOBDDArgument*> args(pf->symb()->nrSorts());
	for(unsigned int n = 0; n < args.size(); ++n) {
		pf->subterm(n)->accept(this);
		args[n] = _argument;
	}
	_kernel = _manager->getAtomKernel(pf->symb(),args);
	if(pf->sign()) _bdd = _manager->getBDD(_kernel,_manager->truebdd(),_manager->falsebdd());
	else  _bdd = _manager->getBDD(_kernel,_manager->falsebdd(),_manager->truebdd());
}

void FOBDDFactory::visit(const BoolForm* bf) {
	if(bf->conj()) {
		FOBDD* temp = _manager->truebdd();
		for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
			bf->subform(n)->accept(this);
			temp = _manager->conjunction(temp,_bdd);
		}
		_bdd = temp;
	}
	else {
		FOBDD* temp = _manager->falsebdd();
		for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
			bf->subform(n)->accept(this);
			temp = _manager->disjunction(temp,_bdd);
		}
		_bdd = temp;
	}
	_bdd = bf->sign() ? _bdd : _manager->negation(_bdd);
}

void FOBDDFactory::visit(const QuantForm* qf) {
	qf->subf()->accept(this);
	FOBDD* qbdd = _bdd;
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		FOBDDVariable* qvar = _manager->getVariable(qf->qvar(n));
		if(qf->univ()) qbdd = _manager->univquantify(qvar,qbdd);
		else qbdd = _manager->existsquantify(qvar,qbdd);
	}
	_bdd = qf->sign() ? qbdd : _manager->negation(qbdd);
}

void FOBDDFactory::visit(const EqChainForm* ) {
	// TODO
	assert(false);
}

void FOBDDFactory::visit(const AggForm* ) {
	// TODO
	assert(false);
}

/****************
	Debugging
****************/

string FOBDDManager::to_string(FOBDD* bdd, unsigned int spaces) const {
	string str = tabstring(spaces);
	if(bdd == _truebdd) str += "true\n";
	else if(bdd == _falsebdd) str += "false\n";
	else {
		str += to_string(bdd->kernel(),spaces);
		str += tabstring(spaces) + string("FALSE BRANCH:\n");
		str += to_string(bdd->falsebranch(),spaces+3);
		str += tabstring(spaces) + string("TRUE BRANCH:\n");
		str += to_string(bdd->truebranch(),spaces+3);
	}
	return str;
}

string FOBDDManager::to_string(FOBDDKernel* kernel, unsigned int spaces) const {
	string str = tabstring(spaces);
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		FOBDDAtomKernel* atomkernel = dynamic_cast<FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		str += symbol->to_string();
		if(symbol->ispred()) {
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
		str += tabstring(spaces) + "}";
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
		str += ElementUtil::ElementToString(dt->value()) + "[" + dt->sort()->to_string() + "]";
	}
	else {
		assert(false);
	}
	return str;
}
