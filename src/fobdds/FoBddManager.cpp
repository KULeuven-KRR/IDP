/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "utils/NumericLimits.hpp"
#include "commontypes.hpp"
#include "FoBddManager.hpp"
#include "bddvisitors/OrderTerms.hpp"
#include "bddvisitors/SymbolCollector.hpp"
#include "bddvisitors/TermOccursNested.hpp"
#include "bddvisitors/ContainsPartialFunctions.hpp"
#include "bddvisitors/TermCollector.hpp"
#include "bddvisitors/IndexCollector.hpp"
#include "bddvisitors/VariableCollector.hpp"
#include "bddvisitors/AddMultSimplifier.hpp"
#include "bddvisitors/SubstituteTerms.hpp"
#include "bddvisitors/ContainsFuncTerms.hpp"
#include "bddvisitors/BumpIndices.hpp"
#include "bddvisitors/TermsToLeft.hpp"
#include "bddvisitors/Copy.hpp"
#include "bddvisitors/RemoveMinus.hpp"
#include "bddvisitors/UngraphFunctions.hpp"
#include "bddvisitors/CollectSameOperationTerms.hpp"
#include "bddvisitors/BddToFormula.hpp"
#include "bddvisitors/ApplyDistributivity.hpp"
#include "bddvisitors/ContainsTerm.hpp"
#include "bddvisitors/CombineConstsOfMults.hpp"

#include "FoBddAggKernel.hpp"
#include "FoBddAggTerm.hpp"
#include "Estimations.hpp"

using namespace std;

KernelOrder FOBDDManager::newOrder(KernelOrderCategory category) {
	KernelOrder order(category, _nextorder[category]);
	++_nextorder[category];
	return order;
}

KernelOrder FOBDDManager::newOrder(const vector<const FOBDDTerm*>& args) {
	auto category = KernelOrderCategory::STANDARDCATEGORY;
	for (size_t n = 0; n < args.size(); ++n) {
		if (args[n]->containsFreeDeBruijnIndex()) {
			category = KernelOrderCategory::DEBRUIJNCATEGORY;
			break;
		}
	}
	return newOrder(category);
}

KernelOrder FOBDDManager::newOrderForQuantifiedBDD(const FOBDD* bdd) {
	//Check for containment of dbrindex 1 since dbrindex 0 is quantified here!
	auto category = (bdd->containsDeBruijnIndex(1)) ? KernelOrderCategory::DEBRUIJNCATEGORY : KernelOrderCategory::STANDARDCATEGORY;
	return newOrder(category);
}

KernelOrder FOBDDManager::newOrder(const FOBDDAggTerm* aggterm) {
	auto category = (aggterm->containsDeBruijnIndex(0)) ? KernelOrderCategory::DEBRUIJNCATEGORY : KernelOrderCategory::STANDARDCATEGORY;
	return newOrder(category);
}

void FOBDDManager::clearDynamicTables() {
	_negationtable.clear();
	_conjunctiontable.clear();
	_disjunctiontable.clear();
	_ifthenelsetable.clear();
	_quanttable.clear();
}

FOBDDKernel* FOBDDManager::kernelAbove(const FOBDDKernel* kernel) {
	auto cat = kernel->category();
	if (cat == KernelOrderCategory::TRUEFALSECATEGORY) {
		return NULL;
	}
	Assert(_kernels.find(cat) != _kernels.cend());
	auto catkernels = _kernels[cat];

	unsigned int nr = kernel->number();
	auto kernelabove = catkernels.find(nr + 1);
	if (kernelabove != catkernels.cend()) {
		Assert(*(kernelabove->second) < *kernel);
		return kernelabove->second;
	}
	return NULL;

}
FOBDDKernel* FOBDDManager::kernelBelow(const FOBDDKernel* kernel) {
	auto cat = kernel->category();
	if (cat == KernelOrderCategory::TRUEFALSECATEGORY) {
		return NULL;
	}
	Assert(_kernels.find(cat) != _kernels.cend());
	auto catkernels = _kernels[cat];
	unsigned int nr = kernel->number();
	auto kernelbelow = catkernels.find(nr - 1);
	if (kernelbelow != catkernels.cend()) {
		Assert((*kernelbelow->second) > *kernel);
		return kernelbelow->second;
	}
	return NULL;
}

void FOBDDManager::moveUp(const FOBDDKernel* kernel) {
	clearDynamicTables();
	auto kernelabove = kernelAbove(kernel);
	if(kernelabove == NULL){
		return;
	}
	moveDown(kernelabove);
}

void FOBDDManager::moveDown(const FOBDDKernel* kernel) {
	clearDynamicTables();
	auto cat = kernel->category();
	if (cat != KernelOrderCategory::TRUEFALSECATEGORY) {
		unsigned int nr = kernel->number();
		vector<const FOBDD*> falseerase;
		vector<const FOBDD*> trueerase;
		const FOBDDKernel* nextkernel = kernelBelow(kernel);
		if(nextkernel == NULL){
			return;
		}
		auto nextKernelNumber = nextkernel->number();
		//bdds contains all bdds with kernel as kernel
		const MBDDMBDDBDD& bdds = _bddtable[kernel];
		for (auto it = bdds.cbegin(); it != bdds.cend(); ++it) {
			for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
				FOBDD* bdd = jt->second;
				auto falsebranch = it->first;
				auto truebranch = jt->first;
				//Swapfalse and swaptrue express whether swapping should happen in the false and in the true branch respectively.
				// If the nextkernel is not in those branches, no swapping should happen.
				bool swapfalse = (nextkernel == falsebranch->kernel());
				bool swaptrue = (nextkernel == truebranch->kernel());
				if (swapfalse || swaptrue) {
					falseerase.push_back(falsebranch);
					trueerase.push_back(truebranch);
				}
				if (swapfalse && swaptrue) {
					const FOBDD* newfalse = getBDD(kernel, truebranch->falsebranch(), falsebranch->falsebranch());
					const FOBDD* newtrue = getBDD(kernel, truebranch->truebranch(), falsebranch->truebranch());
					bdd->replacefalse(newfalse);
					bdd->replacetrue(newtrue);
					bdd->replacekernel(nextkernel);
					_bddtable[nextkernel][newfalse][newtrue] = bdd;
				} else if (swapfalse) {
					const FOBDD* newfalse = getBDD(kernel, truebranch, falsebranch->falsebranch());
					const FOBDD* newtrue = getBDD(kernel, truebranch, falsebranch->truebranch());
					bdd->replacefalse(newfalse);
					bdd->replacetrue(newtrue);
					bdd->replacekernel(nextkernel);
					_bddtable[nextkernel][newfalse][newtrue] = bdd;
				} else if (swaptrue) {
					const FOBDD* newfalse = getBDD(kernel, truebranch->falsebranch(), falsebranch);
					const FOBDD* newtrue = getBDD(kernel, truebranch->truebranch(), falsebranch);
					bdd->replacefalse(newfalse);
					bdd->replacetrue(newtrue);
					bdd->replacekernel(nextkernel);
					_bddtable[nextkernel][newfalse][newtrue] = bdd;
				}
			}

		}
		for (unsigned int n = 0; n < falseerase.size(); ++n) {
			//deleteAllMatching<FOBDD>(_bddtable[kernel][falseerase[n]], trueerase[n]); TODO: correct?
			_bddtable[kernel][falseerase[n]].erase(trueerase[n]);
			if (_bddtable[kernel][falseerase[n]].empty()) {
				_bddtable[kernel].erase(falseerase[n]);
			}
		}
		//We make new kernels, because previously the originals are const.
		FOBDDKernel* tkernel = _kernels[cat][nr];
		FOBDDKernel* nkernel = _kernels[cat][nextKernelNumber];
		nkernel->replacenumber(nr);
		tkernel->replacenumber(nextKernelNumber);
		_kernels[cat][nr] = nkernel;
		_kernels[cat][nextKernelNumber] = tkernel;
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
	auto result = lookup<FOBDD>(_bddtable, kernel, falsebranch, truebranch);
	if (result != NULL) {
		return result;
	}
	// Lookup failed, create a new bdd
	auto returnvalue = addBDD(kernel, truebranch, falsebranch);
	return returnvalue;

}

const FOBDD* FOBDDManager::getBDD(const FOBDD* bdd, std::shared_ptr<FOBDDManager> manager) {
	Copy copier(manager, shared_from_this());
	return copier.copy(bdd);
}
const FOBDD* FOBDDManager::getBDDTryMaintainOrder(const FOBDD* bdd, std::shared_ptr<FOBDDManager> manager) {
	Copy copier(manager, shared_from_this());
	return copier.copyTryMaintainOrder(bdd);
}


FOBDD* FOBDDManager::addBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	Assert(lookup < FOBDD > (_bddtable, kernel, falsebranch, truebranch) == NULL);
	FOBDD* newbdd = new FOBDD(kernel, truebranch, falsebranch,shared_from_this());
	_bddtable[kernel][falsebranch][truebranch] = newbdd;
	return newbdd;
}

