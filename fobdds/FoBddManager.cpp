/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "fobdds/FoBddManager.hpp"
#include "fobdds/bddvisitors/OrderTerms.hpp"
#include "fobdds/bddvisitors/TermOccursNested.hpp"
#include "fobdds/bddvisitors/ContainsPartialFunctions.hpp"
#include "fobdds/bddvisitors/TermCollector.hpp"
#include "fobdds/bddvisitors/IndexCollector.hpp"
#include "fobdds/bddvisitors/VariableCollector.hpp"
#include "fobdds/bddvisitors/AddMultSimplifier.hpp"
#include "fobdds/bddvisitors/SubstituteTerms.hpp"
#include "fobdds/bddvisitors/ContainsFuncTerms.hpp"
#include "fobdds/bddvisitors/BumpIndices.hpp"
#include "fobdds/bddvisitors/TermsToLeft.hpp"
#include "fobdds/bddvisitors/Copy.hpp"
#include "fobdds/bddvisitors/RemoveMinus.hpp"
#include "fobdds/bddvisitors/UngraphFunctions.hpp"
#include "fobdds/bddvisitors/CollectSameOperationTerms.hpp"
#include "fobdds/bddvisitors/BddToFormula.hpp"
#include "fobdds/bddvisitors/CheckIsArithmeticFormula.hpp"
#include "fobdds/bddvisitors/ApplyDistributivity.hpp"
#include "fobdds/bddvisitors/ContainsTerm.hpp"
#include "fobdds/bddvisitors/CombineConstsOfMults.hpp"

#include "fobdds/EstimateBDDInferenceCost.hpp"

using namespace std;

extern int global_seed; // TODO part of global data or options!

static unsigned int STANDARDCATEGORY = 1;
static unsigned int DEBRUIJNCATEGORY = 2;
static unsigned int TRUEFALSECATEGORY = 3;

KernelOrder FOBDDManager::newOrder(unsigned int category) {
	KernelOrder order(category, _nextorder[category]);
	++_nextorder[category];
	return order;
}

KernelOrder FOBDDManager::newOrder(const vector<const FOBDDArgument*>& args) {
	unsigned int category = STANDARDCATEGORY;
	for (size_t n = 0; n < args.size(); ++n) {
		if (args[n]->containsFreeDeBruijnIndex()) {
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
	if (cat != TRUEFALSECATEGORY) {
		unsigned int nr = kernel->number();
		if (nr != 0) {
			--nr;
			const FOBDDKernel* pkernel = _kernels[cat][nr];
			moveDown(pkernel);
		}
	}
}

void FOBDDManager::moveDown(const FOBDDKernel* kernel) {
	clearDynamicTables();
	unsigned int cat = kernel->category();
	if (cat != TRUEFALSECATEGORY) {
		unsigned int nr = kernel->number();
		vector<const FOBDD*> falseerase;
		vector<const FOBDD*> trueerase;
		if (_kernels[cat].find(nr + 1) == _kernels[cat].cend()) {
			return;
		}
		const FOBDDKernel* nextkernel = _kernels[cat][nr + 1];
		const MBDDMBDDBDD& bdds = _bddtable[kernel];
		for (auto it = bdds.cbegin(); it != bdds.cend(); ++it) {
			for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
				FOBDD* bdd = jt->second;
				bool swapfalse = (nextkernel == it->first->kernel());
				bool swaptrue = (nextkernel == jt->first->kernel());
				if (swapfalse || swaptrue) {
					falseerase.push_back(it->first);
					trueerase.push_back(jt->first);
				}
				if (swapfalse) {
					if (swaptrue) {
						const FOBDD* newfalse = getBDD(kernel, jt->first->falsebranch(), it->first->falsebranch());
						const FOBDD* newtrue = getBDD(kernel, jt->first->truebranch(), it->first->truebranch());
						bdd->replacefalse(newfalse);
						bdd->replacetrue(newtrue);
						bdd->replacekernel(nextkernel);
						_bddtable[nextkernel][newfalse][newtrue] = bdd;
					} else {
						const FOBDD* newfalse = getBDD(kernel, jt->first, it->first->falsebranch());
						const FOBDD* newtrue = getBDD(kernel, jt->first, it->first->truebranch());
						bdd->replacefalse(newfalse);
						bdd->replacetrue(newtrue);
						bdd->replacekernel(nextkernel);
						_bddtable[nextkernel][newfalse][newtrue] = bdd;
					}
				} else if (swaptrue) {
					const FOBDD* newfalse = getBDD(kernel, jt->first->falsebranch(), it->first);
					const FOBDD* newtrue = getBDD(kernel, jt->first->truebranch(), it->first);
					bdd->replacefalse(newfalse);
					bdd->replacetrue(newtrue);
					bdd->replacekernel(nextkernel);
					_bddtable[nextkernel][newfalse][newtrue] = bdd;
				}
			}
		}
		for (unsigned int n = 0; n < falseerase.size(); ++n) {
			_bddtable[kernel][falseerase[n]].erase(trueerase[n]);
			if (_bddtable[kernel][falseerase[n]].empty()) {
				_bddtable[kernel].erase(falseerase[n]);
			}
		}
		FOBDDKernel* tkernel = _kernels[cat][nr];
		FOBDDKernel* nkernel = _kernels[cat][nr + 1];
		nkernel->replacenumber(nr);
		tkernel->replacenumber(nr + 1);
		_kernels[cat][nr] = nkernel;
		_kernels[cat][nr + 1] = tkernel;
	}
}

const FOBDD* FOBDDManager::getBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	// Simplification
	if (kernel == _truekernel) {
		return truebranch;
	}
	if (kernel == _falsekernel) {
		return falsebranch;
	}
	if (falsebranch == truebranch) {
		return falsebranch;
	}

	// Lookup
	BDDTable::const_iterator it = _bddtable.find(kernel);
	if (it != _bddtable.cend()) {
		MBDDMBDDBDD::const_iterator jt = it->second.find(falsebranch);
		if (jt != it->second.cend()) {
			MBDDBDD::const_iterator kt = jt->second.find(truebranch);
			if (kt != jt->second.cend()) {
				return kt->second;
			}
		}
	}

	// Lookup failed, create a new bdd
	return addBDD(kernel, truebranch, falsebranch);

}

const FOBDD* FOBDDManager::getBDD(const FOBDD* bdd, FOBDDManager* manager) {
	Copy copier(manager, this);
	const FOBDD* res = copier.copy(bdd);
	return res;
}

FOBDD* FOBDDManager::addBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	FOBDD* newbdd = new FOBDD(kernel, truebranch, falsebranch);
	_bddtable[kernel][falsebranch][truebranch] = newbdd;
	return newbdd;
}

const FOBDDArgument* FOBDDManager::invert(const FOBDDArgument* arg) {
	const DomainElement* minus_one = createDomElem(-1);
	const FOBDDArgument* minus_one_term = getDomainTerm(VocabularyUtils::intsort(), minus_one);
	Function* times = Vocabulary::std()->func("*/2");
	times = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(VocabularyUtils::intsort(), arg->sort())), 0);
	vector<const FOBDDArgument*> timesterms(2);
	timesterms[0] = minus_one_term;
	timesterms[1] = arg;
	return getFuncTerm(times, timesterms);
}

const FOBDDKernel* FOBDDManager::getAtomKernel(PFSymbol* symbol, AtomKernelType akt, const vector<const FOBDDArgument*>& args) {
	// Simplification
	if (symbol->name() == "=/2") {
		if (args[0] == args[1]) {
			return _truekernel;
		}
	} else if (args.size() == 1) {
		if (symbol->sorts()[0]->pred() == symbol) {
			if (SortUtils::isSubsort(args[0]->sort(), symbol->sorts()[0])) {
				return _truekernel;
			}
		}
	}

	// Arithmetic rewriting
	// 1. Remove functions
	if (typeid(*symbol) == typeid(Function) && akt == AtomKernelType::AKT_TWOVALUED) {
		Function* f = dynamic_cast<Function*>(symbol);
		Sort* s = SortUtils::resolve(f->outsort(), args.back()->sort());
		Predicate* equal = VocabularyUtils::equal(s);
		vector<const FOBDDArgument*> funcargs = args;
		funcargs.pop_back();
		const FOBDDArgument* functerm = getFuncTerm(f, funcargs);
		vector<const FOBDDArgument*> newargs;
		newargs.push_back(functerm);
		newargs.push_back(args.back());
		return getAtomKernel(equal, AtomKernelType::AKT_TWOVALUED, newargs);
	}
	// 2. Move all arithmetic terms to the lefthand side of an (in)equality
	if (VocabularyUtils::isComparisonPredicate(symbol)) {
		const FOBDDArgument* leftarg = args[0];
		const FOBDDArgument* rightarg = args[1];
		if (VocabularyUtils::isNumeric(rightarg->sort())) {
			Assert(VocabularyUtils::isNumeric(leftarg->sort()));
			if (typeid(*rightarg) != typeid(FOBDDDomainTerm) || dynamic_cast<const FOBDDDomainTerm*>(rightarg)->value() != createDomElem(0)) {
				const DomainElement* zero = createDomElem(0);
				const FOBDDDomainTerm* zero_term = getDomainTerm(VocabularyUtils::natsort(), zero);
				const FOBDDArgument* minus_rightarg = invert(rightarg);
				Function* plus = Vocabulary::std()->func("+/2");
				plus = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(leftarg->sort(), rightarg->sort())), 0);
				Assert(plus);
				vector<const FOBDDArgument*> plusargs(2);
				plusargs[0] = leftarg;
				plusargs[1] = minus_rightarg;
				const FOBDDArgument* plusterm = getFuncTerm(plus, plusargs);
				vector<const FOBDDArgument*> newargs(2);
				newargs[0] = plusterm;
				newargs[1] = zero_term;
				return getAtomKernel(symbol, akt, newargs);
			}
		}
	}

	// Comparison rewriting
	if (VocabularyUtils::isComparisonPredicate(symbol)) {
		if (Multiplication::before(args[0], args[1])) {
			vector<const FOBDDArgument*> newargs(2);
			newargs[0] = args[1];
			newargs[1] = args[0];
			Predicate* newsymbol = dynamic_cast<Predicate*>(symbol);
			if (symbol->name() == "</2") {
				newsymbol = VocabularyUtils::greaterThan(symbol->sorts()[0]);
			} else if (symbol->name() == ">/2") {
				newsymbol = VocabularyUtils::lessThan(symbol->sorts()[0]);
			}
			return getAtomKernel(newsymbol, akt, newargs);
		}
	}

	// Lookup
	AtomKernelTable::const_iterator it = _atomkerneltable.find(symbol);
	if (it != _atomkerneltable.cend()) {
		MAKTMVAGAK::const_iterator jt = it->second.find(akt);
		if (jt != it->second.cend()) {
			MVAGAK::const_iterator kt = jt->second.find(args);
			if (kt != jt->second.cend()) {
				return kt->second;
			}
		}
	}

	// Lookup failed, create a new atom kernel
	return addAtomKernel(symbol, akt, args);
}

