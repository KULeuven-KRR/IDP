/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "utils/NumericLimits.hpp"
#include "commontypes.hpp"
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

#include "fobdds/FoBddAggKernel.hpp"
#include "fobdds/FoBddAggTerm.hpp"
#include "fobdds/EstimateBDDInferenceCost.hpp"

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

void FOBDDManager::moveUp(const FOBDDKernel* kernel) {
	clearDynamicTables();
	auto cat = kernel->category();
	if (cat != KernelOrderCategory::TRUEFALSECATEGORY) {
		unsigned int nr = kernel->number();
		if (nr != 0) {
			--nr;
			Assert(_kernels.find(cat) != _kernels.cend());
			Assert(_kernels[cat].find(nr) != _kernels[cat].cend());
			const FOBDDKernel* pkernel = _kernels[cat][nr];
			moveDown(pkernel);
		}
	}
}

void FOBDDManager::moveDown(const FOBDDKernel* kernel) {
	clearDynamicTables();
	auto cat = kernel->category();
	if (cat != KernelOrderCategory::TRUEFALSECATEGORY) {
		unsigned int nr = kernel->number();
		vector<const FOBDD*> falseerase;
		vector<const FOBDD*> trueerase;
		Assert(_kernels.find(cat) != _kernels.cend());
		if (_kernels[cat].find(nr + 1) == _kernels[cat].cend()) {
			return;
		}
		const FOBDDKernel* nextkernel = _kernels[cat][nr + 1];
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
	auto result = lookup<FOBDD>(_bddtable, kernel, falsebranch, truebranch);
	if (result != NULL) {
		return result;
	}

	// Lookup failed, create a new bdd
	return addBDD(kernel, truebranch, falsebranch);

}

const FOBDD* FOBDDManager::getBDD(const FOBDD* bdd, FOBDDManager* manager) {
	Copy copier(manager, this);
	return copier.copy(bdd);
}

FOBDD* FOBDDManager::addBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) {
	Assert(lookup < FOBDD > (_bddtable, kernel, falsebranch, truebranch) == NULL);
	FOBDD* newbdd = new FOBDD(kernel, truebranch, falsebranch);
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
			if (Multiplication::before(args[0], args[1])) { //TODO: what does this do?
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
	} else if (bdd == _falsebdd) {
		return _falsekernel;
	} else if (longestbranch(bdd) == 2) {
		const FOBDDDeBruijnIndex* qvar = getDeBruijnIndex(sort, 0);
		const FOBDDTerm* arg = solve(bdd->kernel(), qvar);
		if (arg != NULL && not containsPartialFunctions(arg)) {
			if ((bdd->truebranch() == _truebdd && SortUtils::isSubsort(arg->sort(), sort)) || sort->builtin()) {
				return _truekernel;
			}
			// NOTE: sort->builtin() is used here as an approximate test to see if the sort contains more than
			// one domain element. If that is the case, (? y : F(x) ~= y) is indeed true.
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

set<const FOBDDVariable*, CompareBDDVars> FOBDDManager::getVariables(const set<Variable*>& vars) {
	set<const FOBDDVariable*, CompareBDDVars> bddvars;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		bddvars.insert(getVariable(*it));
	}
	return bddvars;
}

FOBDDVariable* FOBDDManager::addVariable(Variable* var) {
	Assert(lookup < FOBDDVariable > (_variabletable, var) == NULL);
	FOBDDVariable* newvariable = new FOBDDVariable(var);
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
	FOBDDDeBruijnIndex* newindex = new FOBDDDeBruijnIndex(sort, index);
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
					if (Multiplication::before(args[1], leftterm->args(1))) {
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
				} else if (Multiplication::before(args[1], args[0])) {
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = args[1];
					newargs[1] = args[0];
					return getFuncTerm(func, newargs);
				}
			} else if (Multiplication::before(args[1], args[0])) {
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
					if (TermOrder::before(args[1], leftterm->args(1), this)) {
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
				} else if (TermOrder::before(args[1], args[0], this)) {
					vector<const FOBDDTerm*> newargs(2);
					newargs[0] = args[1];
					newargs[1] = args[0];
					return getFuncTerm(func, newargs);
				}
			} else if (TermOrder::before(args[1], args[0], this)) {
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
			CollectSameOperationTerms<Multiplication> collect(this);
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
	FOBDDFuncTerm* newarg = new FOBDDFuncTerm(func, args);
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
	FOBDDAggTerm* result = new FOBDDAggTerm(func, set);
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
	FOBDDDomainTerm* newdt = new FOBDDDomainTerm(sort, value);
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
		const FOBDD* temp = bdd1;
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
	auto result = lookup<const FOBDD>(_disjunctiontable, bdd1, bdd2);
	if (result != NULL) {
		return result;
	}

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
const FOBDDQuantSetExpr* FOBDDManager::setquantify(const std::vector<const FOBDDVariable*>& vars, const FOBDD* formula, const FOBDDTerm* term, Sort* sort) {
	std::vector<Sort*> sorts(vars.size());
	int i = 0;
	const FOBDD* bumpedformula = formula;
	const FOBDDTerm* bumpedterm = term;
	for (auto it = vars.cbegin(); it != vars.cend(); it.operator ++(), i++) {
		BumpIndices b(this, *it, 0);
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

const FOBDD* FOBDDManager::univquantify(const set<const FOBDDVariable*, CompareBDDVars>& qvars, const FOBDD* bdd) {
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

const FOBDD* FOBDDManager::existsquantify(const set<const FOBDDVariable*, CompareBDDVars>& qvars, const FOBDD* bdd) {
	const FOBDD* result = bdd;
	for (auto it = qvars.cbegin(); it != qvars.cend(); ++it) {
		result = existsquantify(*it, result);
	}
	return result;
}

const FOBDD* FOBDDManager::replaceFreeVariablesByIndices(const std::set<const FOBDDVariable*, CompareBDDVars>& vars, const FOBDD* bdd) {
	auto result = bdd;
	for (auto it = vars.crbegin(); it != vars.crend(); ++it) {
		BumpIndices b(this, *it, 0);
		result = b.FOBDDVisitor::change(result);
		auto index = getDeBruijnIndex((*it)->sort(), 0);
		result = substitute(result, *it, index);
	}
	return result;
}

const FOBDD* FOBDDManager::quantify(Sort* sort, const FOBDD* bdd) {
	// base case
	if (bdd == _truebdd || bdd == _falsebdd) {
		if (sort->builtin()) {
			return sort->interpretation()->empty() ? negation(bdd) : bdd;
		}
		auto sortNotEmpty = getQuantKernel(sort,
				getBDD(getAtomKernel(sort->pred(), AtomKernelType::AKT_TWOVALUED, { getDeBruijnIndex(sort, 0) }), _truebdd, _falsebdd)); // ?x[Sort]:Sort(x)
		return getBDD(sortNotEmpty, bdd, negation(bdd));
	}

	// Recursive case
	auto result = lookup<const FOBDD>(_quanttable, sort, bdd);
	if (result != NULL) {
		return result;
	}
	if (bdd->kernel()->category() == KernelOrderCategory::STANDARDCATEGORY) {
		const FOBDD* newfalse = quantify(sort, bdd->falsebranch());
		const FOBDD* newtrue = quantify(sort, bdd->truebranch());
		result = ifthenelse(bdd->kernel(), newtrue, newfalse);
	} else {
		const FOBDDKernel* kernel = getQuantKernel(sort, bdd);
		result = getBDD(kernel, _truebdd, _falsebdd);
	}
	_quanttable[sort][bdd] = result;
	return result;
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const map<const FOBDDVariable*, const FOBDDVariable*>& mvv) {
	SubstituteTerms<FOBDDVariable, FOBDDVariable> s(this, mvv);
	return s.FOBDDVisitor::change(bdd);
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const std::map<const FOBDDDeBruijnIndex*, const FOBDDVariable*>& miv) {
	SubstituteTerms<FOBDDDeBruijnIndex, FOBDDVariable> s(this, miv);
	return s.FOBDDVisitor::change(bdd);
}

const FOBDDTerm* FOBDDManager::substitute(const FOBDDTerm* bddt, const std::map<const FOBDDDeBruijnIndex*, const FOBDDVariable*>& miv) {
	SubstituteTerms<FOBDDDeBruijnIndex, FOBDDVariable> s(this, miv);
	return bddt->acceptchange(&s);
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const map<const FOBDDVariable*, const FOBDDTerm*>& mvv) {
	SubstituteTerms<FOBDDVariable, FOBDDTerm> s(this, mvv);
	return s.FOBDDVisitor::change(bdd);
}

const FOBDDKernel* FOBDDManager::substitute(const FOBDDKernel* kernel, const FOBDDDomainTerm* term, const FOBDDVariable* variable) {
	map<const FOBDDDomainTerm*, const FOBDDVariable*> map;
	map.insert(pair<const FOBDDDomainTerm*, const FOBDDVariable*> { term, variable });
	SubstituteTerms<FOBDDDomainTerm, FOBDDVariable> s(this, map);
	return kernel->acceptchange(&s);
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable) {
	SubstituteIndex s(this, index, variable);
	return s.FOBDDVisitor::change(bdd);
}

const FOBDD* FOBDDManager::substitute(const FOBDD* bdd, const FOBDDVariable* variable, const FOBDDDeBruijnIndex* index) {
	std::map<const FOBDDVariable*, const FOBDDDeBruijnIndex*> m;
	m[variable] = index;
	SubstituteTerms<FOBDDVariable, FOBDDDeBruijnIndex> s(this, m);
	return s.FOBDDVisitor::change(bdd);
}

int FOBDDManager::longestbranch(const FOBDDKernel* kernel) {
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

bool FOBDDManager::contains(const FOBDDTerm* super, const FOBDDTerm* arg) {
	ContainsTerm ac(this);
	return ac.contains(super, arg);
}

const FOBDDTerm* FOBDDManager::solve(const FOBDDKernel* kernel, const FOBDDTerm* argument) {
	if(not _rewriteArithmetic){
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
	auto val = domterm->value();
	Assert((val->type() == DET_DOUBLE && val->value()._double == 0)
			|| (val->type() == DET_INT && val->value()._int == 0));
	//The rewritings in getatomkernel should guarantee this.
#endif

	CollectSameOperationTerms<Addition> fa(this);
	//Collect all occurrences of the wanted argument in the lhs
	vector<const FOBDDTerm*> terms = fa.getTerms(atom->args(0));
	unsigned int occcounter = 0;
	unsigned int occterm;
	unsigned int invertedOcccounter = 0;
	unsigned int invertedOccterm;
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
	if (occcounter != 1 && invertedOcccounter != 1) {
		return NULL;
	}
	//Now we know that atom is of the form x_1 + x_2 + t[argument] + x_4 + ... op 0,
	//where the x_i do not contain argument and op is either =, < or >
	CollectSameOperationTerms<Multiplication> fm(this);
	vector<const FOBDDTerm*> factors;
	if (occcounter == 1) {
		factors = fm.getTerms(terms[occterm]);
	} else if (invertedOcccounter == 1) {
		factors = fm.getTerms(terms[invertedOccterm]);
		factors[1] = invert(factors[1]);
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
	switch(val->type()){
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
	BDDToFO btf(this);
	return btf.createFormula(bdd);
}

Formula* FOBDDManager::toFormula(const FOBDDKernel* kernel) {
	BDDToFO btf(this);
	return btf.createFormula(kernel);
}

Term* FOBDDManager::toTerm(const FOBDDTerm* arg) {
	BDDToFO btf(this);
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

bool FOBDDManager::containsPartialFunctions(const FOBDDTerm* arg) {
	ContainsPartialFunctions bpc(this);
	return bpc.check(arg);
}

/**
 * Returns true iff the bdd contains the variable
 */
bool FOBDDManager::contains(const FOBDD* bdd, const FOBDDVariable* v) {
	ContainsTerm ct(this);
	return ct.contains(bdd, v);
}

/**
 * Returns true iff the kernel contains the variable
 */
bool FOBDDManager::contains(const FOBDDKernel* kernel, const FOBDDVariable* v) {
	ContainsTerm ct(this);
	return ct.contains(kernel, v);
}

/**
 * Returns true iff the argument contains the variable
 */
bool FOBDDManager::contains(const FOBDDTerm* arg, const FOBDDVariable* v) {
	ContainsTerm ct(this);
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
 * Returns the product of the sizes of the interpretations of the sorts of the given variables and indices in the given structure
 */
tablesize univNrAnswers(const set<const FOBDDVariable*, CompareBDDVars>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		const AbstractStructure* structure) {
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
vector<Path> FOBDDManager::pathsToFalse(const FOBDD* bdd) {
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
set<const FOBDDKernel*> FOBDDManager::allkernels(const FOBDD* bdd) {
	Assert(bdd != NULL);
	set<const FOBDDKernel*> result;
	if (bdd != _truebdd && bdd != _falsebdd) {
		auto falsekernels = allkernels(bdd->falsebranch());
		auto truekernels = allkernels(bdd->truebranch());
		result.insert(falsekernels.cbegin(), falsekernels.cend());
		result.insert(truekernels.cbegin(), truekernels.cend());
		result.insert(bdd->kernel());
		if (isa<FOBDDQuantKernel>(*(bdd->kernel()))) {
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
map<const FOBDDKernel*, double> FOBDDManager::kernelAnswers(const FOBDD* bdd, const AbstractStructure* structure) {
	map<const FOBDDKernel*, double> result;
	set<const FOBDDKernel*> kernels = nonnestedkernels(bdd);
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		auto vars = variables(*it);
		auto indices = FOBDDManager::indices(*it);
		result[*it] = estimatedNrAnswers(*it, vars, indices, structure);
	}
	return result;
}

/**
 * Returns all variables that occur in the given bdd
 */
set<const FOBDDVariable*, CompareBDDVars> FOBDDManager::variables(const FOBDD* bdd) {
	VariableCollector vc(this);
	return vc.getVariables(bdd);
}

/**
 * Returns all variables that occur in the given kernel
 */
set<const FOBDDVariable*, CompareBDDVars> FOBDDManager::variables(const FOBDDKernel* kernel) {
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
 * Returns all De Bruijn indices that occur in the given kernel
 */
set<const FOBDDDeBruijnIndex*> FOBDDManager::indices(const FOBDDKernel* kernel) {
	IndexCollector dbc(this);
	return dbc.getVariables(kernel);
}

/**
 * Returns a mapping from the nonnested kernels of the BDD to the maximum number of answers
 */
map<const FOBDDKernel*, tablesize> FOBDDManager::kernelUnivs(const FOBDD* bdd, const AbstractStructure* structure) {
	map<const FOBDDKernel*, tablesize> result;
	set<const FOBDDKernel*> kernels = nonnestedkernels(bdd);
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		auto vars = variables(*it);
		set<const FOBDDDeBruijnIndex*> indices = FOBDDManager::indices(*it);
		result[*it] = univNrAnswers(vars, indices, structure);
	}
	return result;
}

/**
 * Estimates the chance that this kernel evaluates to true
 */
double FOBDDManager::estimatedChance(const FOBDDKernel* kernel, const AbstractStructure* structure) {
	if (isa<FOBDDAggKernel>(*kernel)) {
		//In principle, Aggkernels have exactly one lefthandside for every other tuple of variables.
		//Hence the chance that an aggkernel succeeds is 1/leftsize
		auto aggk = dynamic_cast<const FOBDDAggKernel*>(kernel);
		auto sortinter = structure->inter(aggk->left()->sort());
		tablesize sortsize = sortinter->size();
		if (sortsize._type == TST_APPROXIMATED || sortsize._type == TST_EXACT) {
			double size = double(sortsize._size);
			return size > 0 ? 1 / size : 0;
		}
		return 0;
	}

	if (isa<FOBDDAtomKernel>(*kernel)) {
		const FOBDDAtomKernel* atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		double chance = 0;
		PFSymbol* symbol = atomkernel->symbol();
		PredInter* pinter;
		if (isa<Predicate>(*symbol)) {
			pinter = structure->inter(dynamic_cast<Predicate*>(symbol));
		} else {
			Assert(isa<Function>(*symbol));
			pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		}
		const PredTable* pt = atomkernel->type() == AtomKernelType::AKT_CF ? pinter->cf() : pinter->ct();
		tablesize symbolsize = pt->size();
		double univsize = 1;
		for (auto it = atomkernel->args().cbegin(); it != atomkernel->args().cend(); ++it) {
			tablesize argsize = structure->inter((*it)->sort())->size();
			if (argsize._type == TST_APPROXIMATED || argsize._type == TST_EXACT) {
				univsize = univsize * argsize._size > getMaxElem<double>() ? getMaxElem<double>() : univsize * argsize._size;
			} else {
				univsize = getMaxElem<double>();
				break;
			}
		}
		if (symbolsize._type == TST_APPROXIMATED || symbolsize._type == TST_EXACT) {
			if (univsize < getMaxElem<double>()) {
				chance = double(symbolsize._size) / univsize;
				if (chance > 1) {
					chance = 1;
				}
			} else {
				chance = 0;
			}
		} else {
			// TODO better estimators possible?
			if (univsize < getMaxElem<double>())
				chance = 0.5;
			else
				chance = 0;
		}
		return chance;
	} else { // case of a quantification kernel
		Assert(isa<FOBDDQuantKernel> (*kernel));
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
			if (not quantsorttable->approxFinite()) {
				// if the sort is infinite, the kernel is true iff the chance of the bdd is nonzero.
				double bddchance = estimatedChance(quantkernel->bdd(), structure);
				return bddchance == 0 ? 0 : 1;
			} else {
				// FIXME implement correctly
				return 0.5;
			}
		}

		// collect the paths that lead to node 'false'
		vector<Path> pathstofalse = pathsToFalse(quantkernel->bdd());

		// collect all kernels and their estimated number of answers
		map<const FOBDDKernel*, double> subkernels = kernelAnswers(quantkernel->bdd(), structure);
		map<const FOBDDKernel*, tablesize> subunivs = kernelUnivs(quantkernel->bdd(), structure);

		srand(getOption(IntType::RANDOMSEED));
		double sum = 0; // stores the sum of the chances obtained by each experiment
		int sumcount = 0; // stores the number of succesfull experiments
		for (unsigned int experiment = 0; experiment < 10; ++experiment) { // do 10 experiments
			// An experiment consists of trying to reach N times node 'false',
			// where N is the size of the domain of the quantified variable.

			map<const FOBDDKernel*, double> nbAnswersOfKernels = subkernels;
			bool fail = false;

			double chance = 1;
			//TODO What is this element for?
			for (int element = 0; element < quantsize; ++element) {
				// Compute possibility of each path
				vector<double> cumulative_pathsposs;
				double cumulative_chance = 0;
				for (unsigned int pathnr = 0; pathnr < pathstofalse.size(); ++pathnr) {
					double currchance = 1;
					for (unsigned int nodenr = 0; nodenr < pathstofalse[pathnr].size(); ++nodenr) {
						tablesize ts = subunivs[pathstofalse[pathnr][nodenr].second];
						double nodeunivsize = (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) ? ts._size : getMaxElem<double>();
						if (pathstofalse[pathnr][nodenr].first) {
							//If the path takes the true branch, kernel chance is its number of answers divided by its universe. TODO: WHY "-element"?
							//currchance = currchance * nbAnswersOfKernels[pathstofalse[pathnr][nodenr].second] / double(nodeunivsize - element);
							currchance = currchance * nbAnswersOfKernels[pathstofalse[pathnr][nodenr].second] / double(nodeunivsize);
							Assert(currchance>=0);
						} else {
							//currchance = currchance * (nodeunivsize - nbAnswersOfKernels[pathstofalse[pathnr][nodenr].second] ) / double(nodeunivsize - element);
							Assert(nodeunivsize >= nbAnswersOfKernels[pathstofalse[pathnr][nodenr].second]);
							Assert(nodeunivsize >= 0);
							currchance = currchance * (nodeunivsize - nbAnswersOfKernels[pathstofalse[pathnr][nodenr].second]) / double(nodeunivsize);
							Assert(currchance>=0);
						}
					}
					cumulative_chance += currchance;
					cumulative_pathsposs.push_back(cumulative_chance);
				}

				Assert(cumulative_chance <= 1);
				if (cumulative_chance > 0) { // there is a possible path to false
					chance = chance * cumulative_chance;

					// randomly choose a path
					double toss = double(rand()) / double(RAND_MAX) * cumulative_chance;
					unsigned int chosenpathnr = lower_bound(cumulative_pathsposs.cbegin(), cumulative_pathsposs.cend(), toss) - cumulative_pathsposs.cbegin();
					for (unsigned int nodenr = 0; nodenr < pathstofalse[chosenpathnr].size(); ++nodenr) {
						if (pathstofalse[chosenpathnr][nodenr].first) {
							auto newNbAnswers = nbAnswersOfKernels[pathstofalse[chosenpathnr][nodenr].second] - (1.0);
							nbAnswersOfKernels[pathstofalse[chosenpathnr][nodenr].second] = newNbAnswers > 0 ? newNbAnswers : 0;
						}
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

/**
 * Returns the estimated chance that this BDD evaluates to true
 */
double FOBDDManager::estimatedChance(const FOBDD* bdd, const AbstractStructure* structure) {
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
double FOBDDManager::estimatedNrAnswers(const FOBDDKernel* kernel, const set<const FOBDDVariable*, CompareBDDVars>& vars,
		const set<const FOBDDDeBruijnIndex*>& indices, const AbstractStructure* structure) {
	// TODO: improve this if functional dependency is known
	// TODO For example aggkernels typically have only one answer, for the left variable.
	double maxdouble = getMaxElem<double>();
	double kernelchance = estimatedChance(kernel, structure);
	tablesize univanswers = univNrAnswers(vars, indices, structure);
	if (univanswers._type == TST_INFINITE || univanswers._type == TST_UNKNOWN) {
		return (kernelchance > 0 ? maxdouble : 0);
	} else {
		return kernelchance * univanswers._size;
	}
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | bdd } in the given structure
 */
double FOBDDManager::estimatedNrAnswers(const FOBDD* bdd, const set<const FOBDDVariable*, CompareBDDVars>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		const AbstractStructure* structure) {
	double maxdouble = getMaxElem<double>();
	double bddchance = estimatedChance(bdd, structure);
	tablesize univanswers = univNrAnswers(vars, indices, structure);
	if (univanswers._type == TST_INFINITE || univanswers._type == TST_UNKNOWN) {
		return (bddchance > 0 ? maxdouble : 0);
	} else
		return bddchance * univanswers._size;
}

double FOBDDManager::estimatedCostAll(bool sign, const FOBDDKernel* kernel, const set<const FOBDDVariable*, CompareBDDVars>& vars,
		const set<const FOBDDDeBruijnIndex*>& indices, const AbstractStructure* structure) {
	double maxdouble = getMaxElem<double>();

	if (isa<FOBDDAggKernel>(*kernel)) {
		//TODO: very ad-hoc hack to get some result.  Think this through!!!
		auto aggk = dynamic_cast<const FOBDDAggKernel*>(kernel);
		auto set = aggk->right()->setexpr();
		double d = 0;
		auto newvars = vars;

		if (isa<FOBDDVariable>(*(aggk->left()))) {
			auto leftvar = dynamic_cast<const FOBDDVariable*>(aggk->left());
			newvars.erase(leftvar);
		}
		for (auto quantset = set->subsets().cbegin(); quantset != set->subsets().cend(); quantset++) {
			std::set<const FOBDDDeBruijnIndex*> newindices;
			auto nbquantvars = (*quantset)->quantvarsorts().size();
			if (nbquantvars != 0) {
				for (auto it = indices.cbegin(); it != indices.cend(); ++it) {
					newindices.insert(getDeBruijnIndex((*it)->sort(), (*it)->index() + nbquantvars));
				}
				int i = 0;
				for (auto it = (*quantset)->quantvarsorts().crbegin(); it != (*quantset)->quantvarsorts().crend(); ++it, i++) {
					newindices.insert(getDeBruijnIndex(*it, i));
				}
			} else {
				newindices = indices;
			};
			for (int i = 0; i < set->size(); i++) {
				double extra = estimatedCostAll((*quantset)->subformula(), newvars, newindices, structure);
				d = (d + extra < maxdouble) ? d + extra : maxdouble;
				if (d == maxdouble) {
					break;
				}
			}
		}
		return d;
	}
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
				//TODO: solve method changed, now also includes < and >... Handle this!
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
					//TODO: solve method changed, now also includes < and >... Handle this!
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
		Assert(isa<FOBDDQuantKernel>(*kernel));
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

double FOBDDManager::estimatedCostAll(const FOBDD* bdd, const set<const FOBDDVariable*, CompareBDDVars>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		const AbstractStructure* structure) {

	double maxdouble = getMaxElem<double>();
	if (bdd == _truebdd) {
		tablesize univsize = univNrAnswers(vars, indices, structure);
		if (univsize._type == TST_INFINITE || univsize._type == TST_UNKNOWN) {
			return maxdouble;
		} else {
			return double(univsize._size);
		}

	} else if (bdd == _falsebdd) {
		return 1;
	} else {
		// split variables
		auto kernelvars = variables(bdd->kernel());
		auto kernelindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*, CompareBDDVars> bddvars;
		set<const FOBDDDeBruijnIndex*> bddindices;
		for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
			if (kernelvars.find(*it) == kernelvars.cend()) {
				bddvars.insert(*it);
			}
		}
		for (auto it = indices.cbegin(); it != indices.cend(); ++it) {
			if (kernelindices.find(*it) == kernelindices.cend()) {
				bddindices.insert(*it);
			}
		}
		set<const FOBDDVariable*> removevars;
		set<const FOBDDDeBruijnIndex*> removeindices;
		for (auto it = kernelvars.cbegin(); it != kernelvars.cend(); ++it) {
			if (vars.find(*it) == vars.cend()) {
				removevars.insert(*it);
			}
		}
		for (auto it = kernelindices.cbegin(); it != kernelindices.cend(); ++it) {
			if (indices.find(*it) == indices.cend()) {
				removeindices.insert(*it);
			}
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
			} else {
				return maxdouble;
			}
		} else if (bdd->truebranch() == _falsebdd) {
			double kernelcost = estimatedCostAll(false, bdd->kernel(), kernelvars, kernelindices, structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure);
			tablesize kernelunivsize = univNrAnswers(kernelvars, kernelindices, structure);
			double invkernans =
					(kernelunivsize._type == TST_INFINITE || kernelunivsize._type == TST_UNKNOWN) ? maxdouble : double(kernelunivsize._size) - kernelans;
			double falsecost = estimatedCostAll(bdd->falsebranch(), bddvars, bddindices, structure);
			if (kernelcost + (invkernans * falsecost) < maxdouble) {
				return kernelcost + (invkernans * falsecost);
			} else {
				return maxdouble;
			}
		} else {
			tablesize kernelunivsize = univNrAnswers(kernelvars, kernelindices, structure);
			set<const FOBDDVariable*, CompareBDDVars> emptyvars;
			set<const FOBDDDeBruijnIndex*> emptyindices;
			double kernelcost = estimatedCostAll(true, bdd->kernel(), emptyvars, emptyindices, structure);
			double truecost = estimatedCostAll(bdd->truebranch(), bddvars, bddindices, structure);
			double falsecost = estimatedCostAll(bdd->falsebranch(), bddvars, bddindices, structure);
			double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure);
			if (kernelunivsize._type == TST_UNKNOWN || kernelunivsize._type == TST_INFINITE) {
				return maxdouble;
			} else {
				if ((double(kernelunivsize._size) * kernelcost) + (double(kernelans) * truecost) + ((kernelunivsize._size - kernelans) * falsecost)
						< maxdouble) {
					return (double(kernelunivsize._size) * kernelcost) + (double(kernelans) * truecost) + ((kernelunivsize._size - kernelans) * falsecost);
				} else {
					return maxdouble;
				}
			}
		}
	}
}

void FOBDDManager::optimizeQuery(const FOBDD* query, const set<const FOBDDVariable*, CompareBDDVars>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		const AbstractStructure* structure) {
	Assert(query != NULL);
	if (query != _truebdd && query != _falsebdd) {
		set<const FOBDDKernel*> kernels = allkernels(query);
		for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
			CHECKTERMINATION;
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
			//AT THIS POINT: bestposition is the number of "movedowns" needed from the top to get to bestpositions
			//And the kernel is located at the top

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
			//AT THIS POINT: the kernel is located at the bottom
			// And bestposition is a negative (or 0) number: the number of moveUps needed.

			// move to best position
			Assert(bestposition <= 0);
			//if (bestposition < 0) {
			for (int n = 0; n > bestposition; --n) {
				moveUp(*it);
			}
			/*} else if (bestposition > 0) {
			 for (int n = 0; n < bestposition; ++n)
			 moveDown(*it);
			 }*/
		}
	}
}

const FOBDD* FOBDDManager::makeMore(bool goal, const FOBDD* bdd, const set<const FOBDDVariable*, CompareBDDVars>& vars,
		const set<const FOBDDDeBruijnIndex*>& indices, const AbstractStructure* structure, double weightPerAns) {
	if (isTruebdd(bdd) || isFalsebdd(bdd)) {
		return bdd;
	} else {
		// Split variables
		auto kernelvars = variables(bdd->kernel());
		set<const FOBDDDeBruijnIndex*> kernelindices = FOBDDManager::indices(bdd->kernel());
		set<const FOBDDVariable*, CompareBDDVars> branchvars;
		set<const FOBDDDeBruijnIndex*> branchindices;
		for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
			if (kernelvars.find(*it) == kernelvars.cend())
				branchvars.insert(*it);
		}
		for (auto it = indices.cbegin(); it != indices.cend(); ++it) {
			if (kernelindices.find(*it) == kernelindices.cend())
				branchindices.insert(*it);
		}

		// Recursive call
		double bddCost = estimatedCostAll(bdd, vars, indices, structure);
		double bddAnswers = estimatedNrAnswers(bdd, vars, indices, structure);
		double totalBddCost = getMaxElem<double>();
		if (bddCost + (bddAnswers * weightPerAns) < totalBddCost) {
			totalBddCost = bddCost + (bddAnswers * weightPerAns);
		}

		if (isGoalbdd(not goal, bdd->falsebranch())) {
			double branchCost = estimatedCostAll(bdd->truebranch(), vars, indices, structure);
			double branchAnswers = estimatedNrAnswers(bdd->truebranch(), vars, indices, structure);
			double totalBranchCost = getMaxElem<double>();
			if (branchCost + (branchAnswers * weightPerAns) < totalBranchCost) {
				totalBranchCost = branchCost + (branchAnswers * weightPerAns);
			}
			if (totalBranchCost < totalBddCost) { //Note: smaller branch, so lower cost, but one answer less.
				return makeMore(goal, bdd->truebranch(), vars, indices, structure, weightPerAns);
			}
		} else if (isGoalbdd(not goal, bdd->truebranch())) {
			double branchcost = estimatedCostAll(bdd->falsebranch(), vars, indices, structure);
			double branchans = estimatedNrAnswers(bdd->falsebranch(), vars, indices, structure);
			double totalbranchcost = getMaxElem<double>();
			if (branchcost + (branchans * weightPerAns) < totalbranchcost) {
				totalbranchcost = branchcost + (branchans * weightPerAns);
			}
			if (totalbranchcost < totalBddCost) {
				return makeMore(goal, bdd->falsebranch(), vars, indices, structure, weightPerAns);
			}
		}

		double kernelAnswers = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices, structure);
		double trueBranchWeight = (kernelAnswers * weightPerAns < getMaxElem<double>()) ? kernelAnswers * weightPerAns : getMaxElem<double>();
		const FOBDD* newtrue = makeMore(goal, bdd->truebranch(), branchvars, branchindices, structure, trueBranchWeight);

		tablesize allKernelAnswers = univNrAnswers(kernelvars, kernelindices, structure);
		double chance = estimatedChance(bdd->kernel(), structure);
		double kernelFalseAnswers;
		if (allKernelAnswers._type == TST_APPROXIMATED || allKernelAnswers._type == TST_EXACT) {
			kernelFalseAnswers = allKernelAnswers._size * (1 - chance);
		} else {
			Assert(allKernelAnswers._type == TST_INFINITE || allKernelAnswers._type == TST_UNKNOWN);
			if (chance <= 0) //TODO: i changed this from > 0 to <= 0, seemed more logical... Check for correctness
				kernelFalseAnswers = getMaxElem<double>();
			else
				kernelFalseAnswers = 1; //Why 1?
		}
		double falsebranchweight = (kernelFalseAnswers * weightPerAns < getMaxElem<double>()) ? kernelFalseAnswers * weightPerAns : getMaxElem<double>();
		const FOBDD* newfalse = makeMore(goal, bdd->falsebranch(), branchvars, branchindices, structure, falsebranchweight);
		if (newtrue != bdd->truebranch() || newfalse != bdd->falsebranch()) {
			return makeMore(goal, getBDD(bdd->kernel(), newtrue, newfalse), vars, indices, structure, weightPerAns);
		} else
			return bdd;
	}
}

const FOBDD* FOBDDManager::makeMoreFalse(const FOBDD* bdd, const set<const FOBDDVariable*, CompareBDDVars>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		const AbstractStructure* structure, double weightPerAns) {
	return makeMore(false, bdd, vars, indices, structure, weightPerAns);
}

const FOBDD* FOBDDManager::makeMoreTrue(const FOBDD* bdd, const set<const FOBDDVariable*, CompareBDDVars>& vars, const set<const FOBDDDeBruijnIndex*>& indices,
		const AbstractStructure* structure, double weightPerAns) {
	return makeMore(true, bdd, vars, indices, structure, weightPerAns);
}

FOBDDManager::FOBDDManager(bool rewriteArithmetic)
		: _rewriteArithmetic(rewriteArithmetic) {
	_nextorder[KernelOrderCategory::TRUEFALSECATEGORY] = 0;
	_nextorder[KernelOrderCategory::STANDARDCATEGORY] = 0;
	_nextorder[KernelOrderCategory::DEBRUIJNCATEGORY] = 0;

	KernelOrder ktrue = newOrder(KernelOrderCategory::TRUEFALSECATEGORY);
	KernelOrder kfalse = newOrder(KernelOrderCategory::TRUEFALSECATEGORY);
	_truekernel = new TrueFOBDDKernel(ktrue);
	_falsekernel = new FalseFOBDDKernel(kfalse);
	_truebdd = new TrueFOBDD(_truekernel);
	_falsebdd = new FalseFOBDD(_falsekernel);
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