const FOBDDTerm* FOBDDManager::invert(const FOBDDTerm* arg) {
	const DomainElement* minus_one = createDomElem(-1);
	const FOBDDTerm* minus_one_term = getDomainTerm(get(STDSORT::INTSORT), minus_one);
	auto times = get(STDFUNC::PRODUCT);
	times = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(get(STDSORT::INTSORT), arg->sort())), 0);
	vector<const FOBDDTerm*> timesterms(2);
	timesterms[0] = minus_one_term;
	timesterms[1] = arg;
	return getFuncTerm(times, timesterms);
}

const FOBDDKernel* FOBDDManager::getAtomKernel(PFSymbol* symbol, AtomKernelType akt, const vector<const FOBDDTerm*>& args) {
	// Simplification
	Assert(symbol != NULL);
	if (is(symbol, STDPRED::EQ)) {
		if (args[0] == args[1]) {
			return _truekernel;
		}
	} else if (args.size() == 1) {
		if (symbol->sorts()[0]->pred() == symbol) {
			if (symbol->sorts()[0]->builtin() && SortUtils::isSubsort(args[0]->sort(), symbol->sorts()[0])) {
				//Builtin check: only return truekernel if sort is not empty.
				return _truekernel;
			}
		}
	}
	if (_rewriteArithmetic) {
		// Arithmetic rewriting
		// 1. Remove functions
		if (isa<Function>(*symbol) && akt == AtomKernelType::AKT_TWOVALUED) {
			Function* f = dynamic_cast<Function*>(symbol);
			Sort* s = SortUtils::resolve(f->outsort(), args.back()->sort());
			if (s == NULL) {
				return _falsekernel;
			}
			Predicate* equal = get(STDPRED::EQ, s);
			Assert(equal != NULL);
			vector<const FOBDDTerm*> funcargs = args;
			funcargs.pop_back();
			const FOBDDTerm* functerm = getFuncTerm(f, funcargs);
			vector<const FOBDDTerm*> newargs;
			newargs.push_back(functerm);
			newargs.push_back(args.back());
			return getAtomKernel(equal, AtomKernelType::AKT_TWOVALUED, newargs);
		}
		// 2. Move all arithmetic terms to the lefthand side of an (in)equality
		if (VocabularyUtils::isComparisonPredicate(symbol)) {
			const FOBDDTerm* leftarg = args[0];
			const FOBDDTerm* rightarg = args[1];
			if (VocabularyUtils::isNumeric(rightarg->sort())) {
				Assert(VocabularyUtils::isNumeric(leftarg->sort()));
				if (not isa<FOBDDDomainTerm>(*rightarg) || dynamic_cast<const FOBDDDomainTerm*>(rightarg)->value() != createDomElem(0)) {
					const DomainElement* zero = createDomElem(0);
					const FOBDDDomainTerm* zero_term = getDomainTerm(get(STDSORT::NATSORT), zero);
					const FOBDDTerm* minus_rightarg = invert(rightarg);
					Function* plus = get(STDFUNC::ADDITION);
					plus = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(leftarg->sort(), rightarg->sort())), 0);
					Assert(plus != NULL);
					vector<const FOBDDTerm*> plusargs(2);
					plusargs[0] = leftarg;
					plusargs[1] = minus_rightarg;
					const FOBDDTerm* plusterm = getFuncTerm(plus, plusargs);
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = plusterm;
					newargs[1] = zero_term;
					Predicate* newsymbol = dynamic_cast<Predicate*>(symbol);
					auto sort = SortUtils::resolve(newargs[0]->sort(), newargs[1]->sort());
					if (is(symbol, STDPRED::LT)) {
						newsymbol = get(STDPRED::LT, sort);
					} else if (is(symbol, STDPRED::GT)) {
						newsymbol = get(STDPRED::GT, sort);
					} else {
						Assert(is(symbol, STDPRED::EQ));
						newsymbol = get(STDPRED::EQ, sort);
					}
					return getAtomKernel(newsymbol, akt, newargs);
				}
			}
		}

		// Comparison rewriting
		if (VocabularyUtils::isComparisonPredicate(symbol)) {
			if (Multiplication::before(args[0], args[1], shared_from_this())) { //TODO: what does this do?
				vector<const FOBDDTerm*> newargs(2);
				newargs[0] = args[1];
				newargs[1] = args[0];
				Predicate* newsymbol = dynamic_cast<Predicate*>(symbol);
				if (is(symbol, STDPRED::LT)) {
					newsymbol = get(STDPRED::GT, symbol->sorts()[0]);
				} else if (is(symbol, STDPRED::GT)) {
					newsymbol = get(STDPRED::LT, symbol->sorts()[0]);
				}
				//Here, it is ok to not disambiguate the sort again, since we do not change the types of the (in)equalities
				return getAtomKernel(newsymbol, akt, newargs);
			}
		}
	}

	// Lookup
	auto result = lookup<FOBDDAtomKernel>(_atomkerneltable, symbol, akt, args);
	if (result != NULL) {
		return result;
	}

	// Lookup failed, create a new atom kernel
	return addAtomKernel(symbol, akt, args);
}

FOBDDAtomKernel* FOBDDManager::addAtomKernel(PFSymbol* symbol, AtomKernelType akt, const vector<const FOBDDTerm*>& args) {
	Assert(lookup < FOBDDAtomKernel > (_atomkerneltable, symbol, akt, args) == NULL);
	FOBDDAtomKernel* newkernel = new FOBDDAtomKernel(symbol, akt, args, newOrder(args));
	_atomkerneltable[symbol][akt][args] = newkernel;
	_kernels[newkernel->category()][newkernel->number()] = newkernel;
	return newkernel;
}

const FOBDDKernel* FOBDDManager::getQuantKernel(Sort* sort, const FOBDD* bdd) {
	// Simplification
	if (bdd == _truebdd) {
		return _truekernel;
	}
	if (bdd == _falsebdd) {
		return _falsekernel;
	}
	if (longestbranch(bdd) == 2) {
		//First, we try to do some arithmetic simplifications: in case of a short branch, only one subcondition.
		//If this is of the form ? y: F(x) = y, and F consists only of total functions,
		//then we know that this kernel will always be true.
		auto kernel = bdd->kernel();
		if (isa<FOBDDAtomKernel>(*kernel)) {
			auto atomKernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
			auto symbol = atomKernel->symbol();
			if (is(symbol, STDPRED::EQ)) { //Only simplifie for equalities
				const FOBDDDeBruijnIndex* qvar = getDeBruijnIndex(sort, 0);
				const FOBDDTerm* arg = solve(bdd->kernel(), qvar); //Try to rewrite in terms of as F(x) = y
				if (arg != NULL && not containsPartialFunctions(arg)) {
					//If something is found, i.e.~we can rewrite as F(x) = y, then simplify (at least if F maps to the sort of y)
					if (bdd->truebranch() == _truebdd && SortUtils::isSubsort(arg->sort(), sort)) {
						return _truekernel;
					}

				}
			}
		}
	}

	// Lookup
	auto resultingQK = lookup<FOBDDQuantKernel>(_quantkerneltable, sort, bdd);
	if (resultingQK != NULL) {
		return resultingQK;
	}
	// Lookup failed, create a new quantified kernel

	return addQuantKernel(sort, bdd);

}

FOBDDQuantKernel* FOBDDManager::addQuantKernel(Sort* sort, const FOBDD* bdd) {
	Assert(lookup < FOBDDQuantKernel > (_quantkerneltable, sort, bdd) == NULL);
	FOBDDQuantKernel* newkernel = new FOBDDQuantKernel(sort, bdd, newOrderForQuantifiedBDD(bdd));
	_quantkerneltable[sort][bdd] = newkernel;
	_kernels[newkernel->category()][newkernel->number()] = newkernel;
	return newkernel;
}

const FOBDDKernel* FOBDDManager::getAggKernel(const FOBDDTerm* left, CompType comp, const FOBDDTerm* right) {
#ifndef NDEBUG
	if (not isa<FOBDDAggTerm>(*right)) {
		throw notyetimplemented("Creating aggkernel where right is no aggterm ");
		//this can happen when bdds are simplified...
	}
#endif
	const FOBDDAggTerm* newright = dynamic_cast<const FOBDDAggTerm*>(right);
	auto resultingAK = lookup<FOBDDAggKernel>(_aggkerneltable, left, comp, newright);
	if (resultingAK != NULL) {
		return resultingAK;
	}
	return addAggKernel(left, comp, newright);
}

const FOBDDTerm* FOBDDManager::getFOBDDTerm(Term* t){
	if(isa<VarTerm>(*t)){
		auto vt = dynamic_cast<VarTerm*>(t);
		return getVariable(vt->var());
	}else if(isa<FuncTerm>(*t)){
		auto ft = dynamic_cast<FuncTerm*>(t);
		auto symbol= ft->function();
		vector<const FOBDDTerm*> newargs;
		for(auto subterm:ft->subterms()){
			newargs.push_back(getFOBDDTerm(subterm));
		}
		auto fobddfunc = getFuncTerm(symbol,newargs);
		return fobddfunc;
	}else if(isa<DomainTerm>(*t)){
		auto dt = dynamic_cast<DomainTerm*>(t);
		return getDomainTerm(dt);
	}else if(isa<AggTerm>(*t)){
		throw notyetimplemented("Parsing special FOBDDs");
		return NULL;
	}else{
		throw notyetimplemented("Parsing special FOBDDs");
		return NULL;
	}
}