FOBDDAtomKernel* FOBDDManager::addAtomKernel(PFSymbol* symbol, AtomKernelType akt, const vector<const FOBDDArgument*>& args) {
	FOBDDAtomKernel* newkernel = new FOBDDAtomKernel(symbol, akt, args, newOrder(args));
	_atomkerneltable[symbol][akt][args] = newkernel;
	_kernels[newkernel->category()][newkernel->number()] = newkernel;
	return newkernel;
}

const FOBDDKernel* FOBDDManager::getQuantKernel(Sort* sort, const FOBDD* bdd) {
	// Simplification
	if (bdd == _truebdd) {
		return _truekernel;
	} else if (bdd == _falsebdd) {
		return _falsekernel;
	} else if (longestbranch(bdd) == 2) {
		const FOBDDDeBruijnIndex* qvar = getDeBruijnIndex(sort, 0);
		const FOBDDArgument* arg = solve(bdd->kernel(), qvar);
		if (arg && !partial(arg)) {
			if ((bdd->truebranch() == _truebdd && SortUtils::isSubsort(arg->sort(), sort)) || sort->builtin()) {
				return _truekernel;
			}
			// NOTE: sort->builtin() is used here as an approximate test to see if the sort contains more than
			// one domain element. If that is the case, (? y : F(x) ~= y) is indeed true.
		}
	}

	// Lookup
	QuantKernelTable::const_iterator it = _quantkerneltable.find(sort);
	if (it != _quantkerneltable.cend()) {
		MBDDQK::const_iterator jt = it->second.find(bdd);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}

	// Lookup failed, create a new quantified kernel
	return addQuantKernel(sort, bdd);
}

FOBDDQuantKernel* FOBDDManager::addQuantKernel(Sort* sort, const FOBDD* bdd) {
	FOBDDQuantKernel* newkernel = new FOBDDQuantKernel(sort, bdd, newOrder(bdd));
	_quantkerneltable[sort][bdd] = newkernel;
	_kernels[newkernel->category()][newkernel->number()] = newkernel;
	return newkernel;
}

const FOBDDVariable* FOBDDManager::getVariable(Variable* var) {
	// Lookup
	VariableTable::const_iterator it = _variabletable.find(var);
	if (it != _variabletable.cend()) {
		return it->second;
	}

	// Lookup failed, create a new variable
	return addVariable(var);
}

set<const FOBDDVariable*> FOBDDManager::getVariables(const set<Variable*>& vars) {
	set<const FOBDDVariable*> bddvars;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
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
	if (it != _debruijntable.cend()) {
		MUIDB::const_iterator jt = it->second.find(index);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}
	// Lookup failed, create a new De Bruijn index
	return addDeBruijnIndex(sort, index);
}

FOBDDDeBruijnIndex* FOBDDManager::addDeBruijnIndex(Sort* sort, unsigned int index) {
	FOBDDDeBruijnIndex* newindex = new FOBDDDeBruijnIndex(sort, index);
	_debruijntable[sort][index] = newindex;
	return newindex;
}

const FOBDDArgument* FOBDDManager::getFuncTerm(Function* func, const vector<const FOBDDArgument*>& args) {

//clog << "Get functerm on function " << *func << " and arguments ";
//for(auto it = args.cbegin(); it != args.cend(); ++it) {
//	put(clog,*it); clog << "   ";
//}
//clog << endl;

	// Arithmetic rewriting
	// 1. Remove unary minus
	if (func->name() == "-/1" && Vocabulary::std()->contains(func)) {
		return invert(args[0]);
	}
	// 2. Remove binary minus
	if (func->name() == "-/2" && Vocabulary::std()->contains(func)) {
		const FOBDDArgument* invright = invert(args[1]);
		Function* plus = Vocabulary::std()->func("+/2");
		plus = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(args[0]->sort(), invright->sort())), 0);
		vector<const FOBDDArgument*> newargs(2);
		newargs[0] = args[0];
		newargs[1] = invright;
		return getFuncTerm(plus, newargs);
	}
	if (func->name() == "*/2" && Vocabulary::std()->contains(func)) {
		// 3. Execute computable multiplications
		if (typeid(*(args[0])) == typeid(FOBDDDomainTerm)) {
			const FOBDDDomainTerm* leftterm = dynamic_cast<const FOBDDDomainTerm*>(args[0]);
			if (leftterm->value()->type() == DET_INT) {
				if (leftterm->value()->value()._int == 0) {
					return leftterm;
				} else if (leftterm->value()->value()._int == 1) {
					return args[1];
				}
			}
			if (typeid(*(args[1])) == typeid(FOBDDDomainTerm)) {
				const FOBDDDomainTerm* rightterm = dynamic_cast<const FOBDDDomainTerm*>(args[1]);
				FuncInter* fi = func->interpretation(0);
				vector<const DomainElement*> multargs(2);
				multargs[0] = leftterm->value();
				multargs[1] = rightterm->value();
				const DomainElement* result = fi->funcTable()->operator[](multargs);
				return getDomainTerm(func->outsort(), result);
			}
		}
		// 4. Apply distributivity of */2 with respect to +/2
		if (typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
			if (leftterm->func()->name() == "+/2" && Vocabulary::std()->contains(leftterm->func())) {
				vector<const FOBDDArgument*> newleftargs(2);
				newleftargs[0] = leftterm->args(0);
				newleftargs[1] = args[1];
				vector<const FOBDDArgument*> newrightargs(2);
				newrightargs[0] = leftterm->args(1);
				newrightargs[1] = args[1];
				const FOBDDArgument* newleft = getFuncTerm(func, newleftargs);
				const FOBDDArgument* newright = getFuncTerm(func, newrightargs);
				vector<const FOBDDArgument*> newargs(2);
				newargs[0] = newleft;
				newargs[1] = newright;
				return getFuncTerm(leftterm->func(), newargs);
			}
		}
		if (typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
			if (rightterm->func()->name() == "+/2" && Vocabulary::std()->contains(rightterm->func())) {
				vector<const FOBDDArgument*> newleftargs(2);
				newleftargs[0] = args[0];
				newleftargs[1] = rightterm->args(0);
				vector<const FOBDDArgument*> newrightargs(2);
				newrightargs[0] = args[0];
				newrightargs[1] = rightterm->args(1);
				const FOBDDArgument* newleft = getFuncTerm(func, newleftargs);
				const FOBDDArgument* newright = getFuncTerm(func, newrightargs);
				vector<const FOBDDArgument*> newargs(2);
				newargs[0] = newleft;
				newargs[1] = newright;
				return getFuncTerm(rightterm->func(), newargs);
			}
		}
		// 5. Apply commutativity and associativity to obtain
		// a sorted multiplication of the form ((((t1 * t2) * t3) * t4) * ...)
		if (typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
			if (leftterm->func()->name() == "*/2" && Vocabulary::std()->contains(leftterm->func())) {
				if (typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
					if (rightterm->func()->name() == "*/2" && Vocabulary::std()->contains(rightterm->func())) {
						Function* times = Vocabulary::std()->func("*/2");
						Function* times1 = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(leftterm->sort(), rightterm->args(1)->sort())), 0);
						vector<const FOBDDArgument*> leftargs(2);
						leftargs[0] = leftterm;
						leftargs[1] = rightterm->args(1);
						const FOBDDArgument* newleft = getFuncTerm(times1, leftargs);
						Function* times2 = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(newleft->sort(), rightterm->args(0)->sort())), 0);
						vector<const FOBDDArgument*> newargs(2);
						newargs[0] = newleft;
						newargs[1] = rightterm->args(0);
						return getFuncTerm(times2, newargs);
					}
				}
				if (Multiplication::before(args[1], leftterm->args(1))) {
					Function* times = Vocabulary::std()->func("*/2");
					Function* times1 = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(args[1]->sort(), leftterm->args(0)->sort())), 0);
					vector<const FOBDDArgument*> leftargs(2);
					leftargs[0] = leftterm->args(0);
					leftargs[1] = args[1];
					const FOBDDArgument* newleft = getFuncTerm(times1, leftargs);
					Function* times2 = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(newleft->sort(), leftterm->args(1)->sort())), 0);
					vector<const FOBDDArgument*> newargs(2);
					newargs[0] = newleft;
					newargs[1] = leftterm->args(1);
					return getFuncTerm(times2, newargs);
				}
			}
		} else if (typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
			if (rightterm->func()->name() == "*/2" && Vocabulary::std()->contains(rightterm->func())) {
				vector<const FOBDDArgument*> newargs(2);
				newargs[0] = args[1];
				newargs[1] = args[0];
				return getFuncTerm(func, newargs);
			} else if (Multiplication::before(args[1], args[0])) {
				vector<const FOBDDArgument*> newargs(2);
				newargs[0] = args[1];
				newargs[1] = args[0];
				return getFuncTerm(func, newargs);
			}
		} else if (Multiplication::before(args[1], args[0])) {
			vector<const FOBDDArgument*> newargs(2);
			newargs[0] = args[1];
			newargs[1] = args[0];
			return getFuncTerm(func, newargs);
		}
	} else if (func->name() == "+/2" && Vocabulary::std()->contains(func)) {
		// 6. Execute computable additions
		if (typeid(*(args[0])) == typeid(FOBDDDomainTerm)) {
			const FOBDDDomainTerm* leftterm = dynamic_cast<const FOBDDDomainTerm*>(args[0]);
			if (leftterm->value()->type() == DET_INT && leftterm->value()->value()._int == 0) {
				return args[1];
			}
			if (typeid(*(args[1])) == typeid(FOBDDDomainTerm)) {
				const FOBDDDomainTerm* rightterm = dynamic_cast<const FOBDDDomainTerm*>(args[1]);
				FuncInter* fi = func->interpretation(0);
				vector<const DomainElement*> plusargs(2);
				plusargs[0] = leftterm->value();
				plusargs[1] = rightterm->value();
				const DomainElement* result = fi->funcTable()->operator[](plusargs);
				return getDomainTerm(func->outsort(), result);
			}
		}

		// 7. Apply commutativity and associativity to
		// obtain a sorted addition of the form ((((t1 + t2) + t3) + t4) + ...)
		if (typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
			if (leftterm->func()->name() == "+/2" && Vocabulary::std()->contains(leftterm->func())) {
				if (typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
					if (rightterm->func()->name() == "+/2" && Vocabulary::std()->contains(rightterm->func())) {
						Function* plus = Vocabulary::std()->func("+/2");
						Function* plus1 = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(leftterm->sort(), rightterm->args(1)->sort())), 0);
						vector<const FOBDDArgument*> leftargs(2);
						leftargs[0] = leftterm;
						leftargs[1] = rightterm->args(1);
						const FOBDDArgument* newleft = getFuncTerm(plus1, leftargs);
						Function* plus2 = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(newleft->sort(), rightterm->args(0)->sort())), 0);
						vector<const FOBDDArgument*> newargs(2);
						newargs[0] = newleft;
						newargs[1] = rightterm->args(0);
						return getFuncTerm(plus2, newargs);
					}
				}
				if (TermOrder::before(args[1], leftterm->args(1), this)) {
					Function* plus = Vocabulary::std()->func("+/2");
					Function* plus1 = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(args[1]->sort(), leftterm->args(0)->sort())), 0);
					vector<const FOBDDArgument*> leftargs(2);
					leftargs[0] = leftterm->args(0);
					leftargs[1] = args[1];
					const FOBDDArgument* newleft = getFuncTerm(plus1, leftargs);
					Function* plus2 = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(newleft->sort(), leftterm->args(1)->sort())), 0);
					vector<const FOBDDArgument*> newargs(2);
					newargs[0] = newleft;
					newargs[1] = leftterm->args(1);
					return getFuncTerm(plus2, newargs);
				}
			}
		} else if (typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
			if (rightterm->func()->name() == "+/2" && Vocabulary::std()->contains(rightterm->func())) {
				vector<const FOBDDArgument*> newargs(2);
				newargs[0] = args[1];
				newargs[1] = args[0];
				return getFuncTerm(func, newargs);
			} else if (TermOrder::before(args[1], args[0], this)) {
				vector<const FOBDDArgument*> newargs(2);
				newargs[0] = args[1];
				newargs[1] = args[0];
				return getFuncTerm(func, newargs);
			}
		} else if (TermOrder::before(args[1], args[0], this)) {
			vector<const FOBDDArgument*> newargs(2);
			newargs[0] = args[1];
			newargs[1] = args[0];
			return getFuncTerm(func, newargs);
		}

		// 8. Add terms with the same non-constant part
		const FOBDDArgument* left = 0;
		const FOBDDArgument* right = 0;
		if (typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
			const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
			if (leftterm->func()->name() == "+/2" && Vocabulary::std()->contains(leftterm->func())) {
				left = leftterm->args(0);
				right = leftterm->args(1);
			} else {
				right = args[0];
			}
		} else {
			right = args[0];
		}
		CollectSameOperationTerms<Multiplication> collect(this);
		vector<const FOBDDArgument*> leftflat = collect.getTerms(right);
		vector<const FOBDDArgument*> rightflat = collect.getTerms(args[1]);
		if (leftflat.size() == rightflat.size()) {
			unsigned int n = 1;
			for (; n < leftflat.size(); ++n) {
				if (leftflat[n] != rightflat[n])
					break;
			}
			if (n == leftflat.size()) {
				Function* plus = Vocabulary::std()->func("+/2");
				plus = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(leftflat[0]->sort(), rightflat[0]->sort())), 0);
				vector<const FOBDDArgument*> firstargs(2);
				firstargs[0] = leftflat[0];
				firstargs[1] = rightflat[0];
				const FOBDDArgument* currterm = getFuncTerm(plus, firstargs);
				for (unsigned int m = 1; m < leftflat.size(); ++m) {
					Function* times = Vocabulary::std()->func("*/2");
					times = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(currterm->sort(), leftflat[m]->sort())), 0);
					vector<const FOBDDArgument*> nextargs(2);
					nextargs[0] = currterm;
					nextargs[1] = leftflat[m];
					currterm = getFuncTerm(times, nextargs);
				}
				if (left) {
					Function* plus1 = Vocabulary::std()->func("+/2");
					plus1 = plus1->disambiguate(vector<Sort*>(3, SortUtils::resolve(currterm->sort(), left->sort())), 0);
					vector<const FOBDDArgument*> lastargs(2);
					lastargs[0] = left;
					lastargs[1] = currterm;
					return getFuncTerm(plus1, lastargs);
				} else {
					return currterm;
				}
			}
		}
	}

	// Lookup
	FuncTermTable::const_iterator it = _functermtable.find(func);
	if (it != _functermtable.cend()) {
		MVAFT::const_iterator jt = it->second.find(args);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}
	// Lookup failed, create a new funcion term
	return addFuncTerm(func, args);
}

