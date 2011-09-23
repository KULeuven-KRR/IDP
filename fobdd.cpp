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
#include "error.hpp"


using namespace std;

extern int global_seed;

/*******************
	Kernel order
*******************/

static unsigned int STANDARDCATEGORY = 1;
static unsigned int DEBRUIJNCATEGORY = 2;
static unsigned int TRUEFALSECATEGORY = 3;

bool FOBDDKernel::operator<(const FOBDDKernel& k) const {
	if(_order._category < k._order._category) { return true; }
	else if(_order._category > k._order._category) { return false; }
	else { return _order._number < k._order._number; }
}

bool FOBDDKernel::operator>(const FOBDDKernel& k) const {
	if(_order._category > k._order._category) { return true; }
	else if(_order._category < k._order._category) { return false; }
	else { return _order._number > k._order._number; }
}

KernelOrder FOBDDManager::newOrder(unsigned int category) {
	KernelOrder order(category,_nextorder[category]);
	++_nextorder[category];
	return order;
}

KernelOrder FOBDDManager::newOrder(const vector<const FOBDDArgument*>& args) {
	unsigned int category = STANDARDCATEGORY;
	for(size_t n = 0; n < args.size(); ++n) {
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

void FOBDDManager::clearDynamicTables() {
	_negationtable.clear();
	_conjunctiontable.clear();
	_disjunctiontable.clear();
	_ifthenelsetable.clear();
	_quanttable.clear();
}

void FOBDDManager::moveUp(const FOBDDKernel* kernel) {
	clearDynamicTables();
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
	clearDynamicTables();
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
				if(_bddtable[kernel][falseerase[n]].empty()) { _bddtable[kernel].erase(falseerase[n]); }
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


bool FactorSWOrdering(const FOBDDArgument* arg1, const FOBDDArgument* arg2) {
	if(typeid(*arg1) == typeid(FOBDDDomainTerm)) {
		if(typeid(*arg2) == typeid(FOBDDDomainTerm)) { return arg1 < arg2; }
		else { return true; }
	}
	else if(typeid(*arg2) == typeid(FOBDDDomainTerm)) { return false; }
	else { return arg1 < arg2; }
}

class FlatMult : public FOBDDVisitor {
	private:
		vector<const FOBDDArgument*>	_result;

		void avoidempty() {
			if(_result.empty()) {
				_result.push_back(_manager->getDomainTerm(VocabularyUtils::natsort(),DomainElementFactory::instance()->create(1)));
			}
		}
		void visit(const FOBDDDomainTerm* dt) { 
			_result.push_back(dt); 
		}
		void visit(const FOBDDVariable* v) {
			avoidempty();
			_result.push_back(v);
		}
		void visit(const FOBDDDeBruijnIndex* i) {
			avoidempty();
			_result.push_back(i);
		}
		void visit(const FOBDDFuncTerm* ft) {
			if(ft->func()->name() == "*/2" && Vocabulary::std()->contains(ft->func())) {
				ft->args(0)->accept(this);
				_result.push_back(ft->args(1));
			}
			else {
				avoidempty();
				_result.push_back(ft);
			}
		}
	public:
		FlatMult(FOBDDManager* m) : FOBDDVisitor(m) { }

		vector<const FOBDDArgument*> run(const FOBDDArgument* arg) {
			_result.clear();
			arg->accept(this);
			return _result;
		}
};

class FlatAdd : public FOBDDVisitor {
	private:
		vector<const FOBDDArgument*>	_result;

		void avoidempty() {
			if(_result.empty()) {
				_result.push_back(_manager->getDomainTerm(VocabularyUtils::natsort(),DomainElementFactory::instance()->create(0)));
			}
		}
		void visit(const FOBDDDomainTerm* dt) { 
			_result.push_back(dt); 
		}
		void visit(const FOBDDVariable* v) {
			avoidempty();
			_result.push_back(v);
		}
		void visit(const FOBDDDeBruijnIndex* i) {
			avoidempty();
			_result.push_back(i);
		}
		void visit(const FOBDDFuncTerm* ft) {
			if(ft->func()->name() == "+/2" && Vocabulary::std()->contains(ft->func())) {
				ft->args(0)->accept(this);
				_result.push_back(ft->args(1));
			}
			else {
				avoidempty();
				_result.push_back(ft);
			}
		}
	public:
		FlatAdd(FOBDDManager* m) : FOBDDVisitor(m) { }

		vector<const FOBDDArgument*> run(const FOBDDArgument* arg) {
			_result.clear();
			arg->accept(this);
			return _result;
		}
};

bool TermSWOrdering(const FOBDDArgument* arg1, const FOBDDArgument* arg2,FOBDDManager* manager) {
	FlatMult fa(manager);
	vector<const FOBDDArgument*> flat1 = fa.run(arg1);
	vector<const FOBDDArgument*> flat2 = fa.run(arg2);
	if(flat1.size() < flat2.size()) { return true; }
	else if(flat1.size() > flat2.size()) { return false; }
	else {
		for(size_t n = 1; n < flat1.size(); ++n) {
			if(FactorSWOrdering(flat1[n],flat2[n])) { return true; }
			else if(FactorSWOrdering(flat2[n],flat1[n])) { return false; }
		}
		return false;
	}
}

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
	for(size_t n = 0; n < _args.size(); ++n) {
		if(_args[n]->containsDeBruijnIndex(index)) { return true; }
	}
	return false;
}

bool FOBDDAtomKernel::containsDeBruijnIndex(unsigned int index) const {
	for(size_t n = 0; n < _args.size(); ++n) {
		if(_args[n]->containsDeBruijnIndex(index)) { return true; }
	}
	return false;
}

bool FOBDDQuantKernel::containsDeBruijnIndex(unsigned int index) const {
	return _bdd->containsDeBruijnIndex(index+1);
}

bool FOBDD::containsDeBruijnIndex(unsigned int index) const {
	if(_kernel->containsDeBruijnIndex(index)) { return true; }
	else if(_falsebranch && _falsebranch->containsDeBruijnIndex(index)) { return true; }
	else if(_truebranch && _truebranch->containsDeBruijnIndex(index)) { return true; }
	else { return false; }
}

class Bump : public FOBDDVisitor {
	private:
		unsigned int			_depth;
		const FOBDDVariable*	_variable;
	public:
		Bump(FOBDDManager* manager, const FOBDDVariable* variable, unsigned int depth) :
			FOBDDVisitor(manager), _depth(depth), _variable(variable) { }

		const FOBDDKernel*	change(const FOBDDQuantKernel* kernel) {
			++_depth;
			const FOBDD* nbdd = FOBDDVisitor::change(kernel->bdd());
			--_depth;
			return _manager->getQuantKernel(kernel->sort(),nbdd);
		}

		const FOBDDArgument* change(const FOBDDVariable* var) {
			if(var == _variable) { return _manager->getDeBruijnIndex(var->variable()->sort(),_depth); }
			else { return var; }
		}

		const FOBDDArgument* change(const FOBDDDeBruijnIndex* dbr) {
			if(_depth <= dbr->index()) { return _manager->getDeBruijnIndex(dbr->sort(),dbr->index()+1); }
			else { return dbr; }
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
	_truekernel = new FOBDDKernel(ktrue);
	_falsekernel = new FOBDDKernel(kfalse);
	_truebdd = new FOBDD(_truekernel,0,0);
	_falsebdd = new FOBDD(_falsekernel,0,0);
}

const FOBDD* FOBDDManager::getBDD(const FOBDDKernel* kernel,const FOBDD* truebranch,const FOBDD* falsebranch) {
	// Simplification
	if(kernel == _truekernel) { return truebranch; }
	if(kernel == _falsekernel) { return falsebranch; }
	if(falsebranch == truebranch) { return falsebranch; }

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
			if(bdd == _originalmanager->truebdd()) { return _copymanager->truebdd(); }
			else if(bdd == _originalmanager->falsebdd()) { return _copymanager->falsebdd(); }
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

const FOBDDArgument* FOBDDManager::invert(const FOBDDArgument* arg) {
	const DomainElement* minus_one = DomainElementFactory::instance()->create(-1);
	const FOBDDArgument* minus_one_term = getDomainTerm(VocabularyUtils::intsort(),minus_one);
	Function* times = Vocabulary::std()->func("*/2");
	times = times->disambiguate(vector<Sort*>(3,SortUtils::resolve(VocabularyUtils::intsort(),arg->sort())),0);
	vector<const FOBDDArgument*> timesterms(2); timesterms[0] = minus_one_term; timesterms[1] = arg;
	return getFuncTerm(times,timesterms);
}

const FOBDDKernel* FOBDDManager::getAtomKernel(PFSymbol* symbol,AtomKernelType akt, const vector<const FOBDDArgument*>& args) {
	// Simplification
	if(symbol->name() == "=/2") {
		if(args[0] == args[1]) { return _truekernel; }
	}
	else if(args.size() == 1) {
		if(symbol->sorts()[0]->pred() == symbol) {
			if(SortUtils::isSubsort(args[0]->sort(),symbol->sorts()[0])) { return _truekernel; }
		}
	}

	// Arithmetic rewriting
	// 1. Remove functions
	if(typeid(*symbol) == typeid(Function) && akt == AKT_TWOVAL) {
		Function* f = dynamic_cast<Function*>(symbol);
		Sort* s = SortUtils::resolve(f->outsort(),args.back()->sort());
		Predicate* equal = VocabularyUtils::equal(s);
		vector<const FOBDDArgument*> funcargs = args; funcargs.pop_back();
		const FOBDDArgument* functerm = getFuncTerm(f,funcargs);
		vector<const FOBDDArgument*> newargs;
		newargs.push_back(functerm); newargs.push_back(args.back());
		return getAtomKernel(equal,AKT_TWOVAL,newargs);
	}
	// 2. Move all arithmetic terms to the lefthand side of an (in)equality
	if(VocabularyUtils::isComparisonPredicate(symbol)) {
		const FOBDDArgument* leftarg = args[0];
		const FOBDDArgument* rightarg = args[1];
		if(VocabularyUtils::isNumeric(rightarg->sort())) {
			assert(VocabularyUtils::isNumeric(leftarg->sort()));
			if(typeid(*rightarg) != typeid(FOBDDDomainTerm) || dynamic_cast<const FOBDDDomainTerm*>(rightarg)->value() != DomainElementFactory::instance()->create(0)) {
				const DomainElement* zero = DomainElementFactory::instance()->create(0);
				const FOBDDDomainTerm* zero_term = getDomainTerm(VocabularyUtils::natsort(),zero);
				const FOBDDArgument* minus_rightarg = invert(rightarg);
				Function* plus = Vocabulary::std()->func("+/2");
				plus = plus->disambiguate(vector<Sort*>(3,SortUtils::resolve(leftarg->sort(),rightarg->sort())),0);
				assert(plus);
				vector<const FOBDDArgument*> plusargs(2); plusargs[0] = leftarg; plusargs[1] = minus_rightarg;
				const FOBDDArgument* plusterm = getFuncTerm(plus,plusargs);
				vector<const FOBDDArgument*> newargs(2); newargs[0] = plusterm; newargs[1] = zero_term;
				return getAtomKernel(symbol,akt,newargs);
			}
		}
	}

	// Comparison rewriting
	if(VocabularyUtils::isComparisonPredicate(symbol)) {
		if(FactorSWOrdering(args[0],args[1])) {
			vector<const FOBDDArgument*> newargs(2); newargs[0] = args[1]; newargs[1] = args[0];
			Predicate* newsymbol = dynamic_cast<Predicate*>(symbol);
			if(symbol->name() == "</2") { newsymbol = VocabularyUtils::greaterThan(symbol->sorts()[0]); }
			else if(symbol->name() == ">/2") { newsymbol = VocabularyUtils::lessThan(symbol->sorts()[0]); }
			return getAtomKernel(newsymbol,akt,newargs);
		}
	}


	// Lookup
	AtomKernelTable::const_iterator it = _atomkerneltable.find(symbol);
	if(it != _atomkerneltable.end()) {
		MAKTMVAGAK::const_iterator jt = it->second.find(akt);
		if(jt != it->second.end()) {
			MVAGAK::const_iterator kt = jt->second.find(args);
			if(kt != jt->second.end()) {
				return kt->second;
			}
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

const FOBDDKernel* FOBDDManager::getQuantKernel(Sort* sort,const FOBDD* bdd) {
	// Simplification
	if(bdd == _truebdd) { return _truekernel; }
	else if(bdd == _falsebdd) { return _falsekernel; }
	else if(longestbranch(bdd) == 2) {
		const FOBDDDeBruijnIndex* qvar = getDeBruijnIndex(sort,0);
		const FOBDDArgument* arg = solve(bdd->kernel(),qvar);
		if(arg && !partial(arg)) {
			if((bdd->truebranch() == _truebdd && SortUtils::isSubsort(arg->sort(),sort)) 
				|| sort->builtin()) { return _truekernel;	 }
				// NOTE: sort->builtin() is used here as an approximate test to see if the sort contains more than
				// one domain element. If that is the case, (? y : F(x) ~= y) is indeed true.
		}
	}

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

const FOBDDArgument* FOBDDManager::getFuncTerm(Function* func, const vector<const FOBDDArgument*>& args) {

//cerr << "Get functerm on function " << *func << " and arguments ";
//for(auto it = args.begin(); it != args.end(); ++it) {
//	put(cerr,*it); cerr << "   ";
//}
//cerr << endl;

	// Arithmetic rewriting
	// 1. Remove unary minus
	if(func->name() == "-/1" && Vocabulary::std()->contains(func)) { return invert(args[0]); }
	// 2. Remove binary minus
	if(func->name() == "-/2" && Vocabulary::std()->contains(func)) {
		const FOBDDArgument* invright = invert(args[1]);
		Function* plus = Vocabulary::std()->func("+/2");
		plus = plus->disambiguate(vector<Sort*>(3,SortUtils::resolve(args[0]->sort(),invright->sort())),0);
		vector<const FOBDDArgument*> newargs(2); newargs[0] = args[0]; newargs[1] = invright;
		return getFuncTerm(plus,newargs);
	}
	if(func->name() == "*/2" && Vocabulary::std()->contains(func)) {
		// 3. Execute computable multiplications
		if(typeid(*(args[0])) == typeid(FOBDDDomainTerm)) {
			const FOBDDDomainTerm* leftterm = dynamic_cast<const FOBDDDomainTerm*>(args[0]);
			if(leftterm->value()->type() == DET_INT) {
				if(leftterm->value()->value()._int == 0) { return leftterm; }
				else if(leftterm->value()->value()._int == 1) { return args[1]; }
			}
			if(typeid(*(args[1])) == typeid(FOBDDDomainTerm)) {
				const FOBDDDomainTerm* rightterm = dynamic_cast<const FOBDDDomainTerm*>(args[1]);
				FuncInter* fi = func->interpretation(0);
				vector<const DomainElement*> multargs(2);
				multargs[0] = leftterm->value();
				multargs[1] = rightterm->value();
				const DomainElement* result = fi->funcTable()->operator[](multargs);
				return getDomainTerm(func->outsort(),result);
			}
		}
		// 4. Apply distributivity of */2 with respect to +/2
		if(typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
			if(leftterm->func()->name() == "+/2" && Vocabulary::std()->contains(leftterm->func())) {
				vector<const FOBDDArgument*> newleftargs(2);
				newleftargs[0] = leftterm->args(0); newleftargs[1] = args[1];
				vector<const FOBDDArgument*> newrightargs(2);
				newrightargs[0] = leftterm->args(1); newrightargs[1] = args[1];
				const FOBDDArgument* newleft = getFuncTerm(func,newleftargs);
				const FOBDDArgument* newright = getFuncTerm(func,newrightargs);
				vector<const FOBDDArgument*> newargs(2); newargs[0] = newleft; newargs[1] = newright;
				return getFuncTerm(leftterm->func(),newargs);
			}
		}
		if(typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
			if(rightterm->func()->name() == "+/2" && Vocabulary::std()->contains(rightterm->func())) {
				vector<const FOBDDArgument*> newleftargs(2);
				newleftargs[0] = args[0]; newleftargs[1] = rightterm->args(0);
				vector<const FOBDDArgument*> newrightargs(2);
				newrightargs[0] = args[0]; newrightargs[1] = rightterm->args(1);
				const FOBDDArgument* newleft = getFuncTerm(func,newleftargs);
				const FOBDDArgument* newright = getFuncTerm(func,newrightargs);
				vector<const FOBDDArgument*> newargs(2); newargs[0] = newleft; newargs[1] = newright;
				return getFuncTerm(rightterm->func(),newargs);
			}
		}
		// 5. Apply commutativity and associativity to obtain
		// a sorted multiplication of the form ((((t1 * t2) * t3) * t4) * ...)
		if(typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
			if(leftterm->func()->name() == "*/2" && Vocabulary::std()->contains(leftterm->func())) {
				if(typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
					if(rightterm->func()->name() == "*/2" && Vocabulary::std()->contains(rightterm->func())) {
						Function* times = Vocabulary::std()->func("*/2");
						Function* times1 = times->disambiguate(vector<Sort*>(3,SortUtils::resolve(leftterm->sort(),rightterm->args(1)->sort())),0);
						vector<const FOBDDArgument*> leftargs(2); leftargs[0] = leftterm; leftargs[1] = rightterm->args(1);
						const FOBDDArgument* newleft = getFuncTerm(times1,leftargs);
						Function* times2 = times->disambiguate(vector<Sort*>(3,SortUtils::resolve(newleft->sort(),rightterm->args(0)->sort())),0);
						vector<const FOBDDArgument*> newargs(2); newargs[0] = newleft; newargs[1] = rightterm->args(0);
						return getFuncTerm(times2,newargs);
					}
				}
				if(FactorSWOrdering(args[1],leftterm->args(1))) {
					Function* times = Vocabulary::std()->func("*/2");
					Function* times1 = times->disambiguate(vector<Sort*>(3,SortUtils::resolve(args[1]->sort(),leftterm->args(0)->sort())),0);
					vector<const FOBDDArgument*> leftargs(2); leftargs[0] = leftterm->args(0); leftargs[1] = args[1];
					const FOBDDArgument* newleft = getFuncTerm(times1,leftargs);
					Function* times2 = times->disambiguate(vector<Sort*>(3,SortUtils::resolve(newleft->sort(),leftterm->args(1)->sort())),0);
					vector<const FOBDDArgument*> newargs(2); newargs[0] = newleft; newargs[1] = leftterm->args(1);
					return getFuncTerm(times2,newargs);
				}
			}
		}
		else if(typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
			if(rightterm->func()->name() == "*/2" && Vocabulary::std()->contains(rightterm->func())) {
				vector<const FOBDDArgument*> newargs(2); newargs[0] = args[1]; newargs[1] = args[0];
				return getFuncTerm(func,newargs);
			}
			else if(FactorSWOrdering(args[1],args[0])) {
				vector<const FOBDDArgument*> newargs(2); newargs[0] = args[1]; newargs[1] = args[0];
				return getFuncTerm(func,newargs);
			}
		}
		else if(FactorSWOrdering(args[1],args[0])) {
				vector<const FOBDDArgument*> newargs(2); newargs[0] = args[1]; newargs[1] = args[0];
				return getFuncTerm(func,newargs);
		}
	}
	else if(func->name() == "+/2" && Vocabulary::std()->contains(func)) {
		// 6. Execute computable additions
		if(typeid(*(args[0])) == typeid(FOBDDDomainTerm)) {
			const FOBDDDomainTerm* leftterm = dynamic_cast<const FOBDDDomainTerm*>(args[0]);
			if(leftterm->value()->type() == DET_INT && leftterm->value()->value()._int == 0) { return args[1]; }
			if(typeid(*(args[1])) == typeid(FOBDDDomainTerm)) {
				const FOBDDDomainTerm* rightterm = dynamic_cast<const FOBDDDomainTerm*>(args[1]);
				FuncInter* fi = func->interpretation(0);
				vector<const DomainElement*> plusargs(2);
				plusargs[0] = leftterm->value();
				plusargs[1] = rightterm->value();
				const DomainElement* result = fi->funcTable()->operator[](plusargs);
				return getDomainTerm(func->outsort(),result);
			}
		}

		// 7. Apply commutativity and associativity to 
		// obtain a sorted addition of the form ((((t1 + t2) + t3) + t4) + ...)
		if(typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
			if(leftterm->func()->name() == "+/2" && Vocabulary::std()->contains(leftterm->func())) {
				if(typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
					if(rightterm->func()->name() == "+/2" && Vocabulary::std()->contains(rightterm->func())) {
						Function* plus = Vocabulary::std()->func("+/2");
						Function* plus1 = plus->disambiguate(vector<Sort*>(3,SortUtils::resolve(leftterm->sort(),rightterm->args(1)->sort())),0);
						vector<const FOBDDArgument*> leftargs(2); leftargs[0] = leftterm; leftargs[1] = rightterm->args(1);
						const FOBDDArgument* newleft = getFuncTerm(plus1,leftargs);
						Function* plus2 = plus->disambiguate(vector<Sort*>(3,SortUtils::resolve(newleft->sort(),rightterm->args(0)->sort())),0);
						vector<const FOBDDArgument*> newargs(2); newargs[0] = newleft; newargs[1] = rightterm->args(0);
						return getFuncTerm(plus2,newargs);
					}
				}
				if(TermSWOrdering(args[1],leftterm->args(1),this)) {
					Function* plus = Vocabulary::std()->func("+/2");
					Function* plus1 = plus->disambiguate(vector<Sort*>(3,SortUtils::resolve(args[1]->sort(),leftterm->args(0)->sort())),0);
					vector<const FOBDDArgument*> leftargs(2); leftargs[0] = leftterm->args(0); leftargs[1] = args[1];
					const FOBDDArgument* newleft = getFuncTerm(plus1,leftargs);
					Function* plus2 = plus->disambiguate(vector<Sort*>(3,SortUtils::resolve(newleft->sort(),leftterm->args(1)->sort())),0);
					vector<const FOBDDArgument*> newargs(2); newargs[0] = newleft; newargs[1] = leftterm->args(1);
					return getFuncTerm(plus2,newargs);
				}
			}
		}
		else if(typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
			if(rightterm->func()->name() == "+/2" && Vocabulary::std()->contains(rightterm->func())) {
				vector<const FOBDDArgument*> newargs(2); newargs[0] = args[1]; newargs[1] = args[0];
				return getFuncTerm(func,newargs);
			}
			else if(TermSWOrdering(args[1],args[0],this)) {
				vector<const FOBDDArgument*> newargs(2); newargs[0] = args[1]; newargs[1] = args[0];
				return getFuncTerm(func,newargs);
			}
		}
		else if(TermSWOrdering(args[1],args[0],this)) {
				vector<const FOBDDArgument*> newargs(2); newargs[0] = args[1]; newargs[1] = args[0];
				return getFuncTerm(func,newargs);
		}

		// 8. Add terms with the same non-constant part
		const FOBDDArgument* left = 0;
		const FOBDDArgument* right = 0;
		if(typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
			if(leftterm->func()->name() == "+/2" && Vocabulary::std()->contains(leftterm->func())) {
				left = leftterm->args(0);
				right = leftterm->args(1);
			}
			else { right = args[0]; }
		}
		else right = args[0];
		FlatMult flatadd(this);
		vector<const FOBDDArgument*> leftflat = flatadd.run(right);
		vector<const FOBDDArgument*> rightflat = flatadd.run(args[1]);
		if(leftflat.size() == rightflat.size()) {
			unsigned int n = 1;
			for(; n < leftflat.size(); ++n) {
				if(leftflat[n] != rightflat[n]) break;
			}
			if(n == leftflat.size()) {
				Function* plus = Vocabulary::std()->func("+/2");
				plus = plus->disambiguate(vector<Sort*>(3,SortUtils::resolve(leftflat[0]->sort(),rightflat[0]->sort())),0);
				vector<const FOBDDArgument*> firstargs(2); firstargs[0] = leftflat[0]; firstargs[1] = rightflat[0];
				const FOBDDArgument* currterm = getFuncTerm(plus,firstargs);
				for(unsigned int m = 1; m < leftflat.size(); ++m) {
					Function* times = Vocabulary::std()->func("*/2");
					times = times->disambiguate(vector<Sort*>(3,SortUtils::resolve(currterm->sort(),leftflat[m]->sort())),0);
					vector<const FOBDDArgument*> nextargs(2); nextargs[0] = currterm; nextargs[1] = leftflat[m];
					currterm = getFuncTerm(times,nextargs);
				}
				if(left) {
					Function* plus1 = Vocabulary::std()->func("+/2");
					plus1 = plus1->disambiguate(vector<Sort*>(3,SortUtils::resolve(currterm->sort(),left->sort())),0);
					vector<const FOBDDArgument*> lastargs(2); lastargs[0] = left; lastargs[1] = currterm;
					return getFuncTerm(plus1,lastargs);
				}
				else { return currterm; }
			}
		}
	}
	
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
	// Base cases
	if(bdd == _truebdd) { return _falsebdd; }
	if(bdd == _falsebdd) { return _truebdd; }

	// Recursive case
	map<const FOBDD*,const FOBDD*>::iterator it = _negationtable.find(bdd);
	if(it != _negationtable.end()) { return it->second; }
	else {
		const FOBDD* falsebranch = negation(bdd->falsebranch());
		const FOBDD* truebranch = negation(bdd->truebranch());
		const FOBDD* result = getBDD(bdd->kernel(),truebranch,falsebranch);
		_negationtable[bdd] = result;
		return result;
	}
}

const FOBDD* FOBDDManager::conjunction(const FOBDD* bdd1, const FOBDD* bdd2) {
	// Base cases
	if(bdd1 == _falsebdd || bdd2 == _falsebdd) { return _falsebdd; }
	if(bdd1 == _truebdd) { return bdd2; }
	if(bdd2 == _truebdd) { return bdd1; }
	if(bdd1 == bdd2) { return bdd1; }

	// Recursive case
	if(bdd2 < bdd1) { const FOBDD* temp = bdd1; bdd1 = bdd2; bdd2 = temp; }
	map<const FOBDD*,map<const FOBDD*,const FOBDD*> >::iterator it = _conjunctiontable.find(bdd1);
	if(it != _conjunctiontable.end()) {
		map<const FOBDD*,const FOBDD*>::iterator jt = it->second.find(bdd2);
		if(jt != it->second.end()) { return jt->second; }
	}
	const FOBDD* result = 0;
	if(*(bdd1->kernel()) < *(bdd2->kernel())) {
		const FOBDD* falsebranch = conjunction(bdd1->falsebranch(),bdd2);
		const FOBDD* truebranch = conjunction(bdd1->truebranch(),bdd2);
		result = getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
	else if(*(bdd1->kernel()) > *(bdd2->kernel())) {
		const FOBDD* falsebranch = conjunction(bdd1,bdd2->falsebranch());
		const FOBDD* truebranch = conjunction(bdd1,bdd2->truebranch());
		result = getBDD(bdd2->kernel(),truebranch,falsebranch);
	}
	else {
		assert(bdd1->kernel() == bdd2->kernel());
		const FOBDD* falsebranch = conjunction(bdd1->falsebranch(),bdd2->falsebranch());
		const FOBDD* truebranch = conjunction(bdd1->truebranch(),bdd2->truebranch());
		result = getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
	_conjunctiontable[bdd1][bdd2] = result;
	return result;
}

const FOBDD* FOBDDManager::disjunction(const FOBDD* bdd1, const FOBDD* bdd2) {
	// Base cases
	if(bdd1 == _truebdd || bdd2 == _truebdd) { return _truebdd; }
	if(bdd1 == _falsebdd) { return bdd2; }
	if(bdd2 == _falsebdd) { return bdd1; }
	if(bdd1 == bdd2) { return bdd1; }

	// Recursive case
	if(bdd2 < bdd1) { const FOBDD* temp = bdd1; bdd1 = bdd2; bdd2 = temp; }
	map<const FOBDD*,map<const FOBDD*,const FOBDD*> >::iterator it = _disjunctiontable.find(bdd1);
	if(it != _disjunctiontable.end()) {
		map<const FOBDD*,const FOBDD*>::iterator jt = it->second.find(bdd2);
		if(jt != it->second.end()) { return jt->second; }
	}
	const FOBDD* result = 0;
	if(*(bdd1->kernel()) < *(bdd2->kernel())) {
		const FOBDD* falsebranch = disjunction(bdd1->falsebranch(),bdd2);
		const FOBDD* truebranch = disjunction(bdd1->truebranch(),bdd2);
		result = getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
	else if(*(bdd1->kernel()) > *(bdd2->kernel())) {
		const FOBDD* falsebranch = disjunction(bdd1,bdd2->falsebranch());
		const FOBDD* truebranch = disjunction(bdd1,bdd2->truebranch());
		result = getBDD(bdd2->kernel(),truebranch,falsebranch);
	}
	else {
		assert(bdd1->kernel() == bdd2->kernel());
		const FOBDD* falsebranch = disjunction(bdd1->falsebranch(),bdd2->falsebranch());
		const FOBDD* truebranch = disjunction(bdd1->truebranch(),bdd2->truebranch());
		result = getBDD(bdd1->kernel(),truebranch,falsebranch);
	}
	_disjunctiontable[bdd1][bdd2] = result;
	return result;
}

const FOBDD* FOBDDManager::ifthenelse(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	auto it = _ifthenelsetable.find(kernel);
	if(it != _ifthenelsetable.end()) {
		auto jt = it->second.find(truebranch);
		if(jt != it->second.end()) {
			auto kt = jt->second.find(falsebranch);
			if(kt != jt->second.end()) {
				return kt->second;
			}
		}
	}

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
	auto it = _quanttable.find(sort);
	if(it != _quanttable.end()) {
		auto jt = it->second.find(bdd);
		if(jt != it->second.end()) { return jt->second; }
	}
	if(bdd->kernel()->category() == STANDARDCATEGORY) {
		const FOBDD* newfalse = quantify(sort,bdd->falsebranch());
		const FOBDD* newtrue = quantify(sort,bdd->truebranch());
		const FOBDD* result = ifthenelse(bdd->kernel(),newtrue,newfalse);
		return result;
	}
	else {
		const FOBDDKernel* kernel = getQuantKernel(sort,bdd);
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
			if(it != _mvv.end()) { return it->second; }
			else { return v; }
		}
};

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd,const map<const FOBDDVariable*,const FOBDDVariable*>& mvv) {
	Substitute s(this,mvv);
	return s.FOBDDVisitor::change(bdd);
}

class VarSubstitute : public FOBDDVisitor {
	private:
		map<const FOBDDVariable*,const FOBDDArgument*> _mva;
	public:
		VarSubstitute(FOBDDManager* m, const map<const FOBDDVariable*,const FOBDDArgument*>& mva) :
			FOBDDVisitor(m), _mva(mva) { }

		const FOBDDArgument* change(const FOBDDVariable* v) {
			auto it = _mva.find(v);
			if(it != _mva.end()) { return it->second; }
			else { return v; }
		}
};

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd,const map<const FOBDDVariable*,const FOBDDArgument*>& mvv) {
	VarSubstitute s(this,mvv);
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
			if(dt == _domainterm) { return _variable; }
			else { return dt; }
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
			if(i == _index) { return _variable; }
			else { return i; }
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
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) { return 1; }
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* qk = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		return longestbranch(qk->bdd()) + 1;
	}
}

int FOBDDManager::longestbranch(const FOBDD* bdd) {
	if(bdd == _truebdd || bdd == _falsebdd) { return 1; }
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
				if(_result) { (*it)->accept(this); }
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
				if(_result) { (*it)->accept(this); }
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
 * Class to replace an atom F(x,y) by F(x) = y
 */
class FuncAtomRemover : public FOBDDVisitor {
	public:
		FuncAtomRemover(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDKernel* change(const FOBDDAtomKernel* atom) {
			if(typeid(*(atom->symbol())) == typeid(Function) && atom->type() == AKT_TWOVAL) {
				Function* f = dynamic_cast<Function*>(atom->symbol());
				Sort* s = SortUtils::resolve(f->outsort(),atom->args().back()->sort());
				vector<Sort*> equalsorts(2,s);
				Predicate* equalpred = Vocabulary::std()->pred("=/2");
				equalpred = equalpred->disambiguate(equalsorts);
				vector<const FOBDDArgument*> funcargs = atom->args(); funcargs.pop_back();
				const FOBDDArgument* functerm = _manager->getFuncTerm(f,funcargs);
				vector<const FOBDDArgument*> newargs; 
				newargs.push_back(functerm);
				newargs.push_back(atom->args().back());
				return _manager->getAtomKernel(equalpred,AKT_TWOVAL,newargs);
			}
			else { return atom; }
		}
};

/**
 * Class to move all terms in an equation to the left hand side
 */
class TermsToLeft : public FOBDDVisitor {
	public:
		TermsToLeft(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDKernel* change(const FOBDDAtomKernel* atom) {
			const FOBDDArgument* lhs = 0;
			const FOBDDArgument* rhs = 0;
			if(typeid(*(atom->symbol())) == typeid(Function)) {
				rhs = atom->args().back();
				vector<const FOBDDArgument*> lhsterms = atom->args(); lhsterms.pop_back();
				lhs = _manager->getFuncTerm(dynamic_cast<Function*>(atom->symbol()),lhsterms);
			}
			else {
				const string& predname = atom->symbol()->name();
				if(predname == "=/2" || predname == "</2" || predname == ">/2") {
					rhs = atom->args(1);
					lhs = atom->args(0);
				}
			}

			if(lhs && rhs) {
				if(SortUtils::isSubsort(rhs->sort(),VocabularyUtils::floatsort())) {
					const DomainElement* zero = DomainElementFactory::instance()->create(0);
					const FOBDDDomainTerm* zero_term = _manager->getDomainTerm(rhs->sort(),zero);
					if(rhs != zero_term) {
						Sort* minussort = SortUtils::resolve(lhs->sort(),rhs->sort());
						if(minussort) {
							Function* minus = Vocabulary::std()->func("-/2");
							vector<Sort*> minussorts(2,minussort);
							minus = minus->disambiguate(minussorts,0);
							assert(minus);
							vector<const FOBDDArgument*> newlhsargs;
							newlhsargs.push_back(lhs);
							newlhsargs.push_back(rhs);
							const FOBDDArgument* newlhs = _manager->getFuncTerm(minus,newlhsargs);
							vector<const FOBDDArgument*> newatomargs;
							newatomargs.push_back(newlhs);
							newatomargs.push_back(zero_term);
							return _manager->getAtomKernel(atom->symbol(),atom->type(),newatomargs);
						}
					}
				}
			}
			return atom;
		}
};

/**
 * Class to replace (t1 - t2) by (t1 + (-1) * t2) and (-t) by ((-1) * t)
 */
class RemoveMinus : public FOBDDVisitor {
	public:
		RemoveMinus(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
			if(functerm->func()->name() == "-/2") {
				Function* plus = Vocabulary::std()->func("+/2");
				plus = plus->disambiguate(functerm->func()->sorts(),0); assert(plus);
				vector<const FOBDDArgument*> newargs;
				newargs.push_back(functerm->args(0));
				const FOBDDArgument* rhs = functerm->args(1);
				const DomainElement* minusone = DomainElementFactory::instance()->create(-1);
				const FOBDDDomainTerm* minusone_term = _manager->getDomainTerm(rhs->sort(),minusone);
				vector<const FOBDDArgument*> rhsargs; 
				rhsargs.push_back(minusone_term);
				rhsargs.push_back(rhs);
				Function* times = Vocabulary::std()->func("*/2");
				vector<Sort*> timessorts(3,rhs->sort());
				times = times->disambiguate(timessorts,0);
				newargs.push_back(_manager->getFuncTerm(times,rhsargs));
				const FOBDDArgument* newterm = _manager->getFuncTerm(plus,newargs);
				return newterm->acceptchange(this);
			}
			else if(functerm->func()->name() == "-/1") {
				const DomainElement* minusone = DomainElementFactory::instance()->create(-1);
				const FOBDDDomainTerm* minusone_term = _manager->getDomainTerm(functerm->args(0)->sort(),minusone);
				vector<const FOBDDArgument*> newargs; 
				newargs.push_back(minusone_term);
				newargs.push_back(functerm->args(0));
				Function* times = Vocabulary::std()->func("*/2");
				vector<Sort*> timessorts(3,functerm->args(0)->sort());
				times = times->disambiguate(timessorts,0);
				const FOBDDArgument* newterm = _manager->getFuncTerm(times,newargs);
				return newterm->acceptchange(this);
			}
			else { return FOBDDVisitor::change(functerm); }
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
						const FOBDDArgument* newleft = _manager->getFuncTerm(functerm->func(),newleftargs);
						const FOBDDArgument* newright = _manager->getFuncTerm(functerm->func(),newrightargs);
						vector<const FOBDDArgument*> newargs;
						newargs.push_back(newleft);
						newargs.push_back(newright);
						const FOBDDArgument* newterm = _manager->getFuncTerm(leftfuncterm->func(),newargs);
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
						const FOBDDArgument* newleft = _manager->getFuncTerm(functerm->func(),newleftargs);
						const FOBDDArgument* newright = _manager->getFuncTerm(functerm->func(),newrightargs);
						vector<const FOBDDArgument*> newargs;
						newargs.push_back(newleft);
						newargs.push_back(newright);
						const FOBDDArgument* newterm = _manager->getFuncTerm(rightfuncterm->func(),newargs);
						return newterm->acceptchange(this);
					}
				}
			}
			return FOBDDVisitor::change(functerm);
		}
};

/**
 * Classes to order multiplications
 */

class MultTermExtractor : public FOBDDVisitor {
	private:
		vector<const FOBDDArgument*> _terms;
	public:
		MultTermExtractor(FOBDDManager* m) : FOBDDVisitor(m) { }

		const vector<const FOBDDArgument*>& run(const FOBDDArgument* arg) {
			_terms.clear();
			arg->accept(this);
			return _terms;
		}

		void visit(const FOBDDDomainTerm* domterm) {
			_terms.push_back(domterm);
		}

		void visit(const FOBDDDeBruijnIndex* dbrterm) {
			_terms.push_back(dbrterm);
		}

		void visit(const FOBDDVariable* varterm) {
			_terms.push_back(varterm);
		}

		void visit(const FOBDDFuncTerm* functerm) {
			if(functerm->func()->name() == "*/2") {
				functerm->args(0)->accept(this);
				functerm->args(1)->accept(this);
			}
			else _terms.push_back(functerm);
		}
};

class MultOrderer : public FOBDDVisitor {
	public:
		MultOrderer(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
			if(functerm->func()->name() == "*/2") {
				MultTermExtractor mte(_manager);
				vector<const FOBDDArgument*> multterms = mte.run(functerm);
				for(size_t n = 0; n < multterms.size(); ++n) {
					// TODO? recursive call on all elements of multterms
				}
				std::sort(multterms.begin(),multterms.end(),&FactorSWOrdering); 
				const FOBDDArgument* currarg = multterms.back();
				for(size_t n = multterms.size()-1; n != 0; --n) {
					const FOBDDArgument* nextarg = multterms[n-1];
					Sort* multsort = SortUtils::resolve(currarg->sort(),nextarg->sort());
					vector<Sort*> multsorts(3,multsort);
					Function* mult = Vocabulary::std()->func("*/2");
					mult = mult->disambiguate(multsorts,0); assert(mult);
					vector<const FOBDDArgument*> multargs(2);
					multargs[0] = nextarg;
					multargs[1] = currarg;
					currarg = _manager->getFuncTerm(mult,multargs);
				}
				return currarg;
			}
			else { return FOBDDVisitor::change(functerm); }
		}
};

/**
 * Class to simplify multiplications
 */
class AddMultSimplifier : public FOBDDVisitor {
	public:
		AddMultSimplifier(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
			const FOBDDArgument* recurterm = FOBDDVisitor::change(functerm);
			if(typeid(*recurterm) == typeid(FOBDDFuncTerm)) {
				functerm = dynamic_cast<const FOBDDFuncTerm*>(recurterm);
				if(functerm->func()->name() == "*/2" || functerm->func()->name() == "+/2") {
					if(typeid(*(functerm->args(0))) == typeid(FOBDDDomainTerm)) {
						const FOBDDDomainTerm* leftconstant = dynamic_cast<const FOBDDDomainTerm*>(functerm->args(0));
						if(typeid(*(functerm->args(1))) == typeid(FOBDDFuncTerm)) {
							const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(functerm->args(1));
							if(rightterm->func()->name() == functerm->func()->name()) {
								if(typeid(*(rightterm->args(0))) == typeid(FOBDDDomainTerm)) {
									const FOBDDDomainTerm* rightconstant = 
										dynamic_cast<const FOBDDDomainTerm*>(rightterm->args(0));
									FuncInter* fi = functerm->func()->interpretation(0);
									vector<const DomainElement*> multargs(2);
									multargs[0] = leftconstant->value();
									multargs[1] = rightconstant->value();
									const DomainElement* result = fi->funcTable()->operator[](multargs);
									const FOBDDDomainTerm* multres = 
										_manager->getDomainTerm(functerm->func()->outsort(),result);
									vector<const FOBDDArgument*> newargs(2);
									newargs[0] = multres;
									newargs[1] = rightterm->args(1);
									const FOBDDArgument* newterm = _manager->getFuncTerm(rightterm->func(),newargs);
									return newterm->acceptchange(this);
								}
							}
						}
						else if(typeid(*(functerm->args(1))) == typeid(FOBDDDomainTerm)) {
							const FOBDDDomainTerm* rightconstant = dynamic_cast<const FOBDDDomainTerm*>(functerm->args(1));
							FuncInter* fi = functerm->func()->interpretation(0);
							vector<const DomainElement*> multargs(2);
							multargs[0] = leftconstant->value();
							multargs[1] = rightconstant->value();
							const DomainElement* result = fi->funcTable()->operator[](multargs);
							return _manager->getDomainTerm(functerm->func()->outsort(),result);
						}
					}
				}
			}
			return recurterm;
		}
};


/**
 * Classes to order additions
 */

class NonConstTermExtractor : public FOBDDVisitor {
	private:
		const FOBDDArgument* _result;
	public:
		NonConstTermExtractor() : FOBDDVisitor(0) { }

		const FOBDDArgument* run(const FOBDDArgument* arg) {
			_result = 0;
			arg->accept(this);
			return _result;
		}

		void visit(const FOBDDDomainTerm* dt)		{ _result = dt; }
		void visit(const FOBDDVariable* vt)			{ _result = vt; }
		void visit(const FOBDDDeBruijnIndex* dt)	{ _result = dt;	}
		void visit(const FOBDDFuncTerm* ft) {
			if(ft->func()->name() == "*/2") {
				if(typeid(*(ft->args(0))) == typeid(FOBDDDomainTerm)) {
					ft->args(1)->accept(this);
					return;
				}
			}
			_result = ft;
		}

};

struct AddTermSWOrdering {
	bool operator()(const FOBDDArgument* arg1, const FOBDDArgument* arg2) {
		NonConstTermExtractor ncte;
		const FOBDDArgument* arg1nc = ncte.run(arg1);
		const FOBDDArgument* arg2nc = ncte.run(arg2);
		if(arg1nc == arg2nc) { return arg1 < arg2; }
		else if(typeid(*arg1nc) == typeid(FOBDDDomainTerm)) {
			if(typeid(*arg2nc) == typeid(FOBDDDomainTerm)) { return arg1 < arg2; }
			else { return true; }
		}
		else if(typeid(*arg2nc) == typeid(FOBDDDomainTerm)) { return false; }
		else { return arg1nc < arg2nc; }
	}
};

class AddTermExtractor : public FOBDDVisitor {
	private:
		vector<const FOBDDArgument*> _terms;
	public:
		AddTermExtractor(FOBDDManager* m) : FOBDDVisitor(m) { }

		const vector<const FOBDDArgument*>& run(const FOBDDArgument* arg) {
			_terms.clear();
			arg->accept(this);
			return _terms;
		}

		void visit(const FOBDDDomainTerm* domterm) {
			_terms.push_back(domterm);
		}

		void visit(const FOBDDDeBruijnIndex* dbrterm) {
			_terms.push_back(dbrterm);
		}

		void visit(const FOBDDVariable* varterm) {
			_terms.push_back(varterm);
		}

		void visit(const FOBDDFuncTerm* functerm) {
			if(functerm->func()->name() == "+/2") {
				functerm->args(0)->accept(this);
				functerm->args(1)->accept(this);
			}
			else { _terms.push_back(functerm); }
		}
};

class AddOrderer : public FOBDDVisitor {
	public:
		AddOrderer(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
			if(functerm->func()->name() == "+/2") {
				AddTermExtractor mte(_manager);
				vector<const FOBDDArgument*> addterms = mte.run(functerm);
				for(size_t n = 0; n < addterms.size(); ++n) {
					// TODO? recursive call on all elements of addterms
				}
				AddTermSWOrdering mtswo;
				std::sort(addterms.begin(),addterms.end(),mtswo); 
				const FOBDDArgument* currarg = addterms.back();
				for(size_t n = addterms.size()-1; n != 0; --n) {
					const FOBDDArgument* nextarg = addterms[n-1];
					Sort* addsort = SortUtils::resolve(currarg->sort(),nextarg->sort());
					vector<Sort*> addsorts(3,addsort);
					Function* add = Vocabulary::std()->func("+/2");
					add = add->disambiguate(addsorts,0); assert(add);
					vector<const FOBDDArgument*> addargs(2);
					addargs[0] = nextarg;
					addargs[1] = currarg;
					currarg = _manager->getFuncTerm(add,addargs);
				}
				return currarg;
			}
			else { return FOBDDVisitor::change(functerm); }
		}
};

/**
 *Classes to add terms with the same non-constant factor
 */
class ConstTermExtractor : public FOBDDVisitor {
	private:
		const FOBDDDomainTerm* _result;
	public:
		ConstTermExtractor(FOBDDManager* m) : FOBDDVisitor(m) { }
		const FOBDDDomainTerm* run(const FOBDDArgument* arg) {
			const DomainElement* d = DomainElementFactory::instance()->create(1);
			_result = _manager->getDomainTerm(arg->sort(),d);
			arg->accept(this);
			return _result;
		}
		void visit(const FOBDDFuncTerm* ft) { 
			if(typeid(*(ft->args(0))) == typeid(FOBDDDomainTerm)) {
				assert(typeid(*(ft->args(1))) != typeid(FOBDDDomainTerm));
				_result = dynamic_cast<const FOBDDDomainTerm*>(ft->args(0));
			}
		}
};

class TermAdder : public FOBDDVisitor {
	public:
		TermAdder(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDDomainTerm* add(const FOBDDDomainTerm* d1, const FOBDDDomainTerm* d2) {
			Sort* addsort = SortUtils::resolve(d1->sort(),d2->sort());
			vector<Sort*> addsorts(3,addsort);
			Function* addfunc = Vocabulary::std()->func("+/2");
			addfunc = addfunc->disambiguate(addsorts,0); assert(addfunc);
			FuncInter* fi = addfunc->interpretation(0);
			vector<const DomainElement*> addargs(2);
			addargs[0] = d1->value(); addargs[1] = d2->value();
			const DomainElement* result = fi->funcTable()->operator[](addargs);
			return _manager->getDomainTerm(addsort,result);
		}

		const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
			if(functerm->func()->name() == "+/2") {

				NonConstTermExtractor ncte;
				const FOBDDArgument* leftncte = ncte.run(functerm->args(0));

				if(typeid(*(functerm->args(1))) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(functerm->args(1));
					if(rightterm->func()->name() == "+/2") {
						const FOBDDArgument* rightncte = ncte.run(rightterm->args(0));
						if(leftncte == rightncte) {
							ConstTermExtractor cte(_manager);
							const FOBDDDomainTerm* leftconst = cte.run(functerm->args(0));
							const FOBDDDomainTerm* rightconst = cte.run(rightterm->args(0));
							const FOBDDDomainTerm* addterm = add(leftconst,rightconst);
							Function* mult = Vocabulary::std()->func("*/2");
							Sort* multsort = SortUtils::resolve(addterm->sort(),leftncte->sort());
							vector<Sort*> multsorts(3,multsort);
							mult = mult->disambiguate(multsorts,0); assert(mult);
							vector<const FOBDDArgument*> multargs(2);
							multargs[0] = addterm; multargs[1] = leftncte;
							const FOBDDArgument* newterm = _manager->getFuncTerm(mult,multargs);
							Function* plus = Vocabulary::std()->func("+/2");
							Sort* plussort = SortUtils::resolve(newterm->sort(),rightterm->args(1)->sort());
							vector<Sort*> plussorts(3,plussort);
							plus = plus->disambiguate(plussorts,0); assert(plus);
							vector<const FOBDDArgument*> plusargs(2);
							plusargs[0] = newterm; plusargs[1] = rightterm->args(1);
							const FOBDDArgument* addbddterm = _manager->getFuncTerm(plus,plusargs);
							return addbddterm->acceptchange(this);
						}
						else { return FOBDDVisitor::change(functerm); }
					}
				}

				const FOBDDArgument* rightncte = ncte.run(functerm->args(1));
				if(leftncte == rightncte) {
					ConstTermExtractor cte(_manager);
					const FOBDDDomainTerm* leftconst = cte.run(functerm->args(0));
					const FOBDDDomainTerm* rightconst = cte.run(functerm->args(1));
					const FOBDDDomainTerm* addterm = add(leftconst,rightconst);
					Function* mult = Vocabulary::std()->func("*/2");
					Sort* multsort = SortUtils::resolve(addterm->sort(),leftncte->sort());
					vector<Sort*> multsorts(3,multsort);
					mult = mult->disambiguate(multsorts,0); assert(mult);
					vector<const FOBDDArgument*> multargs(2);
					multargs[0] = addterm; multargs[1] = leftncte;
					return _manager->getFuncTerm(mult,multargs);
				}
			}
			return FOBDDVisitor::change(functerm);
		}
};

class Neutralizer : public FOBDDVisitor {
	public:
		Neutralizer(FOBDDManager* m) : FOBDDVisitor(m) { }
		const FOBDDArgument* change(const FOBDDFuncTerm* ft) {
			const FOBDDArgument* rec = FOBDDVisitor::change(ft);
			if(typeid(*rec) == typeid(FOBDDFuncTerm)) {
				ft = dynamic_cast<const FOBDDFuncTerm*>(rec);
				if(ft->func()->name() == "+/2") {
					if(typeid(*(ft->args(0))) == typeid(FOBDDDomainTerm)) {
						const FOBDDDomainTerm* dt = dynamic_cast<const FOBDDDomainTerm*>(ft->args(0));
						const DomainElement* zero = DomainElementFactory::instance()->create(0);
						if(zero == dt->value()) { return ft->args(1)->acceptchange(this); }
					}
				}
				else if(ft->func()->name() == "*/2") {
					if(typeid(*(ft->args(0))) == typeid(FOBDDDomainTerm)) {
						const FOBDDDomainTerm* dt = dynamic_cast<const FOBDDDomainTerm*>(ft->args(0));
						const DomainElement* zero = DomainElementFactory::instance()->create(0);
						const DomainElement* one = DomainElementFactory::instance()->create(1);
						if(one == dt->value()) { return ft->args(1)->acceptchange(this); }
						else if(zero == dt->value()) { return _manager->getDomainTerm(ft->sort(),zero); }
					}
				}
			}
			return rec;
		}
};

/********************************
	Simplify arithmetic terms
********************************/

const FOBDD* FOBDDManager::simplify(const FOBDD* bdd) {
	FuncAtomRemover far(this);
	bdd = far.FOBDDVisitor::change(bdd);
	TermsToLeft ttl(this);
	bdd = ttl.FOBDDVisitor::change(bdd);
	RemoveMinus rm(this);
	bdd = rm.FOBDDVisitor::change(bdd);
	Distributivity dsbtvt(this);
	bdd = dsbtvt.FOBDDVisitor::change(bdd);
	MultOrderer mo(this);
	bdd = mo.FOBDDVisitor::change(bdd);
	AddMultSimplifier ms(this);
	bdd = ms.FOBDDVisitor::change(bdd);
	AddOrderer ao(this);
	bdd = ao.FOBDDVisitor::change(bdd);
	bdd = ms.FOBDDVisitor::change(bdd);
	TermAdder ta(this);
	bdd = ta.FOBDDVisitor::change(bdd);
	Neutralizer neut(this);
	bdd = neut.FOBDDVisitor::change(bdd);
	return bdd;
}

/**********************
	Solve equations
**********************/

class ArgChecker : public FOBDDVisitor {
	private:
		bool _result;
		const FOBDDArgument* _arg;

		void visit(const FOBDDVariable* var)		{ if(var == _arg) { _result = true;	} }
		void visit(const FOBDDDeBruijnIndex* index)	{ if(index == _arg) { _result = true;	} }
		void visit(const FOBDDDomainTerm* dt)		{ if(dt == _arg) { _result = true;	} }
		void visit(const FOBDDFuncTerm* ft)	{
			if(ft == _arg) {
				_result = true;
				return;
			}
			else {
				for(auto it = ft->args().begin(); it != ft->args().end(); ++it) {
					(*it)->accept(this);
					if(_result) { return; }
				}
			}
		}
	public:
		ArgChecker(FOBDDManager* m) : FOBDDVisitor(m) { }
		bool run(const FOBDDArgument* super, const FOBDDArgument* arg) {
			_result = false;
			_arg = arg;
			super->accept(this);
			return _result;
		}
};

bool FOBDDManager::contains(const FOBDDArgument* super, const FOBDDArgument* arg) {
	ArgChecker ac(this);
	return ac.run(super,arg);
}

const FOBDDArgument* FOBDDManager::solve(const FOBDDKernel* kernel, const FOBDDArgument* argument) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atom = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		if(atom->symbol()->name() == "=/2") {
			if(atom->args(0) == argument) {
				if(not contains(atom->args(1),argument)) { return atom->args(1); }
			}
			if(atom->args(1) == argument) {
				if(not contains(atom->args(0),argument)) { return atom->args(0); }
			}
			if(SortUtils::isSubsort(atom->symbol()->sorts()[0],VocabularyUtils::floatsort())) {
				FlatAdd fa(this);
				vector<const FOBDDArgument*> terms = fa.run(atom->args(0));
				unsigned int occcounter = 0;
				unsigned int occterm;
				for(size_t n = 0; n < terms.size(); ++n) {
					if(contains(terms[n],argument)) {
						++occcounter;
						occterm = n;
					}
				}
				if(occcounter == 1) {
					FlatMult fm(this);
					vector<const FOBDDArgument*> factors = fm.run(terms[occterm]);
					if(factors.size() == 2 && factors[1] == argument) {
						const FOBDDArgument* currterm = 0;
						for(size_t n = 0; n < terms.size(); ++n) {
							if(n != occterm) {
								if(not currterm) { currterm = terms[n]; }
								else {
									Function* plus = Vocabulary::std()->func("+/2");
									plus = plus->disambiguate(vector<Sort*>(3,SortUtils::resolve(currterm->sort(),terms[n]->sort())),0);
									vector<const FOBDDArgument*> newargs(2); newargs[0] = currterm; newargs[1] = terms[n];
									currterm = getFuncTerm(plus,newargs);
								}
							}
						}
						if(not currterm) { return atom->args(1); }
						else {
							const FOBDDDomainTerm* constant = dynamic_cast<const FOBDDDomainTerm*>(factors[0]);
							if(constant->value()->type() == DET_INT) {
								int constval = constant->value()->value()._int;
								if(constval == -1) { return currterm; }
								else if(constval == 1) { return invert(currterm); }
							}
							if(SortUtils::isSubsort(currterm->sort(),VocabularyUtils::intsort())) {
								// TODO: try if constval divides all constant factors
							}
							else {
								Function* times = Vocabulary::std()->func("*/2");
								times = times->disambiguate(vector<Sort*>(3,VocabularyUtils::floatsort()),0);
								vector<const FOBDDArgument*> timesargs(2);
								timesargs[0] = currterm;
								double d = constant->value()->type() == DET_INT ? 
									(double(1) / double(constant->value()->value()._int)) : 
									(double(1) / constant->value()->value()._double);
								timesargs[1] = getDomainTerm(VocabularyUtils::floatsort(),DomainElementFactory::instance()->create(d));
								d = -d;
								return getFuncTerm(times,timesargs);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

/********************
	FOBDD Factory
********************/

const FOBDD* FOBDDFactory::run(const Formula* f) {
	Formula* cf = f->clone();
	cf = FormulaUtils::movePartialTerms(cf);
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
	AtomKernelType akt = AKT_TWOVAL;
	bool notinverse = true;
	PFSymbol* symbol = pf->symbol();
	if(typeid(*symbol) == typeid(Predicate)) {
		Predicate* predicate = dynamic_cast<Predicate*>(symbol);
		if(predicate->type() != ST_NONE) {
			switch(predicate->type()) {
				case ST_CF: akt = AKT_CF; break;
				case ST_CT: akt = AKT_CT; break;
				case ST_PF: akt = AKT_CT; notinverse = false; break;
				case ST_PT: akt = AKT_CF; notinverse = false; break;
				default: assert(false);
			}
			symbol = predicate->parent();
		}
	}
	_kernel = _manager->getAtomKernel(pf->symbol(),akt,args);
	if(pf->sign() == notinverse) { _bdd = _manager->getBDD(_kernel,_manager->truebdd(),_manager->falsebdd()); }
	else { _bdd = _manager->getBDD(_kernel,_manager->falsebdd(),_manager->truebdd()); }
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
	_bdd = (bf->sign() ? _bdd : _manager->negation(_bdd));
}

void FOBDDFactory::visit(const QuantForm* qf) {
	qf->subformula()->accept(this);
	const FOBDD* qbdd = _bdd;
	for(set<Variable*>::const_iterator it = qf->quantVars().begin(); it != qf->quantVars().end(); ++it) {
		const FOBDDVariable* qvar = _manager->getVariable(*it);
		if(qf->univ()) { qbdd = _manager->univquantify(qvar,qbdd); }
		else { qbdd = _manager->existsquantify(qvar,qbdd); }
	}
	_bdd = (qf->sign() ? qbdd : _manager->negation(qbdd));
}

void FOBDDFactory::visit(const EqChainForm* ef) {
	EqChainForm* efclone = ef->clone();
	Formula* f = FormulaUtils::removeEqChains(efclone,_vocabulary);
	f->accept(this);
	f->recursiveDelete();
}

void FOBDDFactory::visit(const AggForm*) {
	// TODO
	assert(false);
}


/****************
	Debugging
****************/

ostream& FOBDDManager::put(ostream& output, const FOBDD* bdd, unsigned int spaces) const {
	if(bdd == _truebdd) {
		printTabs(output,spaces);
		output << "true\n";
	}
	else if(bdd == _falsebdd) {
		printTabs(output,spaces);
		output << "false\n";
	}
	else {
		put(output,bdd->kernel(),spaces);
		printTabs(output,spaces+3);
		output << "FALSE BRANCH:\n";
		put(output,bdd->falsebranch(),spaces+6);
		printTabs(output,spaces+3);
		output << "TRUE BRANCH:\n";
		put(output,bdd->truebranch(),spaces+6);
	}
	return output;
}

ostream& FOBDDManager::put(ostream& output, const FOBDDKernel* kernel, unsigned int spaces) const {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		printTabs(output,spaces);
		output << *symbol;
		if(atomkernel->type() == AKT_CF) { output << "<cf>"; }
		else if(atomkernel->type() == AKT_CT) { output << "<ct>"; }
		if(typeid(*symbol) == typeid(Predicate)) {
			if(symbol->nrSorts()) {
				output << "(";
				put(output,atomkernel->args(0));
				for(size_t n = 1; n < symbol->nrSorts(); ++n) {
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
				for(size_t n = 1; n < symbol->nrSorts()-1; ++n) {
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
		printTabs(output,spaces);
		output << "EXISTS(" << *(quantkernel->sort()) << ") {\n";
		put(output,quantkernel->bdd(),spaces+3);
		printTabs(output,spaces);
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
			for(size_t n = 1; n < f->arity(); ++n) {
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

/*************************
	Convert to formula
*************************/

class BDDToFormula : public FOBDDVisitor {
	private:
		Formula* _currformula;
		Term* _currterm;
		map<const FOBDDDeBruijnIndex*,Variable*> _dbrmapping;

		void visit(const FOBDDDeBruijnIndex* index) {
			auto it = _dbrmapping.find(index);
			Variable* v;
			if(it == _dbrmapping.end()) {
				Variable* v = new Variable(index->sort());
				_dbrmapping[index] = v;
			}
			else { v = it->second; }
			_currterm = new VarTerm(v,TermParseInfo());
		}

		void visit(const FOBDDVariable* var) {
			_currterm = new VarTerm(var->variable(),TermParseInfo());
		}

		void visit(const FOBDDDomainTerm* dt) {
			_currterm = new DomainTerm(dt->sort(),dt->value(),TermParseInfo());
		}

		void visit(const FOBDDFuncTerm* ft) {
			vector<Term*> args;
			for(auto it = ft->args().begin(); it != ft->args().end(); ++it) {
				(*it)->accept(this);
				args.push_back(_currterm);
			}
			_currterm = new FuncTerm(ft->func(),args,TermParseInfo());
		}

		void visit(const FOBDDAtomKernel* atom) {
			vector<Term*> args;
			for(auto it = atom->args().begin(); it != atom->args().end(); ++it) {
				(*it)->accept(this);
				args.push_back(_currterm);
			}
			switch(atom->type()) {
				case AKT_TWOVAL:
					_currformula = new PredForm(true,atom->symbol(),args,FormulaParseInfo());
					break;
				case AKT_CT:
					_currformula = new PredForm(true,atom->symbol()->derivedSymbol(ST_CT),args,FormulaParseInfo());
					break;
				case AKT_CF:
					_currformula = new PredForm(true,atom->symbol()->derivedSymbol(ST_CF),args,FormulaParseInfo());
					break;
				default: 
					assert(false);
			}
		}

		void visit(const FOBDDQuantKernel* quantkernel) {
			map<const FOBDDDeBruijnIndex*,Variable*> savemapping = _dbrmapping;
			_dbrmapping.clear();
			for(auto it = savemapping.begin(); it != savemapping.end(); ++it) 
				_dbrmapping[_manager->getDeBruijnIndex(it->first->sort(),it->first->index()+1)] = it->second;
			FOBDDVisitor::visit(quantkernel->bdd());
			set<Variable*> quantvars;
			quantvars.insert(_dbrmapping[_manager->getDeBruijnIndex(quantkernel->sort(),0)]);
			_dbrmapping = savemapping;
			_currformula = new QuantForm(true,false,quantvars,_currformula,FormulaParseInfo());
		}

		void visit(const FOBDD* bdd) {
			if(_manager->isTruebdd(bdd)) { _currformula =  FormulaUtils::trueFormula(); }
			else if(_manager->isFalsebdd(bdd)) { _currformula = FormulaUtils::falseFormula(); }
			else {
				bdd->kernel()->accept(this);
				if(_manager->isFalsebdd(bdd->falsebranch())) {
					if(not _manager->isTruebdd(bdd->truebranch())) {
						Formula* kernelform = _currformula;
						FOBDDVisitor::visit(bdd->truebranch());
						_currformula = new BoolForm(true,true,kernelform,_currformula,FormulaParseInfo());
					}
				}
				else if(_manager->isFalsebdd(bdd->truebranch())) {
					_currformula->negate();
					if(not _manager->isTruebdd(bdd->falsebranch())) {
						Formula* kernelform = _currformula;
						FOBDDVisitor::visit(bdd->falsebranch());
						_currformula = new BoolForm(true,true,kernelform,_currformula,FormulaParseInfo());
					}
				}
				else {
					Formula* kernelform = _currformula;
					Formula* negkernelform = kernelform->clone(); negkernelform->negate();
					if(_manager->isTruebdd(bdd->falsebranch())) {
						FOBDDVisitor::visit(bdd->truebranch());
						BoolForm* bf = new BoolForm(true,true,kernelform,_currformula,FormulaParseInfo());
						_currformula = new BoolForm(true,false,negkernelform,bf,FormulaParseInfo());
					}
					else if(_manager->isTruebdd(bdd->truebranch())) {
						FOBDDVisitor::visit(bdd->falsebranch());
						BoolForm* bf = new BoolForm(true,true,negkernelform,_currformula,FormulaParseInfo());
						_currformula = new BoolForm(true,false,kernelform,bf,FormulaParseInfo());
					}
					else {
						FOBDDVisitor::visit(bdd->truebranch());
						Formula* trueform = _currformula;
						FOBDDVisitor::visit(bdd->falsebranch());
						Formula* falseform = _currformula;
						BoolForm* bf1 = new BoolForm(true,true,kernelform,trueform,FormulaParseInfo());
						BoolForm* bf2 = new BoolForm(true,true,negkernelform,falseform,FormulaParseInfo());
						_currformula = new BoolForm(true,false,bf1,bf2,FormulaParseInfo());
					}
				}
			}
		}

	public:
		BDDToFormula(FOBDDManager* m) : FOBDDVisitor(m) { }
		Formula* run(const FOBDDKernel* kernel) { 
			kernel->accept(this);
			return _currformula;
		}
		Formula* run(const FOBDD* bdd) { 
			FOBDDVisitor::visit(bdd);
			return _currformula;
		}
		Term* run(const FOBDDArgument* arg) {
			arg->accept(this);
			return _currterm;
		}
};

Formula* FOBDDManager::toFormula(const FOBDD* bdd) {
	BDDToFormula btf(this);
	return btf.run(bdd);
}

Formula* FOBDDManager::toFormula(const FOBDDKernel* kernel) {
	BDDToFormula btf(this);
	return btf.run(kernel);
}

Term* FOBDDManager::toTerm(const FOBDDArgument* arg) {
	BDDToFormula btf(this);
	return btf.run(arg);
}


/*******************************
	Check for function terms
*******************************/

class FuncTermChecker : public FOBDDVisitor {
	private:
		bool _result;
		void visit(const FOBDDFuncTerm*) {
			_result = true;
			return;
		}
	public:
		FuncTermChecker(FOBDDManager* m) : FOBDDVisitor(m) { }
		bool run(const FOBDDKernel* kernel) { 
			_result = false;
			kernel->accept(this);
			return _result;
		}
		bool run(const FOBDD* bdd) { 
			_result = false;
			FOBDDVisitor::visit(bdd);
			return _result;
		}
};

bool FOBDDManager::containsFuncTerms(const FOBDDKernel* kernel) {
	FuncTermChecker ft(this);
	return ft.run(kernel);
}

bool FOBDDManager::containsFuncTerms(const FOBDD* bdd) {
	FuncTermChecker ft(this);
	return ft.run(bdd);
}

class BDDPartialChecker : public FOBDDVisitor {
	private:
		bool _result;
		void visit(const FOBDDFuncTerm* ft) {
			if(ft->func()->partial()) {
				_result = true;
				return;
			}
			else {
				for(auto it = ft->args().begin(); it != ft->args().end(); ++it) {
					(*it)->accept(this);
					if(_result) return;
				}
			}
		}

	public:
		BDDPartialChecker(FOBDDManager* m) : FOBDDVisitor(m) { }
		bool run(const FOBDDArgument* arg) {
			_result = false;
			arg->accept(this);
			return _result;
		}
};

bool FOBDDManager::partial(const FOBDDArgument* arg) {
	BDDPartialChecker bpc(this);
	return bpc.run(arg);
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
tablesize univNrAnswers(const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	vector<SortTable*> vst; 
	for(set<const FOBDDVariable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) 
		vst.push_back(structure->inter((*it)->variable()->sort()));
	for(set<const FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it)
		vst.push_back(structure->inter((*it)->sort()));
	Universe univ(vst);
	return univ.size();
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

map<const FOBDDKernel*,tablesize> FOBDDManager::kernelUnivs(const FOBDD* bdd, AbstractStructure* structure) {
	map<const FOBDDKernel*,tablesize> result;
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
		if(typeid(*symbol) == typeid(Predicate)) { pinter = structure->inter(dynamic_cast<Predicate*>(symbol)); }
		else { pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter(); }
		const PredTable* pt = atomkernel->type() == AKT_CF ? pinter->cf() : pinter->ct();
		tablesize symbolsize = pt->size();
		double univsize = 1;
		for(vector<const FOBDDArgument*>::const_iterator it = atomkernel->args().begin(); it != atomkernel->args().end(); ++it) {
			tablesize argsize = structure->inter((*it)->sort())->size();
			if(argsize._type == TST_APPROXIMATED || argsize._type == TST_EXACT) {
				univsize = univsize * argsize._size;
			}
			else {
				univsize = numeric_limits<double>::max();
				break;
			}
		}
		if(symbolsize._type == TST_APPROXIMATED || symbolsize._type == TST_EXACT) {
			if(univsize < numeric_limits<double>::max()) {
				chance = double(symbolsize._size) / univsize;
				if(chance > 1) { chance = 1; }
			}
			else { chance = 0; }
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
		if(quanttablesize._type == TST_APPROXIMATED || quanttablesize._type == TST_EXACT) {
			if(quanttablesize._size == 0) return 0;	// if the sort is empty, the kernel cannot be true
			else quantsize = quanttablesize._size;
		}
		else {
			if(!quantsorttable->approxFinite()) {
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
		map<const FOBDDKernel*,tablesize> subunivs = kernelUnivs(quantkernel->bdd(),structure);

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
						tablesize ts = subunivs[paths[pathnr][nodenr].second];
						double nodeunivsize = (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) ? ts._size : numeric_limits<double>::max();
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
				if(cumulative_chance > 1) {	// FIXME: looks like a bug :-)
					Warning::cumulchance(cumulative_chance);
					cumulative_chance = 1;
				}
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
	double maxdouble = numeric_limits<double>::max();
	double kernelchance = estimatedChance(kernel,structure);
	tablesize univanswers = univNrAnswers(vars,indices,structure);
	if(univanswers._type == TST_INFINITE || univanswers._type == TST_UNKNOWN) {
		return (kernelchance > 0 ? maxdouble : 0);
	}
	else return kernelchance * univanswers._size;
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | bdd } in the given structure 
 */
double FOBDDManager::estimatedNrAnswers(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	double maxdouble = numeric_limits<double>::max();
	double bddchance = estimatedChance(bdd,structure);
	tablesize univanswers = univNrAnswers(vars,indices,structure);
	if(univanswers._type == TST_INFINITE || univanswers._type == TST_UNKNOWN) {
		return (bddchance > 0 ? maxdouble : 0);
	}
	else return bddchance * univanswers._size;
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
			t->internTable()->accept(this);
			return _result;
		}

		void visit(const ProcInternalPredTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double sz = 1;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(not _pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
						if(sz * ts._size < maxdouble) {
							sz = sz * ts._size;
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
					if(ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
						if(sz * ts._size < maxdouble) {
							sz = sz * ts._size;
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
			t->table()->internTable()->accept(this);
		}

		void visit(const UnionInternalPredTable* ) {
			// TODO
		}

		void visit(const UnionInternalSortTable* ) {
			// TODO
		}

		void visit(const EnumeratedInternalSortTable* ) {
			if(_pattern[0]) {
				_result = log(double(_table->size()._size)) / log(2);
			}
			else {
				_result = _table->size()._size;
			}
		}

		void visit(const IntRangeInternalSortTable* ) {
			if(_pattern[0]) _result = 1;
			else _result = _table->size()._size;
		}

		void visit(const EnumeratedInternalPredTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double inputunivsize = 1;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
						if(inputunivsize * ts._size < maxdouble) {
							inputunivsize = inputunivsize * ts._size;
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
			double lookupsize = inputunivsize < ts._size ? inputunivsize : ts._size;
			if(log(lookupsize) / log(2) < maxdouble) {
				double lookuptime = log(lookupsize) / log(2);
				double iteratetime = ts._size / inputunivsize;
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
				if(ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) _result = ts._size;
				else _result = numeric_limits<double>::max();
			}
		}

		void visit(const StrLessInternalPredTable* ) {
			if(_pattern[0]) {
				if(_pattern[1]) _result = 1;
				else {
					tablesize ts = _table->universe().tables()[0]->size();
					if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size / double(2);
					else _result = numeric_limits<double>::max();
				}
			}
			else if(_pattern[1]) {
				tablesize ts = _table->universe().tables()[0]->size();
				if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size / double(2);
				else _result = numeric_limits<double>::max();
			}
			else {
				tablesize ts = _table->size();
				if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size;
				else _result = numeric_limits<double>::max();
			}
		}

		void visit(const StrGreaterInternalPredTable* ) {
			if(_pattern[0]) {
				if(_pattern[1]) _result = 1;
				else {
					tablesize ts = _table->universe().tables()[0]->size();
					if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size / double(2);
					else _result = numeric_limits<double>::max();
				}
			}
			else if(_pattern[1]) {
				tablesize ts = _table->universe().tables()[0]->size();
				if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size / double(2);
				else _result = numeric_limits<double>::max();
			}
			else {
				tablesize ts = _table->size();
				if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size;
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
						if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
							if(sz * ts._size < maxdouble) {
								sz = sz * ts._size;
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
					if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
						if(sz * ts._size < maxdouble) {
							sz = sz * ts._size;
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
						if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
							if(sz * ts._size < maxdouble) {
								sz = sz * ts._size;
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
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
						if(inputunivsize * ts._size < maxdouble) {
							inputunivsize = inputunivsize * ts._size;
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
			double lookupsize = inputunivsize < ts._size ? inputunivsize : ts._size;
			if(log(lookupsize) / log(2) < maxdouble) {
				double lookuptime = log(lookupsize) / log(2);
				double iteratetime = ts._size / inputunivsize;
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
			if(stsize._type == TST_EXACT || stsize._type == TST_APPROXIMATED) varunivsizes.push_back(double(stsize._size));
			else { varunivsizes.push_back(maxdouble); ++nrinfinite; if(!infinitevar) infinitevar = *it; }
		}
		for(set<const FOBDDDeBruijnIndex*>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
			indicesvector.push_back(*it);
			SortTable* st = structure->inter((*it)->sort());
			tablesize stsize = st->size();
			if(stsize._type == TST_EXACT || stsize._type == TST_APPROXIMATED) indexunivsizes.push_back(double(stsize._size));
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
						if(currresult < bestresult) { bestresult = currresult; }
					}
				}
				for(unsigned int n = 0; n < indicesvector.size(); ++n) {
					if(solve(kernel,indicesvector[n])) {
						double currresult = maxresult / indexunivsizes[n];
						if(currresult < bestresult) { bestresult = currresult; }
					}
				}
				return bestresult;
			}
			else { return maxdouble; }
		}
	}
	else if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		PredInter* pinter;
		if(typeid(*symbol) == typeid(Predicate)) { pinter = structure->inter(dynamic_cast<Predicate*>(symbol)); }
		else { pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter(); }
		const PredTable* pt;
		if(sign) {
			if(atomkernel->type() == AKT_CF) { pt = pinter->cf(); }
			else { pt = pinter->ct(); }
		}
		else {
			if(atomkernel->type() == AKT_CF) { pt = pinter->pt(); }
			else { pt = pinter->pf(); }
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
	double maxdouble = numeric_limits<double>::max();
	if(bdd == _truebdd) {
		tablesize univsize = univNrAnswers(vars,indices,structure);
		if(univsize._type == TST_INFINITE || univsize._type == TST_UNKNOWN) return maxdouble;
		else return double(univsize._size);
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
			tablesize kernelunivsize = univNrAnswers(kernelvars,kernelindices,structure);
			double invkernans = (kernelunivsize._type == TST_INFINITE || kernelunivsize._type == TST_UNKNOWN) ? maxdouble : double(kernelunivsize._size) - kernelans;
			double falsecost = estimatedCostAll(bdd->falsebranch(),bddvars,bddindices,structure);
			if( kernelcost + (invkernans * falsecost) < maxdouble) {
				return kernelcost + (invkernans * falsecost);
			}
			else return maxdouble;
		}
		else {
			tablesize kernelunivsize = univNrAnswers(kernelvars,kernelindices,structure);
			set<const FOBDDVariable*> emptyvars;
			set<const FOBDDDeBruijnIndex*> emptyindices;
			double kernelcost = estimatedCostAll(true,bdd->kernel(),emptyvars,emptyindices,structure);
			double truecost = estimatedCostAll(bdd->truebranch(),bddvars,bddindices,structure);
			double falsecost = estimatedCostAll(bdd->falsebranch(),bddvars,bddindices,structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(),kernelvars,kernelindices,structure);
			if(kernelunivsize._type == TST_UNKNOWN || kernelunivsize._type == TST_INFINITE) return maxdouble;
			else {
				if((double(kernelunivsize._size) * kernelcost) + (double(kernelans) * truecost) + ((kernelunivsize._size - kernelans) * falsecost) < maxdouble) {
					return (double(kernelunivsize._size) * kernelcost) + (double(kernelans) * truecost) + ((kernelunivsize._size - kernelans) * falsecost);
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


const FOBDD* FOBDDManager::make_more_false(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure, double weight_per_ans) {
	
	if(isTruebdd(bdd) || isFalsebdd(bdd)) {
		return bdd;
	}
	else {
		// Split variables
		set<const FOBDDVariable*> kvars = variables(bdd->kernel());
		set<const FOBDDDeBruijnIndex*> kindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*> kernelvars;
		set<const FOBDDVariable*> branchvars;
		set<const FOBDDDeBruijnIndex*> kernelindices;
		set<const FOBDDDeBruijnIndex*> branchindices;
		for(auto it = vars.begin(); it != vars.end(); ++it) {
			if(kvars.find(*it) == kvars.end()) branchvars.insert(*it);
			else kernelvars.insert(*it);
		}
		for(auto it = indices.begin(); it != indices.end(); ++it) {
			if(kindices.find(*it) == kindices.end()) branchindices.insert(*it);
			else kernelindices.insert(*it);
		}

		// Recursive call
		double bddcost = estimatedCostAll(bdd,vars,indices,structure);
		double bddans = estimatedNrAnswers(bdd,vars,indices,structure);
		double totalbddcost = numeric_limits<double>::max();
		if(bddcost + (bddans * weight_per_ans) < totalbddcost) {
			totalbddcost = bddcost + (bddans * weight_per_ans);
		}

		if(isTruebdd(bdd->falsebranch())) {
			double branchcost = estimatedCostAll(bdd->truebranch(),vars,indices,structure);
			double branchans = estimatedNrAnswers(bdd->truebranch(),vars,indices,structure);
			double totalbranchcost = numeric_limits<double>::max();
			if(branchcost + (branchans * weight_per_ans) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weight_per_ans);
			}
			if(totalbranchcost < totalbddcost) {
				return make_more_false(bdd->truebranch(),vars,indices,structure,weight_per_ans);
			}
		}
		else if(isTruebdd(bdd->truebranch())) {
			double branchcost = estimatedCostAll(bdd->falsebranch(),vars,indices,structure);
			double branchans = estimatedNrAnswers(bdd->falsebranch(),vars,indices,structure);
			double totalbranchcost = numeric_limits<double>::max();
			if(branchcost + (branchans * weight_per_ans) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weight_per_ans);
			}
			if(totalbranchcost < totalbddcost) {
				return make_more_false(bdd->falsebranch(),vars,indices,structure,weight_per_ans);
			}
		}

		double kernelans = estimatedNrAnswers(bdd->kernel(),kernelvars,kernelindices,structure);
		double truebranchweight = (kernelans * weight_per_ans < numeric_limits<double>::max()) ? kernelans * weight_per_ans: numeric_limits<double>::max();
		const FOBDD* newtrue = make_more_false(bdd->truebranch(),branchvars,branchindices,structure,truebranchweight);

		tablesize allkernelans = univNrAnswers(kernelvars,kernelindices,structure);
		double chance = estimatedChance(bdd->kernel(),structure);
		double invkernelans;
		if(allkernelans._type == TST_APPROXIMATED || allkernelans._type == TST_EXACT) {
			invkernelans = allkernelans._size * (1 - chance);
		}
		else {
			if(chance > 0) invkernelans = numeric_limits<double>::max();	
			else kernelans = 1;
		}
		double falsebranchweight = (invkernelans * weight_per_ans < numeric_limits<double>::max()) ? invkernelans * weight_per_ans: numeric_limits<double>::max();
		const FOBDD* newfalse = make_more_false(bdd->falsebranch(),branchvars,branchindices,structure,falsebranchweight);
		if(newtrue != bdd->truebranch() || newfalse != bdd->falsebranch()) {
			return make_more_false(getBDD(bdd->kernel(),newtrue,newfalse),vars,indices,structure,weight_per_ans);
		}
		else return bdd;
	}

/*
	if(isFalsebdd(bdd)) {
		return bdd;
	}
	else if(isTruebdd(bdd)) {
		if(max_cost_per_ans < 1) return _falsebdd;
		else return _truebdd;
	}
	else {
		// Split variables
		set<const FOBDDVariable*> kvars = variables(bdd->kernel());
		set<const FOBDDDeBruijnIndex*> kindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*> kernelvars;
		set<const FOBDDVariable*> branchvars;
		set<const FOBDDDeBruijnIndex*> kernelindices;
		set<const FOBDDDeBruijnIndex*> branchindices;
		for(auto it = vars.begin(); it != vars.end(); ++it) {
			if(kvars.find(*it) == kvars.end()) branchvars.insert(*it);
			else kernelvars.insert(*it);
		}
		for(auto it = indices.begin(); it != indices.end(); ++it) {
			if(kindices.find(*it) == kindices.end()) branchindices.insert(*it);
			else kernelindices.insert(*it);
		}

		// Simplify quantification kernels
		if(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel)) {
			const FOBDD* newfalse = make_more_false(bdd->falsebranch(),branchvars,branchindices,structure,max_cost_per_ans);
			const FOBDD* newtrue = make_more_false(bdd->truebranch(),branchvars,branchindices,structure,max_cost_per_ans);
			bdd = getBDD(bdd->kernel(),newtrue,newfalse);
			if(isFalsebdd(bdd)) return bdd;
			else {
				assert(!isTruebdd(bdd));
				assert(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel));
				const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(bdd->kernel());

				set<const FOBDDVariable*> emptyvars;
				set<const FOBDDDeBruijnIndex*> zeroindices;
				zeroindices.insert(getDeBruijnIndex(quantkernel->sort(),0));
				double quantans = estimatedNrAnswers(quantkernel,emptyvars,zeroindices,structure);
				if(quantans < 1) quantans = 1;
				double quant_per_ans = max_cost_per_ans / quantans;

				if(isFalsebdd(bdd->falsebranch())) {
					const FOBDD* newquant = make_more_false(quantkernel->bdd(),kernelvars,kernelindices,structure,quant_per_ans);
					const FOBDDKernel* newkernel = getQuantKernel(quantkernel->sort(),newquant);
					bdd = getBDD(newkernel,bdd->truebranch(),bdd->falsebranch());
				}
				else if(isFalsebdd(bdd->truebranch())) {
					const FOBDD* newquant = make_more_true(quantkernel->bdd(),kernelvars,kernelindices,structure,quant_per_ans);
					const FOBDDKernel* newkernel = getQuantKernel(quantkernel->sort(),newquant);
					bdd = getBDD(newkernel,bdd->truebranch(),bdd->falsebranch());
				}
			}
			if(isFalsebdd(bdd)) return bdd;
		}

		// Recursive call
		double branch_cost_ans;
		if(isFalsebdd(bdd->falsebranch())) {
			double kernelcost = estimatedCostAll(true,bdd->kernel(),kernelvars,kernelindices,structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(),kernelvars,kernelindices,structure);
			if(kernelans < 1) kernelans = 1;
			branch_cost_ans = max_cost_per_ans - (kernelcost / kernelans);
		}
		else if(isFalsebdd(bdd->truebranch())) {
			double kernelcost = estimatedCostAll(false,bdd->kernel(),kernelvars,kernelindices,structure);
			tablesize allkernelans = univNrAnswers(kernelvars,kernelindices,structure);
			double chance = estimatedChance(bdd->kernel(),structure);
			double kernelans;
			if(allkernelans._type == TST_APPROXIMATED || allkernelans._type == TST_EXACT) {
				kernelans = allkernelans._size * (1 - chance);
			}
			else {
				if(chance > 0) kernelans = numeric_limits<double>::max();	
				else kernelans = 1;
			}
			if(kernelans < 1) kernelans = 1;
			branch_cost_ans = max_cost_per_ans - (kernelcost / kernelans);
		}
		else {
			set<const FOBDDVariable*> emptyvars;
			set<const FOBDDDeBruijnIndex*> emptyindices;
			branch_cost_ans = max_cost_per_ans - estimatedCostAll(true,bdd->kernel(),emptyvars,emptyindices,structure);
		}
		const FOBDD* newfalse = make_more_false(bdd->falsebranch(),branchvars,branchindices,structure,branch_cost_ans);
		const FOBDD* newtrue = make_more_false(bdd->truebranch(),branchvars,branchindices,structure,branch_cost_ans);
		return getBDD(bdd->kernel(),newtrue,newfalse);
	}
	*/
}

const FOBDD* FOBDDManager::make_more_true(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure, double weight_per_ans) {
	if(isTruebdd(bdd) || isFalsebdd(bdd)) {
		return bdd;
	}
	else {
		// Split variables
		set<const FOBDDVariable*> kvars = variables(bdd->kernel());
		set<const FOBDDDeBruijnIndex*> kindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*> kernelvars;
		set<const FOBDDVariable*> branchvars;
		set<const FOBDDDeBruijnIndex*> kernelindices;
		set<const FOBDDDeBruijnIndex*> branchindices;
		for(auto it = vars.begin(); it != vars.end(); ++it) {
			if(kvars.find(*it) == kvars.end()) branchvars.insert(*it);
			else kernelvars.insert(*it);
		}
		for(auto it = indices.begin(); it != indices.end(); ++it) {
			if(kindices.find(*it) == kindices.end()) branchindices.insert(*it);
			else kernelindices.insert(*it);
		}

		// Recursive call
		double bddcost = estimatedCostAll(bdd,vars,indices,structure);
		double bddans = estimatedNrAnswers(bdd,vars,indices,structure);
		double totalbddcost = numeric_limits<double>::max();
		if(bddcost + (bddans * weight_per_ans) < totalbddcost) {
			totalbddcost = bddcost + (bddans * weight_per_ans);
		}

		if(isFalsebdd(bdd->falsebranch())) {
			double branchcost = estimatedCostAll(bdd->truebranch(),vars,indices,structure);
			double branchans = estimatedNrAnswers(bdd->truebranch(),vars,indices,structure);
			double totalbranchcost = numeric_limits<double>::max();
			if(branchcost + (branchans * weight_per_ans) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weight_per_ans);
			}
			if(totalbranchcost < totalbddcost) {
				return make_more_true(bdd->truebranch(),vars,indices,structure,weight_per_ans);
			}
		}
		else if(isFalsebdd(bdd->truebranch())) {
			double branchcost = estimatedCostAll(bdd->falsebranch(),vars,indices,structure);
			double branchans = estimatedNrAnswers(bdd->falsebranch(),vars,indices,structure);
			double totalbranchcost = numeric_limits<double>::max();
			if(branchcost + (branchans * weight_per_ans) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weight_per_ans);
			}
			if(totalbranchcost < totalbddcost) {
				return make_more_true(bdd->falsebranch(),vars,indices,structure,weight_per_ans);
			}
		}

		double kernelans = estimatedNrAnswers(bdd->kernel(),kernelvars,kernelindices,structure);
		double truebranchweight = (kernelans * weight_per_ans < numeric_limits<double>::max()) ? kernelans * weight_per_ans: numeric_limits<double>::max();
		const FOBDD* newtrue = make_more_true(bdd->truebranch(),branchvars,branchindices,structure,truebranchweight);

		tablesize allkernelans = univNrAnswers(kernelvars,kernelindices,structure);
		double chance = estimatedChance(bdd->kernel(),structure);
		double invkernelans;
		if(allkernelans._type == TST_APPROXIMATED || allkernelans._type == TST_EXACT) {
			invkernelans = allkernelans._size * (1 - chance);
		}
		else {
			if(chance > 0) invkernelans = numeric_limits<double>::max();	
			else kernelans = 1;
		}
		double falsebranchweight = (invkernelans * weight_per_ans < numeric_limits<double>::max()) ? invkernelans * weight_per_ans: numeric_limits<double>::max();
		const FOBDD* newfalse = make_more_true(bdd->falsebranch(),branchvars,branchindices,structure,falsebranchweight);
		if(newtrue != bdd->truebranch() || newfalse != bdd->falsebranch()) {
			return make_more_true(getBDD(bdd->kernel(),newtrue,newfalse),vars,indices,structure,weight_per_ans);
		}
		else return bdd;

		// Simplify quantification kernels
/*		if(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel)) {
			const FOBDD* newfalse = make_more_true(bdd->falsebranch(),branchvars,branchindices,structure,max_cost_per_ans);
			const FOBDD* newtrue = make_more_true(bdd->truebranch(),branchvars,branchindices,structure,max_cost_per_ans);
			bdd = getBDD(bdd->kernel(),newtrue,newfalse);
			if(isTruebdd(bdd)) return bdd;
			else {
				assert(!isFalsebdd(bdd));
				assert(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel));
				const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(bdd->kernel());

				set<const FOBDDVariable*> emptyvars;
				set<const FOBDDDeBruijnIndex*> zeroindices;
				zeroindices.insert(getDeBruijnIndex(quantkernel->sort(),0));
				double quantans = estimatedNrAnswers(quantkernel,emptyvars,zeroindices,structure);
				if(quantans < 1) quantans = 1;
				double quant_per_ans = max_cost_per_ans / quantans;

				if(isTruebdd(bdd->falsebranch())) {
					const FOBDD* newquant = make_more_false(quantkernel->bdd(),kernelvars,kernelindices,structure,quant_per_ans);
					const FOBDDKernel* newkernel = getQuantKernel(quantkernel->sort(),newquant);
					bdd = getBDD(newkernel,bdd->truebranch(),bdd->falsebranch());
				}
				else if(isTruebdd(bdd->truebranch())) {
					const FOBDD* newquant = make_more_true(quantkernel->bdd(),kernelvars,kernelindices,structure,quant_per_ans);
					const FOBDDKernel* newkernel = getQuantKernel(quantkernel->sort(),newquant);
					bdd = getBDD(newkernel,bdd->truebranch(),bdd->falsebranch());
				}
			}
			if(isTruebdd(bdd)) return bdd;
		}
*/
		// Recursive call
/*		double branch_cost_ans;
		if(isFalsebdd(bdd->falsebranch())) {
			double kernelcost = estimatedCostAll(true,bdd->kernel(),kernelvars,kernelindices,structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(),kernelvars,kernelindices,structure);
			if(kernelans < 1) kernelans = 1;
			branch_cost_ans = max_cost_per_ans - (kernelcost / kernelans);
		}
		else if(isFalsebdd(bdd->truebranch())) {
			double kernelcost = estimatedCostAll(false,bdd->kernel(),kernelvars,kernelindices,structure);
			tablesize allkernelans = univNrAnswers(kernelvars,kernelindices,structure);
			double chance = estimatedChance(bdd->kernel(),structure);
			double kernelans;
			if(allkernelans._type == TST_APPROXIMATED || allkernelans._type == TST_EXACT) {
				kernelans = allkernelans._size * (1 - chance);
			}
			else {
				if(chance > 0) kernelans = numeric_limits<double>::max();	
				else kernelans = 1;
			}
			if(kernelans < 1) kernelans = 1;
			branch_cost_ans = max_cost_per_ans - (kernelcost / kernelans);
		}
		else {
			set<const FOBDDVariable*> emptyvars;
			set<const FOBDDDeBruijnIndex*> emptyindices;
			branch_cost_ans = max_cost_per_ans - estimatedCostAll(true,bdd->kernel(),emptyvars,emptyindices,structure);
		}
		const FOBDD* newfalse = make_more_true(bdd->falsebranch(),branchvars,branchindices,structure,branch_cost_ans);
		const FOBDD* newtrue = make_more_true(bdd->truebranch(),branchvars,branchindices,structure,branch_cost_ans);
		return getBDD(bdd->kernel(),newtrue,newfalse);
*/
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