const FOBDDEnumSetExpr* FOBDDManager::getEnumSetExpr(const std::vector<const FOBDDQuantSetExpr*>& subsets, Sort* sort) {
	//TODO: improve this with dynamic programming!
	return addEnumSetExpr(subsets, sort);
}
const FOBDDQuantSetExpr* FOBDDManager::getQuantSetExpr(const std::vector<Sort*>& varsorts, const FOBDD* formula, const FOBDDTerm* term, Sort* sort) {
	//TODO: improve this with dynamic programming!
	return addQuantSetExpr(varsorts, formula, term, sort);
}

FOBDDAggKernel* FOBDDManager::addAggKernel(const FOBDDTerm* left, CompType comp, const FOBDDAggTerm* right) {
	Assert(lookup < FOBDDAggKernel > (_aggkerneltable, left, comp, right) == NULL);
	auto newkernel = new FOBDDAggKernel(left, comp, right, newOrder(right));
	_aggkerneltable[left][comp][right] = newkernel;
	_kernels[newkernel->category()][newkernel->number()] = newkernel;
	return newkernel;
}

const FOBDDVariable* FOBDDManager::getVariable(Variable* var) {
	// Lookup
	auto result = lookup<FOBDDVariable>(_variabletable, var);
	if (result != NULL) {
		return result;
	}

	// Lookup failed, create a new variable
	return addVariable(var);
}

fobddvarset FOBDDManager::getVariables(const varset& vars) {
	fobddvarset bddvars;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		bddvars.insert(getVariable(*it));
	}
	return bddvars;
}

FOBDDVariable* FOBDDManager::addVariable(Variable* var) {
	Assert(lookup < FOBDDVariable > (_variabletable, var) == NULL);
	Assert(var->sort() != NULL);
	FOBDDVariable* newvariable = new FOBDDVariable(_maxid++, var);
	_variabletable[var] = newvariable;
	return newvariable;
}

const FOBDDDeBruijnIndex* FOBDDManager::getDeBruijnIndex(Sort* sort, unsigned int index) {
	// Lookup
	auto result = lookup<FOBDDDeBruijnIndex>(_debruijntable, sort, index);
	if (result != NULL) {
		return result;
	}

	// Lookup failed, create a new De Bruijn index
	return addDeBruijnIndex(sort, index);
}

FOBDDDeBruijnIndex* FOBDDManager::addDeBruijnIndex(Sort* sort, unsigned int index) {
	Assert(lookup < FOBDDDeBruijnIndex > (_debruijntable, sort, index) == NULL);
	auto newindex = new FOBDDDeBruijnIndex(_maxid++, sort, index);
	_debruijntable[sort][index] = newindex;
	return newindex;
}

const FOBDDTerm* FOBDDManager::getFuncTerm(Function* func, const vector<const FOBDDTerm*>& args) {
	if (_rewriteArithmetic) {
		// Arithmetic rewriting TODO: we might want to use a non-recurive version of "RewriteMinus-visitors and so on..."
		// 1. Remove unary minus
		if (is(func, STDFUNC::UNARYMINUS) && Vocabulary::std()->contains(func)) {
			return invert(args[0]);
		}
		// 2. Remove binary minus
		if (is(func, STDFUNC::SUBSTRACTION) && Vocabulary::std()->contains(func)) {
			auto invright = invert(args[1]);
			auto plus = get(STDFUNC::ADDITION);
			plus = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(args[0]->sort(), invright->sort())), 0);
			vector<const FOBDDTerm*> newargs(2);
			newargs[0] = args[0];
			newargs[1] = invright;
			return getFuncTerm(plus, newargs);
		}
		if (is(func, STDFUNC::PRODUCT) && Vocabulary::std()->contains(func)) {
			// 3. Execute computable multiplications
			if (isa<FOBDDDomainTerm>(*(args[0]))) { //First one is a domain term
				auto leftterm = dynamic_cast<const FOBDDDomainTerm*>(args[0]);
				if (leftterm->value()->type() == DET_INT) {
					if (leftterm->value()->value()._int == 0) {
						return leftterm;
					} else if (leftterm->value()->value()._int == 1) {
						return args[1];
					}
				}
				if (isa<FOBDDDomainTerm>(*(args[1]))) { // Both are domain terms
					auto rightterm = dynamic_cast<const FOBDDDomainTerm*>(args[1]);
					FuncInter* fi = func->interpretation(NULL);
					vector<const DomainElement*> copyargs(2);
					copyargs[0] = leftterm->value();
					copyargs[1] = rightterm->value();
					auto result = fi->funcTable()->operator[](copyargs);
					return getDomainTerm(func->outsort(), result);
				}
			} else if (isa<FOBDDDomainTerm>(*(args[1]))) { // Second one is a domain term
				auto rightterm = dynamic_cast<const FOBDDDomainTerm*>(args[1]);
				if (rightterm->value()->type() == DET_INT) {
					if (rightterm->value()->value()._int == 0) {
						return rightterm;
					} else if (rightterm->value()->value()._int == 1) {
						return args[0];
					}
				}
			}
			// 4. Apply distributivity of */2 with respect to +/2
			if (isa<FOBDDFuncTerm>(*(args[0]))) {
				const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
				if (is(leftterm->func(), STDFUNC::ADDITION) && Vocabulary::std()->contains(leftterm->func())) {
					vector<const FOBDDTerm*> newleftargs(2);
					newleftargs[0] = leftterm->args(0);
					newleftargs[1] = args[1];
					vector<const FOBDDTerm*> newrightargs(2);
					newrightargs[0] = leftterm->args(1);
					newrightargs[1] = args[1];
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = getFuncTerm(func, newleftargs);
					newargs[1] = getFuncTerm(func, newrightargs);
					return getFuncTerm(leftterm->func(), newargs);
				}
			}
			if (isa<FOBDDFuncTerm>(*(args[1]))) {
				auto rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
				if (is(rightterm->func(), STDFUNC::ADDITION) && Vocabulary::std()->contains(rightterm->func())) {
					vector<const FOBDDTerm*> newleftargs(2);
					newleftargs[0] = args[0];
					newleftargs[1] = rightterm->args(0);
					vector<const FOBDDTerm*> newrightargs(2);
					newrightargs[0] = args[0];
					newrightargs[1] = rightterm->args(1);
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = getFuncTerm(func, newleftargs);
					newargs[1] = getFuncTerm(func, newrightargs);
					return getFuncTerm(rightterm->func(), newargs);
				}
			}
			// 5. Apply commutativity and associativity to obtain
			// a sorted multiplication of the form ((((t1 * t2) * t3) * t4) * ...)
			//TODO: didn't review this code yet starting from here, first: I'll check Multiplication...
			if (isa<FOBDDFuncTerm>(*(args[0]))) {
				auto leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
				if (is(leftterm->func(), STDFUNC::PRODUCT) && Vocabulary::std()->contains(leftterm->func())) {
					if (isa<FOBDDFuncTerm>(*(args[1]))) {
						const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
						if (is(rightterm->func(), STDFUNC::PRODUCT) && Vocabulary::std()->contains(rightterm->func())) {
							Function* times = get(STDFUNC::PRODUCT);
							Function* times1 = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(leftterm->sort(), rightterm->args(1)->sort())), 0);
							vector<const FOBDDTerm*> leftargs(2);
							leftargs[0] = leftterm;
							leftargs[1] = rightterm->args(1);
							const FOBDDTerm* newleft = getFuncTerm(times1, leftargs);
							Function* times2 = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(newleft->sort(), rightterm->args(0)->sort())), 0);
							vector<const FOBDDTerm*> newargs(2);
							newargs[0] = newleft;
							newargs[1] = rightterm->args(0);
							return getFuncTerm(times2, newargs);
						}
					}
					if (Multiplication::before(args[1], leftterm->args(1), shared_from_this())) {
						Function* times = get(STDFUNC::PRODUCT);
						Function* times1 = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(args[1]->sort(), leftterm->args(0)->sort())), 0);
						vector<const FOBDDTerm*> leftargs(2);
						leftargs[0] = leftterm->args(0);
						leftargs[1] = args[1];
						const FOBDDTerm* newleft = getFuncTerm(times1, leftargs);
						Function* times2 = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(newleft->sort(), leftterm->args(1)->sort())), 0);
						vector<const FOBDDTerm*> newargs(2);
						newargs[0] = newleft;
						newargs[1] = leftterm->args(1);
						return getFuncTerm(times2, newargs);
					}
				}
			} else if (typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
				const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
				if (is(rightterm->func(), STDFUNC::PRODUCT) && Vocabulary::std()->contains(rightterm->func())) {
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = args[1];
					newargs[1] = args[0];
					return getFuncTerm(func, newargs);
				} else if (Multiplication::before(args[1], args[0], shared_from_this())) {
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = args[1];
					newargs[1] = args[0];
					return getFuncTerm(func, newargs);
				}
			} else if (Multiplication::before(args[1], args[0], shared_from_this())) {
				vector<const FOBDDTerm*> newargs(2);
				newargs[0] = args[1];
				newargs[1] = args[0];
				return getFuncTerm(func, newargs);
			}
		} else if (is(func, STDFUNC::ADDITION) && Vocabulary::std()->contains(func)) {
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
				if (is(leftterm->func(), STDFUNC::ADDITION) && Vocabulary::std()->contains(leftterm->func())) {
					if (typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
						const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
						if (is(rightterm->func(), STDFUNC::ADDITION) && Vocabulary::std()->contains(rightterm->func())) {
							Function* plus = get(STDFUNC::ADDITION);
							Function* plus1 = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(leftterm->sort(), rightterm->args(1)->sort())), 0);
							vector<const FOBDDTerm*> leftargs(2);
							leftargs[0] = leftterm;
							leftargs[1] = rightterm->args(1);
							const FOBDDTerm* newleft = getFuncTerm(plus1, leftargs);
							Function* plus2 = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(newleft->sort(), rightterm->args(0)->sort())), 0);
							vector<const FOBDDTerm*> newargs(2);
							newargs[0] = newleft;
							newargs[1] = rightterm->args(0);
							return getFuncTerm(plus2, newargs);
						}
					}
					if (TermOrder::before(args[1], leftterm->args(1), shared_from_this())) {
						Function* plus = get(STDFUNC::ADDITION);
						Function* plus1 = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(args[1]->sort(), leftterm->args(0)->sort())), 0);
						vector<const FOBDDTerm*> leftargs(2);
						leftargs[0] = leftterm->args(0);
						leftargs[1] = args[1];
						const FOBDDTerm* newleft = getFuncTerm(plus1, leftargs);
						Function* plus2 = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(newleft->sort(), leftterm->args(1)->sort())), 0);
						vector<const FOBDDTerm*> newargs(2);
						newargs[0] = newleft;
						newargs[1] = leftterm->args(1);
						return getFuncTerm(plus2, newargs);
					}
				}
			} else if (typeid(*(args[1])) == typeid(FOBDDFuncTerm)) {
				const FOBDDFuncTerm* rightterm = dynamic_cast<const FOBDDFuncTerm*>(args[1]);
				if (is(rightterm->func(), STDFUNC::ADDITION) && Vocabulary::std()->contains(rightterm->func())) {
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = args[1];
					newargs[1] = args[0];
					return getFuncTerm(func, newargs);
				} else if (TermOrder::before(args[1], args[0], shared_from_this())) {
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = args[1];
					newargs[1] = args[0];
					return getFuncTerm(func, newargs);
				}
			} else if (TermOrder::before(args[1], args[0], shared_from_this())) {
				vector<const FOBDDTerm*> newargs(2);
				newargs[0] = args[1];
				newargs[1] = args[0];
				return getFuncTerm(func, newargs);
			}

			// 8. Add terms with the same non-constant part
			const FOBDDTerm* left = 0;
			const FOBDDTerm* right = 0;
			if (typeid(*(args[0])) == typeid(FOBDDFuncTerm)) {
				const FOBDDFuncTerm* leftterm = dynamic_cast<const FOBDDFuncTerm*>(args[0]);
				if (is(leftterm->func(), STDFUNC::ADDITION) && Vocabulary::std()->contains(leftterm->func())) {
					left = leftterm->args(0);
					right = leftterm->args(1);
				} else {
					right = args[0];
				}
			} else {
				right = args[0];
			}
			CollectSameOperationTerms<Multiplication> collect(shared_from_this());
			vector<const FOBDDTerm*> leftflat = collect.getTerms(right);
			vector<const FOBDDTerm*> rightflat = collect.getTerms(args[1]);
			if (leftflat.size() == rightflat.size()) {
				unsigned int n = 1;
				for (; n < leftflat.size(); ++n) {
					if (leftflat[n] != rightflat[n])
						break;
				}
				if (n == leftflat.size()) {
					Function* plus = get(STDFUNC::ADDITION);
					plus = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(leftflat[0]->sort(), rightflat[0]->sort())), 0);
					vector<const FOBDDTerm*> firstargs(2);
					firstargs[0] = leftflat[0];
					firstargs[1] = rightflat[0];
					const FOBDDTerm* currterm = getFuncTerm(plus, firstargs);
					for (unsigned int m = 1; m < leftflat.size(); ++m) {
						Function* times = get(STDFUNC::PRODUCT);
						times = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(currterm->sort(), leftflat[m]->sort())), 0);
						vector<const FOBDDTerm*> nextargs(2);
						nextargs[0] = currterm;
						nextargs[1] = leftflat[m];
						currterm = getFuncTerm(times, nextargs);
					}
					if (left) {
						Function* plus1 = get(STDFUNC::ADDITION);
						plus1 = plus1->disambiguate(vector<Sort*>(3, SortUtils::resolve(currterm->sort(), left->sort())), 0);
						vector<const FOBDDTerm*> lastargs(2);
						lastargs[0] = left;
						lastargs[1] = currterm;
						return getFuncTerm(plus1, lastargs);
					} else {
						return currterm;
					}
				}
			}
		}
	}

	// Lookup
	auto result = lookup<FOBDDFuncTerm>(_functermtable, func, args);
	if (result != NULL) {
		return result;
	}
	// Lookup failed, create a new funcion term
	return addFuncTerm(func, args);
}