FOBDDFuncTerm* FOBDDManager::addFuncTerm(Function* func, const vector<const FOBDDArgument*>& args) {
	FOBDDFuncTerm* newarg = new FOBDDFuncTerm(func, args);
	_functermtable[func][args] = newarg;
	return newarg;
}

const FOBDDDomainTerm* FOBDDManager::getDomainTerm(Sort* sort, const DomainElement* value) {
	// Lookup
	DomainTermTable::const_iterator it = _domaintermtable.find(sort);
	if (it != _domaintermtable.cend()) {
		MTEDT::const_iterator jt = it->second.find(value);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}
	// Lookup failed, create a new funcion term
	return addDomainTerm(sort, value);
}

FOBDDDomainTerm* FOBDDManager::addDomainTerm(Sort* sort, const DomainElement* value) {
	FOBDDDomainTerm* newdt = new FOBDDDomainTerm(sort, value);
	_domaintermtable[sort][value] = newdt;
	return newdt;
}

/*************************
 Logical operations
 *************************/

const FOBDD* FOBDDManager::negation(const FOBDD* bdd) {
	// Base cases
	if (bdd == _truebdd) {
		return _falsebdd;
	}
	if (bdd == _falsebdd) {
		return _truebdd;
	}

	// Recursive case
	map<const FOBDD*, const FOBDD*>::iterator it = _negationtable.find(bdd);
	if (it != _negationtable.cend()) {
		return it->second;
	} else {
		const FOBDD* falsebranch = negation(bdd->falsebranch());
		const FOBDD* truebranch = negation(bdd->truebranch());
		const FOBDD* result = getBDD(bdd->kernel(), truebranch, falsebranch);
		_negationtable[bdd] = result;
		return result;
	}
}

const FOBDD* FOBDDManager::conjunction(const FOBDD* bdd1, const FOBDD* bdd2) {
	// Base cases
	if (bdd1 == _falsebdd || bdd2 == _falsebdd) {
		return _falsebdd;
	}
	if (bdd1 == _truebdd) {
		return bdd2;
	}
	if (bdd2 == _truebdd) {
		return bdd1;
	}
	if (bdd1 == bdd2) {
		return bdd1;
	}

	// Recursive case
	if (bdd2 < bdd1) {
		const FOBDD* temp = bdd1;
		bdd1 = bdd2;
		bdd2 = temp;
	}
	map<const FOBDD*, map<const FOBDD*, const FOBDD*> >::iterator it = _conjunctiontable.find(bdd1);
	if (it != _conjunctiontable.cend()) {
		map<const FOBDD*, const FOBDD*>::iterator jt = it->second.find(bdd2);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}
	const FOBDD* result = 0;
	if (*(bdd1->kernel()) < *(bdd2->kernel())) {
		const FOBDD* falsebranch = conjunction(bdd1->falsebranch(), bdd2);
		const FOBDD* truebranch = conjunction(bdd1->truebranch(), bdd2);
		result = getBDD(bdd1->kernel(), truebranch, falsebranch);
	} else if (*(bdd1->kernel()) > *(bdd2->kernel())) {
		const FOBDD* falsebranch = conjunction(bdd1, bdd2->falsebranch());
		const FOBDD* truebranch = conjunction(bdd1, bdd2->truebranch());
		result = getBDD(bdd2->kernel(), truebranch, falsebranch);
	} else {
		Assert(bdd1->kernel() == bdd2->kernel());
		const FOBDD* falsebranch = conjunction(bdd1->falsebranch(), bdd2->falsebranch());
		const FOBDD* truebranch = conjunction(bdd1->truebranch(), bdd2->truebranch());
		result = getBDD(bdd1->kernel(), truebranch, falsebranch);
	}
	_conjunctiontable[bdd1][bdd2] = result;
	return result;
}

const FOBDD* FOBDDManager::disjunction(const FOBDD* bdd1, const FOBDD* bdd2) {
	// Base cases
	if (bdd1 == _truebdd || bdd2 == _truebdd) {
		return _truebdd;
	}
	if (bdd1 == _falsebdd) {
		return bdd2;
	}
	if (bdd2 == _falsebdd) {
		return bdd1;
	}
	if (bdd1 == bdd2) {
		return bdd1;
	}

	// Recursive case
	if (bdd2 < bdd1) {
		const FOBDD* temp = bdd1;
		bdd1 = bdd2;
		bdd2 = temp;
	}
	map<const FOBDD*, map<const FOBDD*, const FOBDD*> >::iterator it = _disjunctiontable.find(bdd1);
	if (it != _disjunctiontable.cend()) {
		map<const FOBDD*, const FOBDD*>::iterator jt = it->second.find(bdd2);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}
	const FOBDD* result = 0;
	if (*(bdd1->kernel()) < *(bdd2->kernel())) {
		const FOBDD* falsebranch = disjunction(bdd1->falsebranch(), bdd2);
		const FOBDD* truebranch = disjunction(bdd1->truebranch(), bdd2);
		result = getBDD(bdd1->kernel(), truebranch, falsebranch);
	} else if (*(bdd1->kernel()) > *(bdd2->kernel())) {
		const FOBDD* falsebranch = disjunction(bdd1, bdd2->falsebranch());
		const FOBDD* truebranch = disjunction(bdd1, bdd2->truebranch());
		result = getBDD(bdd2->kernel(), truebranch, falsebranch);
	} else {
		Assert(bdd1->kernel() == bdd2->kernel());
		const FOBDD* falsebranch = disjunction(bdd1->falsebranch(), bdd2->falsebranch());
		const FOBDD* truebranch = disjunction(bdd1->truebranch(), bdd2->truebranch());
		result = getBDD(bdd1->kernel(), truebranch, falsebranch);
	}
	_disjunctiontable[bdd1][bdd2] = result;
	return result;
}

const FOBDD* FOBDDManager::ifthenelse(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	auto it = _ifthenelsetable.find(kernel);
	if (it != _ifthenelsetable.cend()) {
		auto jt = it->second.find(truebranch);
		if (jt != it->second.cend()) {
			auto kt = jt->second.find(falsebranch);
			if (kt != jt->second.cend()) {
				return kt->second;
			}
		}
	}

	const FOBDDKernel* truekernel = truebranch->kernel();
	const FOBDDKernel* falsekernel = falsebranch->kernel();

	if (*kernel < *truekernel) {
		if (*kernel < *falsekernel) {
			return getBDD(kernel, truebranch, falsebranch);
		} else if (kernel == falsekernel) {
			return getBDD(kernel, truebranch, falsebranch->falsebranch());
		} else {
			Assert(*kernel > *falsekernel);
			const FOBDD* newtrue = ifthenelse(kernel, truebranch, falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel, truebranch, falsebranch->falsebranch());
			return getBDD(falsekernel, newtrue, newfalse);
		}
	} else if (kernel == truekernel) {
		if (*kernel < *falsekernel) {
			return getBDD(kernel, truebranch->truebranch(), falsebranch);
		} else if (kernel == falsekernel) {
			return getBDD(kernel, truebranch->truebranch(), falsebranch->falsebranch());
		} else {
			Assert(*kernel > *falsekernel);
			const FOBDD* newtrue = ifthenelse(kernel, truebranch, falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel, truebranch, falsebranch->falsebranch());
			return getBDD(falsekernel, newtrue, newfalse);
		}
	} else {
		Assert(*kernel > *truekernel);
		if (*kernel < *falsekernel || kernel == falsekernel || *truekernel < *falsekernel) {
			const FOBDD* newtrue = ifthenelse(kernel, truebranch->truebranch(), falsebranch);
			const FOBDD* newfalse = ifthenelse(kernel, truebranch->falsebranch(), falsebranch);
			return getBDD(truekernel, newtrue, newfalse);
		} else if (truekernel == falsekernel) {
			const FOBDD* newtrue = ifthenelse(kernel, truebranch->truebranch(), falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel, truebranch->falsebranch(), falsebranch->falsebranch());
			return getBDD(truekernel, newtrue, newfalse);
		} else {
			Assert(*falsekernel < *truekernel);
			const FOBDD* newtrue = ifthenelse(kernel, truebranch, falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel, truebranch, falsebranch->falsebranch());
			return getBDD(falsekernel, newtrue, newfalse);
		}
	}
}

const FOBDD* FOBDDManager::univquantify(const FOBDDVariable* var, const FOBDD* bdd) {
	const FOBDD* negatedbdd = negation(bdd);
	const FOBDD* quantbdd = existsquantify(var, negatedbdd);
	return negation(quantbdd);
}

const FOBDD* FOBDDManager::univquantify(const set<const FOBDDVariable*>& qvars, const FOBDD* bdd) {
	const FOBDD* negatedbdd = negation(bdd);
	const FOBDD* quantbdd = existsquantify(qvars, negatedbdd);
	return negation(quantbdd);
}

const FOBDD* FOBDDManager::existsquantify(const FOBDDVariable* var, const FOBDD* bdd) {
	BumpIndices b(this, var, 0);
	const FOBDD* bumped = b.FOBDDVisitor::change(bdd);
	const FOBDD* q = quantify(var->variable()->sort(), bumped);
	return q;
}

const FOBDD* FOBDDManager::existsquantify(const set<const FOBDDVariable*>& qvars, const FOBDD* bdd) {
	const FOBDD* result = bdd;
	for (auto it = qvars.cbegin(); it != qvars.cend(); ++it) {
		result = existsquantify(*it, result);
	}
	return result;
}

const FOBDD* FOBDDManager::quantify(Sort* sort, const FOBDD* bdd) {
	// base case
	if (bdd == _truebdd || bdd == _falsebdd) {
		// FIXME take empty sorts into account!
		return bdd;
	}

	// Recursive case
	auto it = _quanttable.find(sort);
	if (it != _quanttable.cend()) {
		auto jt = it->second.find(bdd);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}
	if (bdd->kernel()->category() == STANDARDCATEGORY) {
		const FOBDD* newfalse = quantify(sort, bdd->falsebranch());
		const FOBDD* newtrue = quantify(sort, bdd->truebranch());
		const FOBDD* result = ifthenelse(bdd->kernel(), newtrue, newfalse);
		return result;
	} else {
		const FOBDDKernel* kernel = getQuantKernel(sort, bdd);
		return getBDD(kernel, _truebdd, _falsebdd);
	}
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const map<const FOBDDVariable*, const FOBDDVariable*>& mvv) {
	SubstituteTerms<FOBDDVariable, FOBDDVariable> s(this, mvv);
	return s.FOBDDVisitor::change(bdd);
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const map<const FOBDDVariable*, const FOBDDArgument*>& mvv) {
	SubstituteTerms<FOBDDVariable, FOBDDArgument> s(this, mvv);
	return s.FOBDDVisitor::change(bdd);
}

const FOBDDKernel* FOBDDManager::substitute(const FOBDDKernel* kernel, const FOBDDDomainTerm* term, const FOBDDVariable* variable) {
	map<const FOBDDDomainTerm*, const FOBDDVariable*> map;
	map.insert(pair<const FOBDDDomainTerm*, const FOBDDVariable*>(term, variable));
	SubstituteTerms<FOBDDDomainTerm, FOBDDVariable> s(this, map);
	return kernel->acceptchange(&s);
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable) {
	SubstituteIndex s(this, index, variable);
	return s.FOBDDVisitor::change(bdd);
}

int FOBDDManager::longestbranch(const FOBDDKernel* kernel) {
	if (typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		return 1;
	} else {
		Assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* qk = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		return longestbranch(qk->bdd()) + 1;
	}
}

int FOBDDManager::longestbranch(const FOBDD* bdd) {
	if (bdd == _truebdd || bdd == _falsebdd) {
		return 1;
	} else {
		int kernellength = longestbranch(bdd->kernel());
		int truelength = longestbranch(bdd->truebranch()) + kernellength;
		int falselength = longestbranch(bdd->falsebranch()) + kernellength;
		return (truelength > falselength ? truelength : falselength);
	}
}

template<typename BddNode>
bool isArithmetic(const BddNode* k, FOBDDManager* manager) {
	CheckIsArithmeticFormula ac(manager);
	return ac.isArithmeticFormula(k);
}

const FOBDD* FOBDDManager::simplify(const FOBDD* bdd) {
	UngraphFunctions far(this);
	bdd = far.FOBDDVisitor::change(bdd);
	TermsToLeft ttl(this);
	bdd = ttl.FOBDDVisitor::change(bdd);
	RewriteMinus rm(this);
	bdd = rm.FOBDDVisitor::change(bdd);
	ApplyDistributivity dsbtvt(this);
	bdd = dsbtvt.FOBDDVisitor::change(bdd);
	OrderTerms<Multiplication> mo(this);
	bdd = mo.FOBDDVisitor::change(bdd);
	AddMultSimplifier ms(this);
	bdd = ms.FOBDDVisitor::change(bdd);
	OrderTerms<Addition> ao(this);
	bdd = ao.FOBDDVisitor::change(bdd);
	bdd = ms.FOBDDVisitor::change(bdd);
	CombineConstsOfMults ta(this);
	bdd = ta.FOBDDVisitor::change(bdd);
	AddMultSimplifier neut(this);
	bdd = neut.FOBDDVisitor::change(bdd);
	return bdd;
}

bool FOBDDManager::contains(const FOBDDArgument* super, const FOBDDArgument* arg) {
	ContainsTerm ac(this);
	return ac.contains(super, arg);
}