FOBDDFuncTerm* FOBDDManager::addFuncTerm(Function* func, const vector<const FOBDDTerm*>& args) {
	Assert(lookup < FOBDDFuncTerm > (_functermtable, func, args) == NULL);
	FOBDDFuncTerm* newarg = new FOBDDFuncTerm(_maxid++, func, args);
	_functermtable[func][args] = newarg;
	return newarg;
}

const FOBDDTerm* FOBDDManager::getAggTerm(AggFunction func, const FOBDDEnumSetExpr* set) {
	auto result = lookup<FOBDDAggTerm>(_aggtermtable, func, set);
	if (result != NULL) {
		return result;
	}
	// Lookup failed, create a new funcion term
	return addAggTerm(func, set);
}

FOBDDAggTerm* FOBDDManager::addAggTerm(AggFunction func, const FOBDDEnumSetExpr* set) {
	Assert(lookup<FOBDDAggTerm>(_aggtermtable, func, set) == NULL);
	FOBDDAggTerm* result = new FOBDDAggTerm(_maxid++, func, set);
	_aggtermtable[func][set] = result;
	return result;
}

const FOBDDDomainTerm* FOBDDManager::getDomainTerm(const DomainTerm* dt) {
	return getDomainTerm(dt->sort(), dt->value());
}

const FOBDDDomainTerm* FOBDDManager::getDomainTerm(Sort* sort, const DomainElement* value) {
	// Lookup
	auto result = lookup<FOBDDDomainTerm>(_domaintermtable, sort, value);
	if (result != NULL) {
		return result;
	}
	// Lookup failed, create a new funcion term
	return addDomainTerm(sort, value);
}

FOBDDDomainTerm* FOBDDManager::addDomainTerm(Sort* sort, const DomainElement* value) {
	Assert(lookup < FOBDDDomainTerm > (_domaintermtable, sort, value) == NULL);
	FOBDDDomainTerm* newdt = new FOBDDDomainTerm(_maxid++, sort, value);
	_domaintermtable[sort][value] = newdt;
	return newdt;
}

FOBDDEnumSetExpr* FOBDDManager::addEnumSetExpr(const std::vector<const FOBDDQuantSetExpr*>& subsets, Sort* sort) {
	//TODO: improve with dynamic programming
	return new FOBDDEnumSetExpr(subsets, sort);
}
FOBDDQuantSetExpr* FOBDDManager::addQuantSetExpr(const std::vector<Sort*>& varsorts, const FOBDD* formula, const FOBDDTerm* term, Sort* sort) {
	//TODO: improve with dynamic programming
	return new FOBDDQuantSetExpr(varsorts, formula, term, sort);
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

	// Recursive case - first try to lookup
	auto result = lookup<const FOBDD>(_negationtable, bdd);
	if (result == NULL) {
		//Lookup failed => Push the negations down to the lowest level
		const FOBDD* falsebranch = negation(bdd->falsebranch());
		const FOBDD* truebranch = negation(bdd->truebranch());
		result = getBDD(bdd->kernel(), truebranch, falsebranch);
		_negationtable[bdd] = result;
	}
	return result;
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
	if (bdd2 < bdd1) { //This check is done in order to have a uniform representation in the _conjunctiontable.  The order doens't matter as long as there is one
		auto temp = bdd1;
		bdd1 = bdd2;
		bdd2 = temp;
	}

	//Try to find the conjunction from previous calculations
	auto result = lookup<const FOBDD>(_conjunctiontable, bdd1, bdd2);
	if (result != NULL) {
		return result;
	}

	//Lookup failed, calculate
	if (*(bdd1->kernel()) < *(bdd2->kernel())) {
		auto falsebranch = conjunction(bdd1->falsebranch(), bdd2);
		auto truebranch = conjunction(bdd1->truebranch(), bdd2);
		result = getBDD(bdd1->kernel(), truebranch, falsebranch);
	} else if (*(bdd1->kernel()) > *(bdd2->kernel())) {
		auto falsebranch = conjunction(bdd1, bdd2->falsebranch());
		auto truebranch = conjunction(bdd1, bdd2->truebranch());
		result = getBDD(bdd2->kernel(), truebranch, falsebranch);
	} else {
		Assert(bdd1->kernel() == bdd2->kernel());
		auto falsebranch = conjunction(bdd1->falsebranch(), bdd2->falsebranch());
		auto truebranch = conjunction(bdd1->truebranch(), bdd2->truebranch());
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
		auto temp = bdd1;
		bdd1 = bdd2;
		bdd2 = temp;
	}
	auto result = lookup<const FOBDD>(_disjunctiontable, bdd1, bdd2);
	if (result != NULL) {
		return result;
	}

	if (*(bdd1->kernel()) < *(bdd2->kernel())) {
		auto falsebranch = disjunction(bdd1->falsebranch(), bdd2);
		auto truebranch = disjunction(bdd1->truebranch(), bdd2);
		result = getBDD(bdd1->kernel(), truebranch, falsebranch);
	} else if (*(bdd1->kernel()) > *(bdd2->kernel())) {
		auto falsebranch = disjunction(bdd1, bdd2->falsebranch());
		auto truebranch = disjunction(bdd1, bdd2->truebranch());
		result = getBDD(bdd2->kernel(), truebranch, falsebranch);
	} else {
		Assert(bdd1->kernel() == bdd2->kernel());
		auto falsebranch = disjunction(bdd1->falsebranch(), bdd2->falsebranch());
		auto truebranch = disjunction(bdd1->truebranch(), bdd2->truebranch());
		result = getBDD(bdd1->kernel(), truebranch, falsebranch);
	}
	_disjunctiontable[bdd1][bdd2] = result;
	return result;
}

//Note: there is a difference with getBDD... getBDD is good when everything is already sorted.  This method serves for creating a new bdd and sorting it at the mean time.
const FOBDD* FOBDDManager::ifthenelse(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	auto result = lookup<const FOBDD>(_ifthenelsetable, kernel, truebranch, falsebranch);
	if (result != NULL) {
		return result;
	}
	if (kernel == _truekernel) {
		return truebranch;
	}
	if (kernel == _falsekernel) {
		return falsebranch;
	}
	if (truebranch == falsebranch) {
		return truebranch;
	}
	const FOBDDKernel* truekernel = truebranch->kernel();
	const FOBDDKernel* falsekernel = falsebranch->kernel();
	if (*kernel < *truekernel) {
		if (*kernel < *falsekernel) {
			result = getBDD(kernel, truebranch, falsebranch);
		} else if (kernel == falsekernel) {

			result = getBDD(kernel, truebranch, falsebranch->falsebranch());
		} else {
			Assert(*kernel > *falsekernel);
			const FOBDD* newtrue = ifthenelse(kernel, truebranch, falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel, truebranch, falsebranch->falsebranch());
			result = getBDD(falsekernel, newtrue, newfalse);
		}
	} else if (kernel == truekernel) {
		if (*kernel < *falsekernel) {
			result = getBDD(kernel, truebranch->truebranch(), falsebranch);
		} else if (kernel == falsekernel) {
			result = getBDD(kernel, truebranch->truebranch(), falsebranch->falsebranch());
		} else {
			Assert(*kernel > *falsekernel);
			const FOBDD* newtrue = ifthenelse(kernel, truebranch, falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel, truebranch, falsebranch->falsebranch());
			result = getBDD(falsekernel, newtrue, newfalse);
		}
	} else {
		Assert(*kernel > *truekernel);
		if (*kernel < *falsekernel || kernel == falsekernel || *truekernel < *falsekernel) {
			const FOBDD* newtrue = ifthenelse(kernel, truebranch->truebranch(), falsebranch);
			const FOBDD* newfalse = ifthenelse(kernel, truebranch->falsebranch(), falsebranch);
			result = getBDD(truekernel, newtrue, newfalse);
		} else if (truekernel == falsekernel) {
			const FOBDD* newtrue = ifthenelse(kernel, truebranch->truebranch(), falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel, truebranch->falsebranch(), falsebranch->falsebranch());
			result = getBDD(truekernel, newtrue, newfalse);
		} else {
			Assert(*falsekernel < *truekernel);
			const FOBDD* newtrue = ifthenelse(kernel, truebranch, falsebranch->truebranch());
			const FOBDD* newfalse = ifthenelse(kernel, truebranch, falsebranch->falsebranch());
			result = getBDD(falsekernel, newtrue, newfalse);
		}
	}
	_ifthenelsetable[kernel][truebranch][falsebranch] = result;
	return result;

}
const FOBDD* FOBDDManager::ifthenelseTryMaintainOrder(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	auto ifthenelsebdd=ifthenelse(kernel,truebranch,falsebranch);
	const FOBDDKernel* truekernel = truebranch->kernel();
	const FOBDDKernel* falsekernel = falsebranch->kernel();
	while((*kernel>*truekernel)||(*kernel>*falsekernel)){
		moveUp(kernel);
	}
	return ifthenelsebdd;
}
const FOBDDQuantSetExpr* FOBDDManager::setquantify(const std::vector<const FOBDDVariable*>& vars, const FOBDD* formula, const FOBDDTerm* term, Sort* sort) {
	std::vector<Sort*> sorts(vars.size());
	int i = 0;
	const FOBDD* bumpedformula = formula;
	const FOBDDTerm* bumpedterm = term;
	for (auto it = vars.cbegin(); it != vars.cend(); it.operator ++(), i++) {
		BumpIndices b(shared_from_this(), *it, 0);
		bumpedformula = b.FOBDDVisitor::change(bumpedformula);
		bumpedterm = bumpedterm->acceptchange(&b);
	}
	i = 0;
	for (auto it = vars.cbegin(); it != vars.cend(); it.operator ++(), i++) {
		sorts[i] = (*it)->sort();
	}
	return getQuantSetExpr(sorts, bumpedformula, bumpedterm, sort);
}

const FOBDD* FOBDDManager::univquantify(const FOBDDVariable* var, const FOBDD* bdd) {
	const FOBDD* negatedbdd = negation(bdd);
	const FOBDD* quantbdd = existsquantify(var, negatedbdd);
	return negation(quantbdd);
}

const FOBDD* FOBDDManager::univquantify(const fobddvarset& qvars, const FOBDD* bdd) {
	const FOBDD* negatedbdd = negation(bdd);
	const FOBDD* quantbdd = existsquantify(qvars, negatedbdd);
	return negation(quantbdd);
}

const FOBDD* FOBDDManager::existsquantify(const FOBDDVariable* var, const FOBDD* bdd) {
	BumpIndices b(shared_from_this(), var, 0);
	const FOBDD* bumped = b.FOBDDVisitor::change(bdd);
	const FOBDD* q = quantify(var->variable()->sort(), bumped);
	return q;
}

const FOBDD* FOBDDManager::existsquantify(const fobddvarset& qvars, const FOBDD* bdd) {
	const FOBDD* result = bdd;
	for (auto it = qvars.cbegin(); it != qvars.cend(); ++it) {
		result = existsquantify(*it, result);
	}
	return result;
}

const FOBDD* FOBDDManager::replaceFreeVariablesByIndices(const fobddvarset& vars, const FOBDD* bdd) {
	auto result = bdd;
	for (auto it = vars.crbegin(); it != vars.crend(); ++it) {
		BumpIndices b(shared_from_this(), *it, 0);
		result = b.FOBDDVisitor::change(result);
		auto index = getDeBruijnIndex((*it)->sort(), 0);
		result = substitute(result, *it, index);
	}
	return result;
}

const FOBDD* FOBDDManager::quantify(Sort* sort, const FOBDD* bdd) {
	CHECKTERMINATION;
	// base case
	if (bdd == _falsebdd) {
		return bdd;
	}
	if (bdd == _truebdd || not bdd->containsDeBruijnIndex(0)) {
		if (sort->builtin()) {
			return sort->interpretation()->empty() ? negation(bdd) : bdd;
		}
		auto sortNotEmpty = getQuantKernel(sort,
				getBDD(getAtomKernel(sort->pred(), AtomKernelType::AKT_TWOVALUED, { getDeBruijnIndex(sort, 0) }), _truebdd, _falsebdd));
		// sortNotEmpty is: ?x[Sort]:Sort(x)
		return getBDD(sortNotEmpty, bdd, _falsebdd);
	}

	// Recursive case
	auto result = lookup<const FOBDD>(_quanttable, sort, bdd);
	if (result != NULL) {
		return result;
	}
	if (bdd->kernel()->category() == KernelOrderCategory::STANDARDCATEGORY) {
		// NOTE: this code explodes the bdd for large longestbranch option
		auto newfalse = quantify(sort, bdd->falsebranch());
		auto newtrue = quantify(sort, bdd->truebranch());
		result = ifthenelse(bdd->kernel(), newtrue, newfalse);
	} else {
		auto kernel = getQuantKernel(sort, bdd);
		result = getBDD(kernel, _truebdd, _falsebdd);
	}
	_quanttable[sort][bdd] = result;
	return result;
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const map<const FOBDDVariable*, const FOBDDVariable*>& mvv) {
	SubstituteTerms<FOBDDVariable, FOBDDVariable> s(shared_from_this(), mvv);
	return s.FOBDDVisitor::change(bdd);
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const map<const FOBDDVariable*, const FOBDDTerm*>& mvv) {
       SubstituteTerms<FOBDDVariable, FOBDDTerm> s(shared_from_this(), mvv);
       return s.FOBDDVisitor::change(bdd);
}