const FOBDDArgument* FOBDDManager::solve(const FOBDDKernel* kernel, const FOBDDArgument* argument) {
	if (typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atom = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		if (atom->symbol()->name() == "=/2") {
			if (atom->args(0) == argument) {
				if (not contains(atom->args(1), argument)) {
					return atom->args(1);
				}
			}
			if (atom->args(1) == argument) {
				if (not contains(atom->args(0), argument)) {
					return atom->args(0);
				}
			}
			if (SortUtils::isSubsort(atom->symbol()->sorts()[0], VocabularyUtils::floatsort())) {
				CollectSameOperationTerms<Addition> fa(this);
				vector<const FOBDDArgument*> terms = fa.getTerms(atom->args(0));
				unsigned int occcounter = 0;
				unsigned int occterm;
				for (size_t n = 0; n < terms.size(); ++n) {
					if (contains(terms[n], argument)) {
						++occcounter;
						occterm = n;
					}
				}
				if (occcounter == 1) {
					CollectSameOperationTerms<Multiplication> fm(this);
					vector<const FOBDDArgument*> factors = fm.getTerms(terms[occterm]);
					if (factors.size() == 2 && factors[1] == argument) {
						const FOBDDArgument* currterm = 0;
						for (size_t n = 0; n < terms.size(); ++n) {
							if (n != occterm) {
								if (not currterm) {
									currterm = terms[n];
								} else {
									Function* plus = Vocabulary::std()->func("+/2");
									plus = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(currterm->sort(), terms[n]->sort())), 0);
									vector<const FOBDDArgument*> newargs(2);
									newargs[0] = currterm;
									newargs[1] = terms[n];
									currterm = getFuncTerm(plus, newargs);
								}
							}
						}
						if (not currterm) {
							return atom->args(1);
						} else {
							const FOBDDDomainTerm* constant = dynamic_cast<const FOBDDDomainTerm*>(factors[0]);
							if (constant->value()->type() == DET_INT) {
								int constval = constant->value()->value()._int;
								if (constval == -1) {
									return currterm;
								} else if (constval == 1) {
									return invert(currterm);
								}
							}
							if (SortUtils::isSubsort(currterm->sort(), VocabularyUtils::intsort())) {
								// TODO: try if constval divides all constant factors
							} else {
								Function* times = Vocabulary::std()->func("*/2");
								times = times->disambiguate(vector<Sort*>(3, VocabularyUtils::floatsort()), 0);
								vector<const FOBDDArgument*> timesargs(2);
								timesargs[0] = currterm;
								double d =
										constant->value()->type() == DET_INT ?
												(double(1) / double(constant->value()->value()._int)) :
												(double(1) / constant->value()->value()._double);
								timesargs[1] = getDomainTerm(VocabularyUtils::floatsort(), createDomElem(d));
								d = -d;
								return getFuncTerm(times, timesargs);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

ostream& FOBDDManager::put(ostream& output, const FOBDD* bdd) const {
	if (bdd == _truebdd) {
		output << tabs();
		output << "true\n";
	} else if (bdd == _falsebdd) {
		output << tabs();
		output << "false\n";
	} else {
		put(output, bdd->kernel());

		pushtab();
		output << tabs();
		output << "FALSE BRANCH:\n";
		pushtab();
		output << tabs();
		put(output, bdd->falsebranch());
		poptab();
		output << tabs();
		output << "TRUE BRANCH:\n";
		pushtab();
		output << tabs();
		put(output, bdd->truebranch());
		poptab();
		poptab();
	}
	return output;
}

ostream& FOBDDManager::put(ostream& output, const FOBDDKernel* kernel) const {
	if (typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		output << tabs();
		output << *symbol;
		if (atomkernel->type() == AtomKernelType::AKT_CF) {
			output << "<cf>";
		} else if (atomkernel->type() == AtomKernelType::AKT_CT) {
			output << "<ct>";
		}
		if (typeid(*symbol) == typeid(Predicate)) {
			Assert(atomkernel->args().size()==symbol->nrSorts());
			if (symbol->nrSorts() > 0) {
				output << "(";
				put(output, atomkernel->args(0));
				for (size_t n = 1; n < symbol->nrSorts(); ++n) {
					output << ",";
					put(output, atomkernel->args(n));
				}
				output << ")";
			}
		} else {
			if (symbol->nrSorts() > 1) {
				output << "(";
				put(output, atomkernel->args(0));
				for (size_t n = 1; n < symbol->nrSorts() - 1; ++n) {
					output << ",";
					put(output, atomkernel->args(n));
				}
				output << ")";
			}
			output << " = ";
			put(output, atomkernel->args(symbol->nrSorts() - 1));
		}
	} else if (typeid(*kernel) == typeid(FOBDDQuantKernel)) {
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		output << tabs();
		output << "EXISTS(" << toString(quantkernel->sort()) << ") {\n";
		pushtab();
		put(output, quantkernel->bdd());
		poptab();
		output << tabs();
		output << "}";
	} else {
		throw notyetimplemented("Cannot print kerneltype, missing case in switch.");
	}
	output << '\n';
	return output;
}

ostream& FOBDDManager::put(ostream& output, const FOBDDArgument* arg) const {
	if (typeid(*arg) == typeid(FOBDDVariable)) {
		const FOBDDVariable* var = dynamic_cast<const FOBDDVariable*>(arg);
		var->variable()->put(output);
	} else if (typeid(*arg) == typeid(FOBDDDeBruijnIndex)) {
		const FOBDDDeBruijnIndex* dbr = dynamic_cast<const FOBDDDeBruijnIndex*>(arg);
		output << "<" << dbr->index() << ">[" << toString(dbr->sort()) << "]";
	} else if (typeid(*arg) == typeid(FOBDDFuncTerm)) {
		const FOBDDFuncTerm* ft = dynamic_cast<const FOBDDFuncTerm*>(arg);
		Function* f = ft->func();
		output << *f;
		if (f->arity()) {
			output << "(";
			put(output, ft->args(0));
			for (size_t n = 1; n < f->arity(); ++n) {
				output << ",";
				put(output, ft->args(n));
			}
			output << ")";
		}
	} else if (typeid(*arg) == typeid(FOBDDDomainTerm)) {
		const FOBDDDomainTerm* dt = dynamic_cast<const FOBDDDomainTerm*>(arg);
		output << *(dt->value()) << "[" << toString(dt->sort()) << "]";
	} else {
		throw notyetimplemented("Cannot print bddterm, missing case in switch.");
	}
	return output;
}

Formula* FOBDDManager::toFormula(const FOBDD* bdd) {
	BDDToFormula btf(this);
	return btf.createFormula(bdd);
}

Formula* FOBDDManager::toFormula(const FOBDDKernel* kernel) {
	BDDToFormula btf(this);
	return btf.createFormula(kernel);
}

Term* FOBDDManager::toTerm(const FOBDDArgument* arg) {
	BDDToFormula btf(this);
	return btf.createTerm(arg);
}

bool FOBDDManager::containsFuncTerms(const FOBDDKernel* kernel) {
	ContainsFuncTerms ft(this);
	return ft.check(kernel);
}

bool FOBDDManager::containsFuncTerms(const FOBDD* bdd) {
	ContainsFuncTerms ft(this);
	return ft.check(bdd);
}

bool FOBDDManager::partial(const FOBDDArgument* arg) {
	ContainsPartialFunctions bpc(this);
	return bpc.check(arg);
}

/**
 * Returns true iff the bdd contains the variable
 */
bool FOBDDManager::contains(const FOBDD* bdd, const FOBDDVariable* v) {
	if (bdd == _truebdd || bdd == _falsebdd)
		return false;
	else {
		if (contains(bdd->kernel(), v))
			return true;
		else if (contains(bdd->truebranch(), v))
			return true;
		else if (contains(bdd->falsebranch(), v))
			return true;
		else
			return false;
	}
}

/**
 * Returns true iff the kernel contains the variable
 */
bool FOBDDManager::contains(const FOBDDKernel* kernel, const FOBDDVariable* v) {
	if (typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		for (unsigned int n = 0; n < atomkernel->symbol()->sorts().size(); ++n) {
			if (contains(atomkernel->args(n), v))
				return true;
		}
		return false;
	} else {
		Assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		return contains(quantkernel->bdd(), v);
	}
}

/**
 * Returns true iff the argument contains the variable
 */
bool FOBDDManager::contains(const FOBDDArgument* arg, const FOBDDVariable* v) {
	if (typeid(*arg) == typeid(FOBDDVariable))
		return arg == v;
	else if (typeid(*arg) == typeid(FOBDDFuncTerm)) {
		const FOBDDFuncTerm* farg = dynamic_cast<const FOBDDFuncTerm*>(arg);
		for (unsigned int n = 0; n < farg->func()->arity(); ++n) {
			if (contains(farg->args(n), v))
				return true;
		}
		return false;
	} else
		return false;
}

/**
 * Returns true iff the kernel contains the variable
 */
bool FOBDDManager::contains(const FOBDDKernel* kernel, Variable* v) {
	const FOBDDVariable* var = getVariable(v);
	return contains(kernel, var);
}

/**
 * Returns the product of the sizes of the interpretations of the sorts of the given variables and indices in the given structure
 */
tablesize univNrAnswers(const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	vector<SortTable*> vst;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it)
		vst.push_back(structure->inter((*it)->variable()->sort()));
	for (auto it = indices.cbegin(); it != indices.cend(); ++it)
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
vector<vector<pair<bool, const FOBDDKernel*> > > FOBDDManager::pathsToFalse(const FOBDD* bdd) {
	vector<vector<pair<bool, const FOBDDKernel*> > > result;
	if (bdd == _falsebdd) {
		result.push_back(vector<pair<bool, const FOBDDKernel*> >(0));
	} else if (bdd != _truebdd) {
		auto falsepaths = pathsToFalse(bdd->falsebranch());
		auto truepaths = pathsToFalse(bdd->truebranch());
		for (auto it = falsepaths.cbegin(); it != falsepaths.cend(); ++it) {
			result.push_back( { pair<bool, const FOBDDKernel*>(false, bdd->kernel()) });
			for (auto jt = it->begin(); jt != it->end(); ++jt) {
				result.back().push_back(*jt);
			}
		}
		for (auto it = truepaths.cbegin(); it != truepaths.cend(); ++it) {
			result.push_back( { pair<bool, const FOBDDKernel*>(true, bdd->kernel()) });
			for (auto jt = it->begin(); jt != it->end(); ++jt) {
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
	if (bdd != _truebdd && bdd != _falsebdd) {
		auto falsekernels = allkernels(bdd->falsebranch());
		auto truekernels = allkernels(bdd->truebranch());
		result.insert(falsekernels.cbegin(), falsekernels.cend());
		result.insert(truekernels.cbegin(), truekernels.cend());
		result.insert(bdd->kernel());
		if (typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel)) {
			auto kernelkernels = allkernels(dynamic_cast<const FOBDDQuantKernel*>(bdd->kernel())->bdd());
			result.insert(kernelkernels.cbegin(), kernelkernels.cend());
		}
	}
	return result;
}

/**
 * Return all kernels of the given bdd that occur outside the scope of quantifiers
 */
set<const FOBDDKernel*> FOBDDManager::nonnestedkernels(const FOBDD* bdd) {
	set<const FOBDDKernel*> result;
	if (bdd != _truebdd && bdd != _falsebdd) {
		set<const FOBDDKernel*> falsekernels = nonnestedkernels(bdd->falsebranch());
		set<const FOBDDKernel*> truekernels = nonnestedkernels(bdd->truebranch());
		result.insert(falsekernels.cbegin(), falsekernels.cend());
		result.insert(truekernels.cbegin(), truekernels.cend());
		result.insert(bdd->kernel());
	}
	return result;
}

/**
 * Return a mapping from the non-nested kernels of the given bdd to their estimated number of answers
 */
map<const FOBDDKernel*, double> FOBDDManager::kernelAnswers(const FOBDD* bdd, AbstractStructure* structure) {
	map<const FOBDDKernel*, double> result;
	set<const FOBDDKernel*> kernels = nonnestedkernels(bdd);
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		set<const FOBDDVariable*> vars = variables(*it);
		set<const FOBDDDeBruijnIndex*> indices = FOBDDManager::indices(*it);
		result[*it] = estimatedNrAnswers(*it, vars, indices, structure);
	}
	return result;
}

/**
 * Returns all variables that occur in the given bdd
 */
set<const FOBDDVariable*> FOBDDManager::variables(const FOBDD* bdd) {
	VariableCollector vc(this);
	return vc.getVariables(bdd);
}

/**
 * Returns all variables that occur in the given kernel
 */
set<const FOBDDVariable*> FOBDDManager::variables(const FOBDDKernel* kernel) {
	VariableCollector vc(this);
	return vc.getVariables(kernel);
}

/**
 * Returns all De Bruijn indices that occur in the given bdd.
 */
set<const FOBDDDeBruijnIndex*> FOBDDManager::indices(const FOBDD* bdd) {
	IndexCollector dbc(this);
	return dbc.getVariables(bdd);
}

/**
 * Returns all variables that occur in the given kernel
 */
set<const FOBDDDeBruijnIndex*> FOBDDManager::indices(const FOBDDKernel* kernel) {
	IndexCollector dbc(this);
	return dbc.getVariables(kernel);
}

map<const FOBDDKernel*, tablesize> FOBDDManager::kernelUnivs(const FOBDD* bdd, AbstractStructure* structure) {
	map<const FOBDDKernel*, tablesize> result;
	set<const FOBDDKernel*> kernels = nonnestedkernels(bdd);
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		set<const FOBDDVariable*> vars = variables(*it);
		set<const FOBDDDeBruijnIndex*> indices = FOBDDManager::indices(*it);
		result[*it] = univNrAnswers(vars, indices, structure);
	}
	return result;
}

double FOBDDManager::estimatedChance(const FOBDDKernel* kernel, AbstractStructure* structure) {
	if (typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		double chance = 0;
		PFSymbol* symbol = atomkernel->symbol();
		PredInter* pinter;
		if (typeid(*symbol) == typeid(Predicate)) {
			pinter = structure->inter(dynamic_cast<Predicate*>(symbol));
		} else {
			pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		}
		const PredTable* pt = atomkernel->type() == AtomKernelType::AKT_CF ? pinter->cf() : pinter->ct();
		tablesize symbolsize = pt->size();
		double univsize = 1;
		for (auto it = atomkernel->args().cbegin(); it != atomkernel->args().cend(); ++it) {
			tablesize argsize = structure->inter((*it)->sort())->size();
			if (argsize._type == TST_APPROXIMATED || argsize._type == TST_EXACT) {
				univsize = univsize * argsize._size;
			} else {
				univsize = numeric_limits<double>::max();
				break;
			}
		}
		if (symbolsize._type == TST_APPROXIMATED || symbolsize._type == TST_EXACT) {
			if (univsize < numeric_limits<double>::max()) {
				chance = double(symbolsize._size) / univsize;
				if (chance > 1) {
					chance = 1;
				}
			} else {
				chance = 0;
			}
		} else {
			// TODO better estimators possible?
			if (univsize < numeric_limits<double>::max())
				chance = 0.5;
			else
				chance = 0;
		}
		return chance;
	} else { // case of a quantification kernel
		Assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);

		// get the table of the sort of the quantified variable
		SortTable* quantsorttable = structure->inter(quantkernel->sort());
		tablesize quanttablesize = quantsorttable->size();

		// some simple checks
		int quantsize = 0;
		if (quanttablesize._type == TST_APPROXIMATED || quanttablesize._type == TST_EXACT) {
			if (quanttablesize._size == 0)
				return 0; // if the sort is empty, the kernel cannot be true
			else
				quantsize = quanttablesize._size;
		} else {
			if (!quantsorttable->approxFinite()) {
				// if the sort is infinite, the kernel is true if the chance of the bdd is nonzero.
				double bddchance = estimatedChance(quantkernel->bdd(), structure);
				return bddchance == 0 ? 0 : 1;
			} else {
				// FIXME implement correctly
				return 0.5;
			}
		}

		// collect the paths that lead to node 'false'
		vector<vector<pair<bool, const FOBDDKernel*> > > paths = pathsToFalse(quantkernel->bdd());

		// collect all kernels and their estimated number of answers
		map<const FOBDDKernel*, double> subkernels = kernelAnswers(quantkernel->bdd(), structure);
		map<const FOBDDKernel*, tablesize> subunivs = kernelUnivs(quantkernel->bdd(), structure);

		srand(global_seed);
		double sum = 0; // stores the sum of the chances obtained by each experiment
		int sumcount = 0; // stores the number of succesfull experiments
		for (unsigned int experiment = 0; experiment < 10; ++experiment) { // do 10 experiments
			// An experiment consists of trying to reach N times node 'false',
			// where N is the size of the domain of the quantified variable.

			map<const FOBDDKernel*, double> dynsubkernels = subkernels;
			bool fail = false;

			double chance = 1;
			for (int element = 0; element < quantsize; ++element) {
				// Compute possibility of each path
				vector<double> cumulative_pathsposs;
				double cumulative_chance = 0;
				for (unsigned int pathnr = 0; pathnr < paths.size(); ++pathnr) {
					double currchance = 1;
					for (unsigned int nodenr = 0; nodenr < paths[pathnr].size(); ++nodenr) {
						tablesize ts = subunivs[paths[pathnr][nodenr].second];
						double nodeunivsize = (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) ? ts._size : numeric_limits<double>::max();
						if (paths[pathnr][nodenr].first)
							currchance = currchance * dynsubkernels[paths[pathnr][nodenr].second] / double(nodeunivsize - element);
						else
							currchance = currchance * (nodeunivsize - element - dynsubkernels[paths[pathnr][nodenr].second])
									/ double(nodeunivsize - element);
					}
					cumulative_chance += currchance;
					cumulative_pathsposs.push_back(cumulative_chance);
				}

				// TODO there is a bug in the probability code, leading to P > 1, such that the following check is necessary
				if (cumulative_chance > 1) {
					//Warning::cumulchance(cumulative_chance);
					cumulative_chance = 1;
				}
				if (cumulative_chance > 0) { // there is a possible path to false
					chance = chance * cumulative_chance;

					// randomly choose a path
					double toss = double(rand()) / double(RAND_MAX) * cumulative_chance;
					unsigned int chosenpathnr = lower_bound(cumulative_pathsposs.cbegin(), cumulative_pathsposs.cend(), toss)
							- cumulative_pathsposs.cbegin();
					for (unsigned int nodenr = 0; nodenr < paths[chosenpathnr].size(); ++nodenr) {
						if (paths[chosenpathnr][nodenr].first)
							dynsubkernels[paths[chosenpathnr][nodenr].second] += -(1.0);
					}
				} else { // the experiment failed
					fail = true;
					break;
				}
			}

			if (!fail) {
				sum += chance;
				++sumcount;
			}
		}

		if (sum == 0) { // no experiment succeeded
			return 1;
		} else { // at least one experiment succeeded: take average of all succesfull experiments
			return 1 - (sum / double(sumcount));
		}
	}
}

double FOBDDManager::estimatedChance(const FOBDD* bdd, AbstractStructure* structure) {
	if (bdd == _falsebdd)
		return 0;
	else if (bdd == _truebdd)
		return 1;
	else {
		double kernchance = estimatedChance(bdd->kernel(), structure);
		double falsechance = estimatedChance(bdd->falsebranch(), structure);
		double truechance = estimatedChance(bdd->truebranch(), structure);
		return (kernchance * truechance) + ((1 - kernchance) * falsechance);
	}
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | kernel } in the given structure
 */
double FOBDDManager::estimatedNrAnswers(const FOBDDKernel* kernel, const set<const FOBDDVariable*>& vars,
		const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	// TODO: improve this if functional dependency is known
	double maxdouble = numeric_limits<double>::max();
	double kernelchance = estimatedChance(kernel, structure);
	tablesize univanswers = univNrAnswers(vars, indices, structure);
	if (univanswers._type == TST_INFINITE || univanswers._type == TST_UNKNOWN) {
		return (kernelchance > 0 ? maxdouble : 0);
	} else
		return kernelchance * univanswers._size;
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | bdd } in the given structure
 */
double FOBDDManager::estimatedNrAnswers(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		AbstractStructure* structure) {
	double maxdouble = numeric_limits<double>::max();
	double bddchance = estimatedChance(bdd, structure);
	tablesize univanswers = univNrAnswers(vars, indices, structure);
	if (univanswers._type == TST_INFINITE || univanswers._type == TST_UNKNOWN) {
		return (bddchance > 0 ? maxdouble : 0);
	} else
		return bddchance * univanswers._size;
}

double FOBDDManager::estimatedCostAll(bool sign, const FOBDDKernel* kernel, const set<const FOBDDVariable*>& vars,
		const set<const FOBDDDeBruijnIndex*>& indices, AbstractStructure* structure) {
	double maxdouble = numeric_limits<double>::max();
	if (isArithmetic(kernel, this)) {
		vector<double> varunivsizes;
		vector<double> indexunivsizes;
		vector<const FOBDDVariable*> varsvector;
		vector<const FOBDDDeBruijnIndex*> indicesvector;
		unsigned int nrinfinite = 0;
		const FOBDDVariable* infinitevar = 0;
		const FOBDDDeBruijnIndex* infiniteindex = 0;
		for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
			varsvector.push_back(*it);
			SortTable* st = structure->inter((*it)->sort());
			tablesize stsize = st->size();
			if (stsize._type == TST_EXACT || stsize._type == TST_APPROXIMATED)
				varunivsizes.push_back(double(stsize._size));
			else {
				varunivsizes.push_back(maxdouble);
				++nrinfinite;
				if (!infinitevar)
					infinitevar = *it;
			}
		}
		for (auto it = indices.cbegin(); it != indices.cend(); ++it) {
			indicesvector.push_back(*it);
			SortTable* st = structure->inter((*it)->sort());
			tablesize stsize = st->size();
			if (stsize._type == TST_EXACT || stsize._type == TST_APPROXIMATED)
				indexunivsizes.push_back(double(stsize._size));
			else {
				indexunivsizes.push_back(maxdouble);
				++nrinfinite;
				if (!infiniteindex)
					infiniteindex = *it;
			}
		}
		if (nrinfinite > 1) {
			return maxdouble;
		} else if (nrinfinite == 1) {
			if (infinitevar) {
				if (!solve(kernel, infinitevar))
					return maxdouble;
			} else {
				Assert(infiniteindex);
				if (!solve(kernel, infiniteindex))
					return maxdouble;
			}
			double result = 1;
			for (unsigned int n = 0; n < varsvector.size(); ++n) {
				if (varsvector[n] != infinitevar) {
					result = (result * varunivsizes[n] < maxdouble) ? (result * varunivsizes[n]) : maxdouble;
				}
			}
			for (unsigned int n = 0; n < indicesvector.size(); ++n) {
				if (indicesvector[n] != infiniteindex) {
					result = (result * indexunivsizes[n] < maxdouble) ? (result * indexunivsizes[n]) : maxdouble;
				}
			}
			return result;
		} else {
			double maxresult = 1;
			for (auto it = varunivsizes.cbegin(); it != varunivsizes.cend(); ++it) {
				maxresult = (maxresult * (*it) < maxdouble) ? (maxresult * (*it)) : maxdouble;
			}
			for (auto it = indexunivsizes.cbegin(); it != indexunivsizes.cend(); ++it) {
				maxresult = (maxresult * (*it) < maxdouble) ? (maxresult * (*it)) : maxdouble;
			}
			if (maxresult < maxdouble) {
				double bestresult = maxresult;
				for (unsigned int n = 0; n < varsvector.size(); ++n) {
					if (solve(kernel, varsvector[n])) {
						double currresult = maxresult / varunivsizes[n];
						if (currresult < bestresult) {
							bestresult = currresult;
						}
					}
				}
				for (unsigned int n = 0; n < indicesvector.size(); ++n) {
					if (solve(kernel, indicesvector[n])) {
						double currresult = maxresult / indexunivsizes[n];
						if (currresult < bestresult) {
							bestresult = currresult;
						}
					}
				}
				return bestresult;
			} else {
				return maxdouble;
			}
		}
	} else if (typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		PFSymbol* symbol = atomkernel->symbol();
		PredInter* pinter;
		if (typeid(*symbol) == typeid(Predicate)) {
			pinter = structure->inter(dynamic_cast<Predicate*>(symbol));
		} else {
			pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		}
		const PredTable* pt;
		if (sign) {
			if (atomkernel->type() == AtomKernelType::AKT_CF) {
				pt = pinter->cf();
			} else {
				pt = pinter->ct();
			}
		} else {
			if (atomkernel->type() == AtomKernelType::AKT_CF) {
				pt = pinter->pt();
			} else {
				pt = pinter->pf();
			}
		}

		vector<bool> pattern;
		for (auto it = atomkernel->args().cbegin(); it != atomkernel->args().cend(); ++it) {
			bool input = true;
			for (auto jt = vars.cbegin(); jt != vars.cend(); ++jt) {
				if (contains(*it, *jt)) {
					input = false;
					break;
				}
			}
			if (input) {
				for (auto jt = indices.cbegin(); jt != indices.cend(); ++jt) {
					if ((*it)->containsDeBruijnIndex((*jt)->index())) {
						input = false;
						break;
					}
				}
			}
			pattern.push_back(input);
		}

		EstimateBDDInferenceCost tce;
		double result = tce.run(pt, pattern);
		return result;
	} else {
		// NOTE: implement a better estimator if backjumping on bdds is implemented
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		set<const FOBDDDeBruijnIndex*> newindices;
		for (auto it = indices.cbegin(); it != indices.cend(); ++it) {
			newindices.insert(getDeBruijnIndex((*it)->sort(), (*it)->index() + 1));
		}
		newindices.insert(getDeBruijnIndex(quantkernel->sort(), 0));
		double result = estimatedCostAll(quantkernel->bdd(), vars, newindices, structure);
		return result;
	}
}

double FOBDDManager::estimatedCostAll(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		AbstractStructure* structure) {
	double maxdouble = numeric_limits<double>::max();
	if (bdd == _truebdd) {
		tablesize univsize = univNrAnswers(vars, indices, structure);
		if (univsize._type == TST_INFINITE || univsize._type == TST_UNKNOWN)
			return maxdouble;
		else
			return double(univsize._size);
	} else if (bdd == _falsebdd) {
		return 1;
	} else {
		// split variables
		set<const FOBDDVariable*> kernelvars = variables(bdd->kernel());
		set<const FOBDDDeBruijnIndex*> kernelindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*> bddvars;
		set<const FOBDDDeBruijnIndex*> bddindices;
		for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
			if (kernelvars.find(*it) == kernelvars.cend())
				bddvars.insert(*it);
		}
		for (auto it = indices.cbegin(); it != indices.cend(); ++it) {
			if (kernelindices.find(*it) == kernelindices.cend())
				bddindices.insert(*it);
		}
		set<const FOBDDVariable*> removevars;
		set<const FOBDDDeBruijnIndex*> removeindices;
		for (auto it = kernelvars.cbegin(); it != kernelvars.cend(); ++it) {
			if (vars.find(*it) == vars.cend())
				removevars.insert(*it);
		}
		for (auto it = kernelindices.cbegin(); it != kernelindices.cend(); ++it) {
			if (indices.find(*it) == indices.cend())
				removeindices.insert(*it);
		}
		for (auto it = removevars.cbegin(); it != removevars.cend(); ++it) {
			kernelvars.erase(*it);
		}
		for (auto it = removeindices.cbegin(); it != removeindices.cend(); ++it) {
			kernelindices.erase(*it);
		}

		// recursive case
		if (bdd->falsebranch() == _falsebdd) {
			double kernelcost = estimatedCostAll(true, bdd->kernel(), kernelvars, kernelindices, structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure);
			double truecost = estimatedCostAll(bdd->truebranch(), bddvars, bddindices, structure);
			if (kernelcost < maxdouble && kernelans < maxdouble && truecost < maxdouble && kernelcost + (kernelans * truecost) < maxdouble) {
				return kernelcost + (kernelans * truecost);
			} else
				return maxdouble;
		} else if (bdd->truebranch() == _falsebdd) {
			double kernelcost = estimatedCostAll(false, bdd->kernel(), kernelvars, kernelindices, structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure);
			tablesize kernelunivsize = univNrAnswers(kernelvars, kernelindices, structure);
			double invkernans =
					(kernelunivsize._type == TST_INFINITE || kernelunivsize._type == TST_UNKNOWN) ?
							maxdouble : double(kernelunivsize._size) - kernelans;
			double falsecost = estimatedCostAll(bdd->falsebranch(), bddvars, bddindices, structure);
			if (kernelcost + (invkernans * falsecost) < maxdouble) {
				return kernelcost + (invkernans * falsecost);
			} else
				return maxdouble;
		} else {
			tablesize kernelunivsize = univNrAnswers(kernelvars, kernelindices, structure);
			set<const FOBDDVariable*> emptyvars;
			set<const FOBDDDeBruijnIndex*> emptyindices;
			double kernelcost = estimatedCostAll(true, bdd->kernel(), emptyvars, emptyindices, structure);
			double truecost = estimatedCostAll(bdd->truebranch(), bddvars, bddindices, structure);
			double falsecost = estimatedCostAll(bdd->falsebranch(), bddvars, bddindices, structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure);
			if (kernelunivsize._type == TST_UNKNOWN || kernelunivsize._type == TST_INFINITE)
				return maxdouble;
			else {
				if ((double(kernelunivsize._size) * kernelcost) + (double(kernelans) * truecost) + ((kernelunivsize._size - kernelans) * falsecost)
						< maxdouble) {
					return (double(kernelunivsize._size) * kernelcost) + (double(kernelans) * truecost)
							+ ((kernelunivsize._size - kernelans) * falsecost);
				} else
					return maxdouble;
			}
		}
	}
}

void FOBDDManager::optimizequery(const FOBDD* query, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		AbstractStructure* structure) {
	if (query != _truebdd && query != _falsebdd) {
		set<const FOBDDKernel*> kernels = allkernels(query);
		for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
			double bestscore = estimatedCostAll(query, vars, indices, structure);
			int bestposition = 0;
			// move upward
			while ((*it)->number() != 0) {
				moveUp(*it);
				double currscore = estimatedCostAll(query, vars, indices, structure);
				if (currscore < bestscore) {
					bestscore = currscore;
					bestposition = 0;
				} else
					bestposition += 1;
			}
			// move downward
			while ((*it)->number() < _kernels[(*it)->category()].size() - 1) {
				moveDown(*it);
				double currscore = estimatedCostAll(query, vars, indices, structure);
				if (currscore < bestscore) {
					bestscore = currscore;
					bestposition = 0;
				} else
					bestposition += -1;
			}
			// move to best position
			if (bestposition < 0) {
				for (int n = 0; n > bestposition; --n)
					moveUp(*it);
			} else if (bestposition > 0) {
				for (int n = 0; n < bestposition; ++n)
					moveDown(*it);
			}
		}
	}
}

const FOBDD* FOBDDManager::make_more_false(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		AbstractStructure* structure, double weight_per_ans) {

	if (isTruebdd(bdd) || isFalsebdd(bdd)) {
		return bdd;
	} else {
		// Split variables
		set<const FOBDDVariable*> kvars = variables(bdd->kernel());
		set<const FOBDDDeBruijnIndex*> kindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*> kernelvars;
		set<const FOBDDVariable*> branchvars;
		set<const FOBDDDeBruijnIndex*> kernelindices;
		set<const FOBDDDeBruijnIndex*> branchindices;
		for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
			if (kvars.find(*it) == kvars.cend())
				branchvars.insert(*it);
			else
				kernelvars.insert(*it);
		}
		for (auto it = indices.cbegin(); it != indices.cend(); ++it) {
			if (kindices.find(*it) == kindices.cend())
				branchindices.insert(*it);
			else
				kernelindices.insert(*it);
		}

		// Recursive call
		double bddcost = estimatedCostAll(bdd, vars, indices, structure);
		double bddans = estimatedNrAnswers(bdd, vars, indices, structure);
		double totalbddcost = numeric_limits<double>::max();
		if (bddcost + (bddans * weight_per_ans) < totalbddcost) {
			totalbddcost = bddcost + (bddans * weight_per_ans);
		}

		if (isTruebdd(bdd->falsebranch())) {
			double branchcost = estimatedCostAll(bdd->truebranch(), vars, indices, structure);
			double branchans = estimatedNrAnswers(bdd->truebranch(), vars, indices, structure);
			double totalbranchcost = numeric_limits<double>::max();
			if (branchcost + (branchans * weight_per_ans) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weight_per_ans);
			}
			if (totalbranchcost < totalbddcost) {
				return make_more_false(bdd->truebranch(), vars, indices, structure, weight_per_ans);
			}
		} else if (isTruebdd(bdd->truebranch())) {
			double branchcost = estimatedCostAll(bdd->falsebranch(), vars, indices, structure);
			double branchans = estimatedNrAnswers(bdd->falsebranch(), vars, indices, structure);
			double totalbranchcost = numeric_limits<double>::max();
			if (branchcost + (branchans * weight_per_ans) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weight_per_ans);
			}
			if (totalbranchcost < totalbddcost) {
				return make_more_false(bdd->falsebranch(), vars, indices, structure, weight_per_ans);
			}
		}

		double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure);
		double truebranchweight =
				(kernelans * weight_per_ans < numeric_limits<double>::max()) ? kernelans * weight_per_ans : numeric_limits<double>::max();
		const FOBDD* newtrue = make_more_false(bdd->truebranch(), branchvars, branchindices, structure, truebranchweight);

		tablesize allkernelans = univNrAnswers(kernelvars, kernelindices, structure);
		double chance = estimatedChance(bdd->kernel(), structure);
		double invkernelans;
		if (allkernelans._type == TST_APPROXIMATED || allkernelans._type == TST_EXACT) {
			invkernelans = allkernelans._size * (1 - chance);
		} else {
			if (chance > 0)
				invkernelans = numeric_limits<double>::max();
			else
				kernelans = 1;
		}
		double falsebranchweight =
				(invkernelans * weight_per_ans < numeric_limits<double>::max()) ? invkernelans * weight_per_ans : numeric_limits<double>::max();
		const FOBDD* newfalse = make_more_false(bdd->falsebranch(), branchvars, branchindices, structure, falsebranchweight);
		if (newtrue != bdd->truebranch() || newfalse != bdd->falsebranch()) {
			return make_more_false(getBDD(bdd->kernel(), newtrue, newfalse), vars, indices, structure, weight_per_ans);
		} else
			return bdd;
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
	 for(auto it = vars.cbegin(); it != vars.cend(); ++it) {
	 if(kvars.find(*it) == kvars.cend()) branchvars.insert(*it);
	 else kernelvars.insert(*it);
	 }
	 for(auto it = indices.cbegin(); it != indices.cend(); ++it) {
	 if(kindices.find(*it) == kindices.cend()) branchindices.insert(*it);
	 else kernelindices.insert(*it);
	 }

	 // Simplify quantification kernels
	 if(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel)) {
	 const FOBDD* newfalse = make_more_false(bdd->falsebranch(),branchvars,branchindices,structure,max_cost_per_ans);
	 const FOBDD* newtrue = make_more_false(bdd->truebranch(),branchvars,branchindices,structure,max_cost_per_ans);
	 bdd = getBDD(bdd->kernel(),newtrue,newfalse);
	 if(isFalsebdd(bdd)) return bdd;
	 else {
	 Assert(!isTruebdd(bdd));
	 Assert(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel));
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

const FOBDD* FOBDDManager::make_more_true(const FOBDD* bdd, const set<const FOBDDVariable*>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		AbstractStructure* structure, double weight_per_ans) {
	if (isTruebdd(bdd) || isFalsebdd(bdd)) {
		return bdd;
	} else {
		// Split variables
		set<const FOBDDVariable*> kvars = variables(bdd->kernel());
		set<const FOBDDDeBruijnIndex*> kindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*> kernelvars;
		set<const FOBDDVariable*> branchvars;
		set<const FOBDDDeBruijnIndex*> kernelindices;
		set<const FOBDDDeBruijnIndex*> branchindices;
		for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
			if (kvars.find(*it) == kvars.cend())
				branchvars.insert(*it);
			else
				kernelvars.insert(*it);
		}
		for (auto it = indices.cbegin(); it != indices.cend(); ++it) {
			if (kindices.find(*it) == kindices.cend())
				branchindices.insert(*it);
			else
				kernelindices.insert(*it);
		}

		// Recursive call
		double bddcost = estimatedCostAll(bdd, vars, indices, structure);
		double bddans = estimatedNrAnswers(bdd, vars, indices, structure);
		double totalbddcost = numeric_limits<double>::max();
		if (bddcost + (bddans * weight_per_ans) < totalbddcost) {
			totalbddcost = bddcost + (bddans * weight_per_ans);
		}

		if (isFalsebdd(bdd->falsebranch())) {
			double branchcost = estimatedCostAll(bdd->truebranch(), vars, indices, structure);
			double branchans = estimatedNrAnswers(bdd->truebranch(), vars, indices, structure);
			double totalbranchcost = numeric_limits<double>::max();
			if (branchcost + (branchans * weight_per_ans) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weight_per_ans);
			}
			if (totalbranchcost < totalbddcost) {
				return make_more_true(bdd->truebranch(), vars, indices, structure, weight_per_ans);
			}
		} else if (isFalsebdd(bdd->truebranch())) {
			double branchcost = estimatedCostAll(bdd->falsebranch(), vars, indices, structure);
			double branchans = estimatedNrAnswers(bdd->falsebranch(), vars, indices, structure);
			double totalbranchcost = numeric_limits<double>::max();
			if (branchcost + (branchans * weight_per_ans) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weight_per_ans);
			}
			if (totalbranchcost < totalbddcost) {
				return make_more_true(bdd->falsebranch(), vars, indices, structure, weight_per_ans);
			}
		}

		double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure);
		double truebranchweight =
				(kernelans * weight_per_ans < numeric_limits<double>::max()) ? kernelans * weight_per_ans : numeric_limits<double>::max();
		const FOBDD* newtrue = make_more_true(bdd->truebranch(), branchvars, branchindices, structure, truebranchweight);

		tablesize allkernelans = univNrAnswers(kernelvars, kernelindices, structure);
		double chance = estimatedChance(bdd->kernel(), structure);
		double invkernelans;
		if (allkernelans._type == TST_APPROXIMATED || allkernelans._type == TST_EXACT) {
			invkernelans = allkernelans._size * (1 - chance);
		} else {
			if (chance > 0)
				invkernelans = numeric_limits<double>::max();
			else {
				kernelans = 1;
				return bdd; // FIXME was no here orginally!
			}
		}
		double falsebranchweight =
				(invkernelans * weight_per_ans < numeric_limits<double>::max()) ? invkernelans * weight_per_ans : numeric_limits<double>::max();
		const FOBDD* newfalse = make_more_true(bdd->falsebranch(), branchvars, branchindices, structure, falsebranchweight);
		if (newtrue != bdd->truebranch() || newfalse != bdd->falsebranch()) {
			return make_more_true(getBDD(bdd->kernel(), newtrue, newfalse), vars, indices, structure, weight_per_ans);
		} else
			return bdd;

		// Simplify quantification kernels
		/*		if(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel)) {
		 const FOBDD* newfalse = make_more_true(bdd->falsebranch(),branchvars,branchindices,structure,max_cost_per_ans);
		 const FOBDD* newtrue = make_more_true(bdd->truebranch(),branchvars,branchindices,structure,max_cost_per_ans);
		 bdd = getBDD(bdd->kernel(),newtrue,newfalse);
		 if(isTruebdd(bdd)) return bdd;
		 else {
		 Assert(!isFalsebdd(bdd));
		 Assert(typeid(*(bdd->kernel())) == typeid(FOBDDQuantKernel));
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

FOBDDManager::FOBDDManager() {
	_nextorder[TRUEFALSECATEGORY] = 0;
	_nextorder[STANDARDCATEGORY] = 0;
	_nextorder[DEBRUIJNCATEGORY] = 0;

	KernelOrder ktrue = newOrder(TRUEFALSECATEGORY);
	KernelOrder kfalse = newOrder(TRUEFALSECATEGORY);
	_truekernel = new FOBDDKernel(ktrue);
	_falsekernel = new FOBDDKernel(kfalse);
	_truebdd = new FOBDD(_truekernel, 0, 0);
	_falsebdd = new FOBDD(_falsekernel, 0, 0);
}