const FOBDDKernel* FOBDDManager::substitute(const FOBDDKernel* kernel, const FOBDDDomainTerm* term, const FOBDDVariable* variable) {
	map<const FOBDDDomainTerm*, const FOBDDVariable*> map;
	map.insert(pair<const FOBDDDomainTerm*, const FOBDDVariable*> { term, variable });
	SubstituteTerms<FOBDDDomainTerm, FOBDDVariable> s(shared_from_this(), map);
	return kernel->acceptchange(&s);
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const FOBDDVariable* variable, const FOBDDDeBruijnIndex* index) {
	std::map<const FOBDDVariable*, const FOBDDDeBruijnIndex*> m;
	m[variable] = index;
	SubstituteTerms<FOBDDVariable, FOBDDDeBruijnIndex> s(shared_from_this(), m);
	return s.FOBDDVisitor::change(bdd);
}

const FOBDD* FOBDDManager::substituteIndex(const FOBDD* bdd, const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable) {
	SubstituteIndex s(shared_from_this(), index, variable);
	return s.FOBDDVisitor::change(bdd);
}
const FOBDDTerm* FOBDDManager::substituteIndex(const FOBDDTerm* bddt, const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable) {
	SubstituteIndex s(shared_from_this(), index, variable);
	return bddt->acceptchange(&s);
}

int FOBDDManager::longestbranch(const FOBDDKernel* kernel) {
	if (kernel == _truekernel || kernel == _falsekernel) {
		return 0;
	}
	if (isa<FOBDDAtomKernel>(*kernel)) {
		return 1;
	} else if (isa<FOBDDQuantKernel>(*kernel)) {
		const FOBDDQuantKernel* qk = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		return longestbranch(qk->bdd()) + 1;
	} else {
		Assert(isa<FOBDDAggKernel>(*kernel));
		const FOBDDAggKernel* ak = dynamic_cast<const FOBDDAggKernel*>(kernel);
		auto set = ak->right()->setexpr();
		int result = 0;
		for (auto subset = set->subsets().cbegin(); subset != set->subsets().cend(); ++subset) {
			for (int i = 0; i < set->size(); i++) {
				int oneres = longestbranch((*subset)->subformula()) + 1;
				result = oneres > result ? oneres : result;
			}

		}
		return result;
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

const FOBDD* FOBDDManager::simplify(const FOBDD* bdd) {
	UngraphFunctions far(shared_from_this());
	bdd = far.FOBDDVisitor::change(bdd);
	TermsToLeft ttl(shared_from_this());
	bdd = ttl.FOBDDVisitor::change(bdd);
	RewriteMinus rm(shared_from_this());
	bdd = rm.FOBDDVisitor::change(bdd);
	ApplyDistributivity dsbtvt(shared_from_this());
	bdd = dsbtvt.FOBDDVisitor::change(bdd);
	OrderTerms<Multiplication> mo(shared_from_this());
	bdd = mo.FOBDDVisitor::change(bdd);
	AddMultSimplifier ms(shared_from_this());
	bdd = ms.FOBDDVisitor::change(bdd);
	OrderTerms<Addition> ao(shared_from_this());
	bdd = ao.FOBDDVisitor::change(bdd);
	bdd = ms.FOBDDVisitor::change(bdd);
	CombineConstsOfMults ta(shared_from_this());
	bdd = ta.FOBDDVisitor::change(bdd);
	AddMultSimplifier neut(shared_from_this());
	bdd = neut.FOBDDVisitor::change(bdd);
	return bdd;
}

bool FOBDDManager::contains(const FOBDDTerm* super, const FOBDDTerm* arg) {
	ContainsTerm ac(shared_from_this());
	return ac.contains(super, arg);
}

const FOBDDTerm* FOBDDManager::solve(const FOBDDKernel* kernel, const FOBDDTerm* argument) {
	if (not _rewriteArithmetic) {
		return NULL;
		//TODO: code is written with the knowledge that we rewrite arith.
	}
	if (not isa<FOBDDAtomKernel>(*kernel)) {
		return NULL;
	}
	auto atom = dynamic_cast<const FOBDDAtomKernel*>(kernel);
	if (not VocabularyUtils::isComparisonPredicate(atom->symbol())) {
		return NULL;
	}
	if (atom->args(0) == argument && not contains(atom->args(1), argument)) {
		return is(atom->symbol(), STDPRED::EQ) ? atom->args(1) : NULL; // y < t cannot be rewritten to t2 < y
	}
	if (atom->args(1) == argument && not contains(atom->args(0), argument)) {
		return atom->args(0);
	}
	if (not SortUtils::isSubsort(atom->symbol()->sorts()[0], get(STDSORT::FLOATSORT))) {
		//We only do arithmetic on float and subsorts
		return NULL;
	}
	if (not SortUtils::isSubsort(argument->sort(), get(STDSORT::FLOATSORT))) {
		//We only do arithmetic on float and subsorts
		return NULL;
	}
#ifndef NDEBUG
	auto domterm = dynamic_cast<const FOBDDDomainTerm*>(atom->args(1));
	Assert(domterm!=NULL);
	auto domtermvalue = domterm->value();
	Assert((domtermvalue->type() == DET_DOUBLE && domtermvalue->value()._double == 0) || (domtermvalue->type() == DET_INT && domtermvalue->value()._int == 0));
	//The rewritings in getatomkernel should guarantee this.
#endif
	CollectSameOperationTerms<Addition> fa(shared_from_this());
	//Collect all occurrences of the wanted argument in the lhs
	vector<const FOBDDTerm*> terms = fa.getTerms(atom->args(0));
	unsigned int occcounter = 0;
	unsigned int occterm = 0;
	unsigned int invertedOcccounter = 0;
	unsigned int invertedOccterm = 0;
	for (size_t n = 0; n < terms.size(); ++n) {
		if (contains(terms[n], argument)) {
			++occcounter;
			occterm = n;
		}
		if (contains(terms[n], invert(argument))) {
			++invertedOcccounter;
			invertedOccterm = n;
		}
	}
	//If there is more than one occurence of the given argument, we don't do anything
	if ((occcounter > 1) || (invertedOcccounter > 1)) {
		return NULL;
	}
	//Now we know that atom is of the form x_1 + x_2 + t[argument] + x_4 + ... op 0,
	//where the x_i do not contain argument and op is either =, < or >
	CollectSameOperationTerms<Multiplication> fm(shared_from_this());
	vector<const FOBDDTerm*> factors;
	if (occcounter == 1) {
		factors = fm.getTerms(terms[occterm]);
		if (factors[1] != argument) {
			return NULL;
		}
	} else if (invertedOcccounter == 1) {
		factors = fm.getTerms(terms[invertedOccterm]);
		if (factors[1] != invert(argument)) {
			return NULL;
		}
	}
	if (not (factors.size() == 2 && factors[1] == argument)) {
		return NULL;
	}
	if (not isa<FOBDDDomainTerm>(*(factors[0]))) {
		return NULL;
	}
	const FOBDDDomainTerm* constant = dynamic_cast<const FOBDDDomainTerm*>(factors[0]);
	double constval;
	auto val = constant->value();
	switch (val->type()) {
	case DomainElementType::DET_INT:
		constval = val->value()._int;
		break;
	case DomainElementType::DET_DOUBLE:
		constval = val->value()._double;
		break;
	default:
		throw IdpException("Invalid code path");
	}
	if (invertedOcccounter != 0 && occcounter == 0) {
		constval = -constval;
		occterm = invertedOccterm;
	}
	//Now we know that atom is of the form x_1 + x_2 + constval * argument + x_4 + ... op 0,
	//where the x_i do not contain argument and op is either =, < or >

	//rewrite to constval * argument + restterm op 0,
	const FOBDDTerm* restterm = 0;
	for (size_t n = 0; n < terms.size(); ++n) {
		if (n != occterm) {
			if (not restterm) {
				restterm = terms[n];
			} else {
				Function* plus = get(STDFUNC::ADDITION);
				plus = plus->disambiguate(vector<Sort*>(3, SortUtils::resolve(restterm->sort(), terms[n]->sort())), 0);
				vector<const FOBDDTerm*> newargs(2);
				newargs[0] = restterm;
				newargs[1] = terms[n];
				restterm = getFuncTerm(plus, newargs);
			}
		}
	}
	if (restterm == NULL) {
		//atom is of the form  constval * argument op 0,
		if (constval < 0) {
			return atom->args(1);
		} else if (is(atom->symbol(), STDPRED::EQ)) {
			return atom->args(1);
		} else {
			//d *arg <0 cannot be rewritten to x < arg if d is positive
			return NULL;
		}
	}

	if (constval == -1) {
		// restterm - d op 0 --> resterm op d
		return restterm;
	} else if (constval == 1) {
		if (is(atom->symbol(), STDPRED::EQ)) {
			return invert(restterm);
		} else {
			return NULL;
		}
	}

	if (SortUtils::isSubsort(restterm->sort(), get(STDSORT::INTSORT))) {
		// TODO: try if constval divides all constant factors
	} else {
		//constval * argument + restterm op 0
		if (constval > 0 && not is(atom->symbol(), STDPRED::EQ)) {
			return NULL;
		}
		Function* times = get(STDFUNC::PRODUCT);
		times = times->disambiguate(vector<Sort*>(3, get(STDSORT::FLOATSORT)), 0);
		vector<const FOBDDTerm*> timesargs(2);
		timesargs[0] = restterm;
		double d = -double(1) / constval;
		timesargs[1] = getDomainTerm(get(STDSORT::FLOATSORT), createDomElem(d));
		return getFuncTerm(times, timesargs);
	}

	return NULL;
}

Formula* FOBDDManager::toFormula(const FOBDD* bdd) {
	BDDToFO btf(shared_from_this());
	return btf.createFormula(bdd);
}

Formula* FOBDDManager::toFormula(const FOBDDKernel* kernel) {
	BDDToFO btf(shared_from_this());
	return btf.createFormula(kernel);
}

Term* FOBDDManager::toTerm(const FOBDDTerm* arg) {
	BDDToFO btf(shared_from_this());
	return btf.createTerm(arg);
}

bool FOBDDManager::containsFuncTerms(const FOBDDKernel* kernel) {
	ContainsFuncTerms ft(shared_from_this());
	return ft.check(kernel);
}

bool FOBDDManager::containsFuncTerms(const FOBDD* bdd) {
	ContainsFuncTerms ft(shared_from_this());
	return ft.check(bdd);
}

bool FOBDDManager::containsPartialFunctions(const FOBDDTerm* arg) {
	ContainsPartialFunctions bpc(shared_from_this());
	return bpc.check(arg);
}

/**
 * Returns true iff the bdd contains the variable
 */
bool FOBDDManager::contains(const FOBDD* bdd, const FOBDDVariable* v) {
	ContainsTerm ct(shared_from_this());
	return ct.contains(bdd, v);
}

/**
 * Returns true iff the kernel contains the variable
 */
bool FOBDDManager::contains(const FOBDDKernel* kernel, const FOBDDVariable* v) {
	ContainsTerm ct(shared_from_this());
	return ct.contains(kernel, v);
}

/**
 * Returns true iff the argument contains the variable
 */
bool FOBDDManager::contains(const FOBDDTerm* arg, const FOBDDVariable* v) {
	ContainsTerm ct(shared_from_this());
	return ct.contains(arg, v);
}

/**
 * Returns true iff the kernel contains the variable
 */
bool FOBDDManager::contains(const FOBDDKernel* kernel, Variable* v) {
	const FOBDDVariable* var = getVariable(v);
	return contains(kernel, var);
}

/**
 * Returns all paths in the given bdd that end in the node 'false'
 * Each path is represented by a vector of pairs of booleans and kernels.
 * The kernels are the succesive nodes in the path,
 * the booleans indicate whether the path continues via the false or true branch.
 */
vector<Path> FOBDDManager::pathsToFalse(const FOBDD* bdd) const {
	vector<Path> result;
	if (bdd == _falsebdd) {
		result.push_back( { });
	} else if (bdd != _truebdd) {
		auto falsePathsToFalse = pathsToFalse(bdd->falsebranch());
		auto truePathsToFalse = pathsToFalse(bdd->truebranch());
		for (auto it = falsePathsToFalse.cbegin(); it != falsePathsToFalse.cend(); ++it) {
			result.push_back( { pair<bool, const FOBDDKernel*> { false, bdd->kernel() } });
			for (auto jt = it->begin(); jt != it->end(); ++jt) {
				result.back().push_back(*jt);
			}
		}
		for (auto it = truePathsToFalse.cbegin(); it != truePathsToFalse.cend(); ++it) {
			result.push_back( { pair<bool, const FOBDDKernel*> { true, bdd->kernel() } });
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
int countkernels(const FOBDD* bdd, const std::shared_ptr<FOBDDManager> manager) {
	Assert(bdd != NULL);
	int result = 0;
	if (bdd != manager->truebdd() && bdd != manager->falsebdd()) {
		result += countkernels(bdd->falsebranch(), manager);
		result += countkernels(bdd->truebranch(), manager);
		result += 1;
		if (isa<FOBDDQuantKernel>(*(bdd->kernel()))) {
			result += countkernels(dynamic_cast<const FOBDDQuantKernel*>(bdd->kernel())->bdd(), manager);
		}
	}
	return result;
}

/**
 * Return all kernels of the given bdd
 */
set<const FOBDDKernel*> allkernels(const FOBDD* bdd, const std::shared_ptr<FOBDDManager> manager) {
	Assert(bdd != NULL);
	set<const FOBDDKernel*> result;
	if (bdd != manager->truebdd() && bdd != manager->falsebdd()) {
		auto falsekernels = allkernels(bdd->falsebranch(), manager);
		auto truekernels = allkernels(bdd->truebranch(), manager);
		result.insert(falsekernels.cbegin(), falsekernels.cend());
		result.insert(truekernels.cbegin(), truekernels.cend());
		result.insert(bdd->kernel());
		if (isa<FOBDDQuantKernel>(*(bdd->kernel()))) {
			auto kernelkernels = allkernels(dynamic_cast<const FOBDDQuantKernel*>(bdd->kernel())->bdd(), manager);
			result.insert(kernelkernels.cbegin(), kernelkernels.cend());
		}
	}
	return result;
}

/**
 * Return all kernels of the given bdd that occur outside the scope of quantifiers
 */
set<const FOBDDKernel*> nonnestedkernels(const FOBDD* bdd, const std::shared_ptr<FOBDDManager> manager) {
	set<const FOBDDKernel*> result;
	if (bdd != manager->truebdd() && bdd != manager->falsebdd()) {
		result.insert(bdd->kernel());
		auto falsekernels = nonnestedkernels(bdd->falsebranch(), manager);
		result.insert(falsekernels.cbegin(), falsekernels.cend());
		auto truekernels = nonnestedkernels(bdd->truebranch(), manager);
		result.insert(truekernels.cbegin(), truekernels.cend());
	}
	return result;
}

/**
 * Returns all variables that occur in the given bdd
 */
fobddvarset variables(const FOBDD* bdd, std::shared_ptr<FOBDDManager> manager) {
	VariableCollector vc(manager);
	return vc.getVariables(bdd);
}

/**
 * Returns all variables that occur in the given kernel
 */
fobddvarset variables(const FOBDDKernel* kernel, std::shared_ptr<FOBDDManager> manager) {
	VariableCollector vc(manager);
	return vc.getVariables(kernel);
}

/**
 * Returns all De Bruijn indices that occur in the given bdd.
 */
fobddindexset indices(const FOBDD* bdd, std::shared_ptr<FOBDDManager> manager) {
	IndexCollector dbc(manager);
	return dbc.getVariables(bdd);
}

/**
 * Returns all De Bruijn indices that occur in the given kernel
 */
fobddindexset indices(const FOBDDKernel* kernel, std::shared_ptr<FOBDDManager> manager) {
	IndexCollector dbc(manager);
	return dbc.getVariables(kernel);
}

void FOBDDManager::optimizeQuery(const FOBDD* query, const fobddvarset& vars, const fobddindexset& indices,
		const Structure* structure) {
	Assert(query != NULL);
	if (query == _truebdd || query == _falsebdd) {
		return;
	}
	auto kernels = allkernels(query, shared_from_this());
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		CHECKTERMINATION;
		// move kernel to the top
		while (kernelAbove(*it) != NULL) {
			moveUp(*it);
		}
		double bestscore = BddStatistics::estimateCostAll(query, vars, indices, structure, shared_from_this());
		int bestposition = 0;
		//AT THIS POINT: bestposition is the number of "movedowns" needed from the top to get to bestpositions
		// move downward
		while (kernelBelow(*it) != NULL) {
			moveDown(*it);
			double currscore = BddStatistics::estimateCostAll(query, vars, indices, structure, shared_from_this());
			if (currscore < bestscore) {
				bestscore = currscore;
				bestposition = 0;
			} else{
				bestposition += -1;
			}
		}
		//AT THIS POINT: the kernel is located at the bottom
		// And bestposition is a negative (or 0) number: the number of moveUps needed.

		// move to best position
		Assert(bestposition <= 0);
		for (int n = 0; n > bestposition; --n) {
			moveUp(*it);
		}
	}
}

double FOBDDManager::getTotalWeigthedCost(const FOBDD* bdd, const fobddvarset& vars,
		const fobddindexset& indices, const Structure* structure, double weightPerAns) {
	// Recursive call
	//TotalBddCost is the total cost of evaluating a bdd + the cost of all answers that are still present.
	double bddCost = BddStatistics::estimateCostAll(bdd, vars, indices, structure, shared_from_this());
	double bddAnswers = BddStatistics::estimateNrAnswers(bdd, vars, indices, structure, shared_from_this());
	return bddCost + (bddAnswers * weightPerAns);
}

const FOBDD* FOBDDManager::makeMore(bool goal, const FOBDD* bdd, const fobddvarset& vars,
		const fobddindexset& ind, const Structure* structure, double weightPerAns) {
	if (isTruebdd(bdd) || isFalsebdd(bdd)) {
		return bdd;
	} else {
		if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
			clog << "For bdd \n" << print(bdd) << "\n";
		}
		// Split variables
		// * kernelvars and kernelindices are all vars and indices that appear in the kernel.
		// * branchvars and branchidices are the rest.
		auto kernelvars = variables(bdd->kernel(), shared_from_this());
		auto kernelindices = indices(bdd->kernel(), shared_from_this());
		fobddvarset branchvars;
		fobddindexset branchindices;
		for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
			if (kernelvars.find(*it) == kernelvars.cend())
				branchvars.insert(*it);
		}
		for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
			if (kernelindices.find(*it) == kernelindices.cend())
				branchindices.insert(*it);
		}

		// Recursive call
		//TotalBddCost is the total cost of evaluating a bdd + the cost of all answers that are still present.
		auto totalBddCost = getTotalWeigthedCost(bdd, vars, ind, structure, weightPerAns);
		if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
			clog << "The total cost is " << totalBddCost << "\n";
		}

		if (isGoalbdd(not goal, bdd->falsebranch())) {
			if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
				clog << "Only interested in the truebranch, which has cost ";
			}
			//If the falsebranch is a bdd we are not interested in, we might just return the truebranch,
			// which will in general have a lower cost, but might provide for more answers.
			auto totalBranchCost = getTotalWeigthedCost(bdd->truebranch(), vars, ind, structure, weightPerAns);
			if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
				clog << totalBranchCost << "\n";
			}
			if (totalBranchCost < totalBddCost) { //Note: smaller branch, so lower cost, but one answer less.
				if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
					clog << "Which is smaller, to going into the true part\n";
				}
				return makeMore(goal, bdd->truebranch(), vars, ind, structure, weightPerAns);
			}
		} else if (isGoalbdd(not goal, bdd->truebranch())) {
			if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
				clog << "Only interested in the falsebranch, which has cost ";
			}
			//If the truebranch is a bdd we are not interested in, we might just return the falsebranch,
			// which will in general have a lower cost, but might provide for more answers.
			auto totalBranchCost = getTotalWeigthedCost(bdd->falsebranch(), vars, ind, structure, weightPerAns);
			if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
				clog << totalBranchCost << "\n";
			}
			if (totalBranchCost < totalBddCost) { //Note: smaller branch, so lower cost, but one answer less.
				if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
					clog << "Which is smaller, to going into the false part\n";
				}
				return makeMore(goal, bdd->falsebranch(), vars, ind, structure, weightPerAns);
			}
		}

		//Number of answers in the kernel.
		double kernelAnswers = BddStatistics::estimateNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure, shared_from_this());
		if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
			clog << "Number of kernel answers is " << kernelAnswers << "\n";
		}

		tablesize kernelUnivSize = univNrAnswers(kernelvars, kernelindices, structure);
		double chance = BddStatistics::estimateChance(bdd->kernel(), structure, shared_from_this());
		if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
			clog << "Kernel chance is " << chance << "\n";
		}

		//For the true and false branch, we calculate the weight as follows:
		//The cost of one answer in truebranch is weight * kernelanswers (they speak about different variables)
		double trueBranchWeight = kernelAnswers * weightPerAns;
		if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
			clog << "Truebranchweight is " << trueBranchWeight << "\n";
			clog << "Making more " << (goal ? "true" : "false") << " for the true branch\n";
		}

		auto newtrue = makeMore(goal, bdd->truebranch(), branchvars, branchindices, structure, trueBranchWeight);

		double kernelFalseAnswers = toDouble(kernelUnivSize) * (1 - chance);

		if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
			clog << "Kernel false answers is " << kernelFalseAnswers << "\n";
		}
		double falsebranchweight = kernelFalseAnswers * weightPerAns;
		if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
			clog << "False branch weight is " << falsebranchweight << "\n";
			clog << "Making more " << (goal ? "true" : "false") << " for the false branch\n";
		}

		auto newfalse = makeMore(goal, bdd->falsebranch(), branchvars, branchindices, structure, falsebranchweight);
		if (newtrue != bdd->truebranch() || newfalse != bdd->falsebranch()) {
			if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
				clog << "Truebranch not true or falsebranch not false, so creating reduced bdd and calling recursively\n";
			}
			return makeMore(goal, getBDD(bdd->kernel(), newtrue, newfalse), vars, ind, structure, weightPerAns);
		} else {
			return bdd;
		}
	}
}

const FOBDD* FOBDDManager::makeMoreFalse(const FOBDD* bdd, const fobddvarset& vars, const fobddindexset& indices,
		const Structure* structure, double weightPerAns) {
	if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
		clog << "Making the following bdd more false: \n" << print(bdd) << "\nResulted in :\n";
	}
	auto result = makeMore(false, bdd, vars, indices, structure, weightPerAns);
	if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
		clog << "\nResulted in :\n" << print(result) << "\n";
	}
	return result;
}

const FOBDD* FOBDDManager::makeMoreTrue(const FOBDD* bdd, const fobddvarset& vars, const fobddindexset& indices,
		const Structure* structure, double weightPerAns) {
	if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
		clog << "Making the following bdd more true: \n" << print(bdd) << "\nResulted in :\n";
	}
	auto result = makeMore(true, bdd, vars, indices, structure, weightPerAns);
	if (getOption(VERBOSE_GEN_AND_CHECK) > 3) {
		clog << "\nResulted in :\n" << print(result) << "\n";
	}
	return result;
}

//Makes more parts of a bdd false. The resulting bdd will contain no symbols
const FOBDD* FOBDDManager::makeMoreFalse(const FOBDD* bdd, const std::set<PFSymbol*>& symbolsToRemove){
	return makeMore(false,bdd,symbolsToRemove);
}

const FOBDD* FOBDDManager::makeMoreTrue(const FOBDD* bdd, const std::set<PFSymbol*>& symbolsToRemove ){
	return makeMore(true,bdd,symbolsToRemove);
}

const FOBDD* FOBDDManager::makeMore(bool goal, const FOBDD* bdd, const std::set<PFSymbol*>& symbolsToRemove) {
	if (isTruebdd(bdd) || isFalsebdd(bdd)) {
		return bdd;
	}
	SymbolCollector sc(shared_from_this());
	auto kernelsymbols = sc.collectSymbols(bdd->kernel());
	for (auto sym : symbolsToRemove) {
		if (kernelsymbols.find(sym) != kernelsymbols.end()) {
			if(goal){
				return makeMore(goal, disjunction(bdd->truebranch(), bdd->falsebranch()), symbolsToRemove);
			}else{
				return makeMore(goal, conjunction(bdd->truebranch(), bdd->falsebranch()), symbolsToRemove);
			}
		}
	}
	auto newtrue = makeMore(goal, bdd->truebranch(), symbolsToRemove);
	auto newfalse = makeMore(goal, bdd->falsebranch(), symbolsToRemove);
	return getBDD(bdd->kernel(), newtrue, newfalse);
}

int created = 0, deleted = 0;
FOBDDManager::FOBDDManager(bool rewriteArithmetic)
		: _maxid(1), _rewriteArithmetic(rewriteArithmetic) {
	_nextorder[KernelOrderCategory::TRUEFALSECATEGORY] = 0;
	_nextorder[KernelOrderCategory::STANDARDCATEGORY] = 0;
	_nextorder[KernelOrderCategory::DEBRUIJNCATEGORY] = 0;
	_truekernel = NULL;
	_falsekernel = NULL;
	_truebdd = NULL;
	_falsebdd = NULL;
}
shared_ptr<FOBDDManager> FOBDDManager::createManager(bool rewriteArithmetic){
	auto returnmanager = shared_ptr<FOBDDManager>(new FOBDDManager(rewriteArithmetic));
	auto ktrue = returnmanager->newOrder(KernelOrderCategory::TRUEFALSECATEGORY);
	auto kfalse = returnmanager->newOrder(KernelOrderCategory::TRUEFALSECATEGORY);
	auto truekernel = new TrueFOBDDKernel(ktrue);
	auto falsekernel = new FalseFOBDDKernel(kfalse);
	returnmanager->setTrueKernel(truekernel);
	returnmanager->setFalseKernel(falsekernel);
	returnmanager->setTrueBDD(new TrueFOBDD(truekernel,returnmanager));
	returnmanager->setFalseBDD(new FalseFOBDD(falsekernel,returnmanager));
	return returnmanager;
}

FOBDDManager::~FOBDDManager() {
	delete _truebdd;
	delete _falsebdd; //!< the BDD 'false'
	delete _truekernel; //!< the kernel 'true'
	delete _falsekernel; //!< the kernel 'false'

// Global tables
	deleteAll<FOBDD>(_bddtable);
	deleteAll<FOBDDKernel>(_kernels);
	/*deleteAll<FOBDDAtomKernel>(_atomkerneltable);
	 deleteAll<FOBDDQuantKernel>(_quantkerneltable);
	 deleteAll<FOBDDAggKernel>(_aggkerneltable);*/ //THOSE THREE ARE DELETED BY THE PREVIOUS deletall
	deleteAll<FOBDDVariable>(_variabletable);
	deleteAll<FOBDDDeBruijnIndex>(_debruijntable);
	deleteAll<FOBDDFuncTerm>(_functermtable);
	deleteAll<FOBDDAggTerm>(_aggtermtable);
	deleteAll<FOBDDDomainTerm>(_domaintermtable);
}
