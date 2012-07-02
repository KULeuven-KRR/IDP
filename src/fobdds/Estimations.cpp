/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "Estimations.hpp"
#include "IncludeComponents.hpp"
#include "FoBddManager.hpp"
#include "FoBddAggKernel.hpp"
#include "FoBddAggTerm.hpp"
#include "FoBddKernel.hpp"
#include "FoBddAtomKernel.hpp"
#include "FoBddQuantKernel.hpp"
#include "FoBddIndex.hpp"
#include "FoBddTerm.hpp"
#include "FoBdd.hpp"
#include "bddvisitors/CheckIsArithmeticFormula.hpp"
#include <algorithm>
#include "structure/information/EstimateBDDInferenceCost.hpp"

using namespace std;

/**
 * Returns the product of the sizes of the interpretations of the sorts of the given variables and indices in the given structure
 */
tablesize univNrAnswers(const varset& vars, const indexset& ind, const AbstractStructure* structure) {
	vector<SortTable*> vst;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		vst.push_back(structure->inter((*it)->variable()->sort()));
	}
	for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
		vst.push_back(structure->inter((*it)->sort()));
	}
	return Universe(vst).size();
}

/**
 * Mapping from each nonnested kernel to its estimated number of answers
 */
map<const FOBDDKernel*, double> BddStatistics::kernelAnswers(const FOBDD* bdd) {
	map<const FOBDDKernel*, double> result;
	auto kernels = nonnestedkernels(bdd, manager);
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		result[*it] = tabledEstimateNrAnswers(*it, variables(*it, manager), indices(*it, manager));
	}
	return result;
}

/**
 * Mapping from each nonnested kernel to its maximum number of answers
 */
map<const FOBDDKernel*, tablesize> BddStatistics::kernelUnivs(const FOBDD* bdd) {
	map<const FOBDDKernel*, tablesize> result;
	auto kernels = nonnestedkernels(bdd, manager);
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		result[*it] = univNrAnswers(variables(*it, manager), indices(*it, manager), structure);
	}
	return result;
}

/**
 * Returns the estimated chance that a BDD evaluates to true
 */
double BddStatistics::estimateChance(const FOBDD* bdd) {
	double result = 0;
	if (bdd == manager->falsebdd()) {
		result = 0;
	} else if (bdd == manager->truebdd()) {
		result = 1;
	} else {
		auto kernchance = tabledEstimateChance(bdd->kernel());
		Assert(kernchance >= 0);
		Assert(kernchance <= 1);
		auto falsechance = tabledEstimateChance(bdd->falsebranch());
		Assert(falsechance >= 0);
		Assert(falsechance <= 1);
		auto truechance = tabledEstimateChance(bdd->truebranch());
		Assert(truechance >= 0);
		Assert(truechance <= 1);
		result = (kernchance * truechance) + ((1 - kernchance) * falsechance);
		Assert(result >= 0);
		Assert(result <= 1);
	}
	return result;
}

/**
 * Returns an estimate of the number of answers to the query { vars, indices | bdd(kernel) }
 * TODO Improve this if functional dependency is known
 */
double BddStatistics::estimateNrAnswers(const FOBDD* bdd, const varset& vars, const indexset& indices) {
	auto chance = tabledEstimateChance(bdd);
	if (chance <= 0) {
		return 0;
	}
	auto univanswers = univNrAnswers(vars, indices, structure);
	if (univanswers.isInfinite()) {
		return getMaxElem<double>();
	}
	Assert(0<= chance && chance <= 1);
	return chance * toDouble(univanswers);
}
double BddStatistics::estimateNrAnswers(const FOBDDKernel* kernel, const varset& vars, const indexset& indices) {
	auto chance = tabledEstimateChance(kernel);
	if (chance <= 0) {
		return 0;
	}
	auto univanswers = univNrAnswers(vars, indices, structure);
	return toDouble(chance * univanswers);
}

template<typename BddNode>
bool isArithmetic(const BddNode* k, FOBDDManager* manager) {
	CheckIsArithmeticFormula ac(manager);
	return ac.isArithmeticFormula(k);
}

/**
 * Estimates the chance that this kernel evaluates to true
 */
double BddStatistics::estimateChance(const FOBDDKernel* kernel) {
	Assert(isa<FOBDDAggKernel>(*kernel) || isa<FOBDDAtomKernel>(*kernel) || isa<FOBDDQuantKernel>(*kernel));

	if (isa<FOBDDAggKernel>(*kernel)) {
		//In principle, Aggkernels have exactly one lefthandside for every other tuple of variables.
		//Hence the chance that an aggkernel succeeds is 1/leftsize
		auto aggk = dynamic_cast<const FOBDDAggKernel*>(kernel);
		auto sortinter = structure->inter(aggk->left()->sort());
		auto sortsize = sortinter->size();
		if (not sortsize.isInfinite()) {
			if (sortsize._size > 0) {
				return 1 / toDouble(sortsize);
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}

	if (isa<FOBDDAtomKernel>(*kernel)) {
		auto atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		auto symbol = atomkernel->symbol();
		PredInter* pinter;
		if (symbol->isPredicate()) {
			pinter = structure->inter(dynamic_cast<Predicate*>(symbol));
		} else {
			Assert(isa<Function>(*symbol));
			pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		}
		Assert(pinter!=NULL);

		auto pt = atomkernel->type() == AtomKernelType::AKT_CF ? pinter->cf() : pinter->ct(); // TODO in general, should be adapted to handle the unknowns
		auto symbolsize = pt->size();
		auto univsize = tablesize(TST_EXACT, 1);
		for (auto it = atomkernel->symbol()->sorts().cbegin(); it != atomkernel->symbol()->sorts().cend(); ++it) {
			univsize *= structure->inter((*it))->size();
		}

		if (symbolsize.isInfinite()) {
			Assert(univsize.isInfinite());
			return 0.5;
		}
		if (univsize.isInfinite()) {
			return 0;
		}
		if (toDouble(univsize) == 0) {
			return 0;
		}
#ifndef NDEBUG
		if (not (toDouble(symbolsize) <= toDouble(univsize))) {
			std::cerr << toDouble(symbolsize) << " vs " << toDouble(univsize) << endl;
			std::cerr <<toString(atomkernel)<<endl;
		}
#endif
		Assert(toDouble(symbolsize) <= toDouble(univsize));
		return toDouble(symbolsize) / toDouble(univsize);

	}

	Assert(isa<FOBDDQuantKernel> (*kernel));
	auto quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);

	// get the table of the sort of the quantified variable
	auto quantsorttable = structure->inter(quantkernel->sort());
	auto quanttablesize = quantsorttable->size();

// FIXME review below
	// some simple checks
//	int quantsize = 0;
	if (quanttablesize.isInfinite()) {
		if (not quantsorttable->approxFinite()) {
			// if the sort is infinite, the kernel is true iff the chance of the bdd is nonzero.
			double bddchance = tabledEstimateChance(quantkernel->bdd());
			return bddchance == 0 ? 0 : 1;
		} else {
			return 0;
		}
	} else {
		if (quanttablesize._size == 0) {
			return 0; // if the sort is empty, the kernel cannot be true
		} else {
//			quantsize = quanttablesize._size;
		}
	}

//	// collect the paths that lead to node 'false'
//	auto pathstofalse = manager->pathsToFalse(quantkernel->bdd());
//
//	// collect all kernels and their estimated number of answers
//	auto subkernels = kernelAnswers(quantkernel->bdd());
//	auto subunivs = kernelUnivs(quantkernel->bdd());
//
//	srand(getOption(IntType::RANDOMSEED));
//	double sum = 0; // stores the sum of the chances obtained by each experiment
//	int sumcount = 0; // stores the number of successfull experiments
//	for (unsigned int experiment = 0; experiment < 10; ++experiment) { // do 10 experiments
//		// An experiment consists of trying to reach N times node 'false',
//		// where N is the size of the domain of the quantified variable.
//
//		auto nbAnswersOfKernels = subkernels;
//		bool fail = false;
//
//		double chance = 1;
//		//TODO What is this element for?
//		for (int element = 0; element < quantsize; ++element) {
//			// Compute possibility of each path
//			vector<double> cumulative_pathsposs;
//			double cumulative_chance = 0;
//			for (unsigned int pathnr = 0; pathnr < pathstofalse.size(); ++pathnr) {
//				double currchance = 1;
//				for (unsigned int nodenr = 0; nodenr < pathstofalse[pathnr].size(); ++nodenr) {
//					tablesize ts = subunivs[pathstofalse[pathnr][nodenr].second];
//					double nodeunivsize = (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) ? ts._size : getMaxElem<double>();
//					if (pathstofalse[pathnr][nodenr].first) {
//						//If the path takes the true branch, kernel chance is its number of answers divided by its universe.
//						currchance = currchance * nbAnswersOfKernels[pathstofalse[pathnr][nodenr].second] / double(nodeunivsize);
//						Assert(currchance>=0);
//					} else {
//						Assert(nodeunivsize >= nbAnswersOfKernels[pathstofalse[pathnr][nodenr].second]);
//						Assert(nodeunivsize >= 0);
//						currchance = currchance * (nodeunivsize - nbAnswersOfKernels[pathstofalse[pathnr][nodenr].second]) / double(nodeunivsize);
//						Assert(currchance>=0);
//					}
//				}
//				cumulative_chance += currchance;
//				cumulative_pathsposs.push_back(cumulative_chance);
//			}
//
//			Assert(cumulative_chance <= 1);
//			if (cumulative_chance > 0) { // there is a possible path to false
//				chance = chance * cumulative_chance;
//
//				// randomly choose a path
//				double toss = double(rand()) / double(RAND_MAX) * cumulative_chance;
//				unsigned int chosenpathnr = lower_bound(cumulative_pathsposs.cbegin(), cumulative_pathsposs.cend(), toss) - cumulative_pathsposs.cbegin();
//				for (unsigned int nodenr = 0; nodenr < pathstofalse[chosenpathnr].size(); ++nodenr) {
//					if (pathstofalse[chosenpathnr][nodenr].first) {
//						auto newNbAnswers = nbAnswersOfKernels[pathstofalse[chosenpathnr][nodenr].second] - (1.0);
//						nbAnswersOfKernels[pathstofalse[chosenpathnr][nodenr].second] = newNbAnswers > 0 ? newNbAnswers : 0;
//					}
//				}
//			} else { // the experiment failed
//				fail = true;
//				break;
//			}
//		}
//
//		if (!fail) {
//			sum += chance;
//			++sumcount;
//		}
//	}

	auto sum = 1;
	auto sumcount = 2;

	// FIXME review above

	if (sum == 0) { // no experiment succeeded
		return 1;
	} else { // at least one experiment succeeded: take average of all successfull experiments
		return 1 - (sum / double(sumcount));
	}
}

// FIXME review this method
double BddStatistics::estimateCostAll(bool sign, const FOBDDKernel* kernel, const varset& vars, const indexset& ind) {
	double maxdouble = getMaxElem<double>();

	if (kernelstorage[sign][kernel][vars].find(ind) != kernelstorage[sign][kernel][vars].cend()) {
		return kernelstorage[sign][kernel][vars][ind];
	}

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

			// Schuif de indices op want we gaan 1 niveau dieper
			if (nbquantvars != 0) {
				for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
					newindices.insert(manager->getDeBruijnIndex((*it)->sort(), (*it)->index() + nbquantvars));
				}
				int i = 0;
				for (auto it = (*quantset)->quantvarsorts().crbegin(); it != (*quantset)->quantvarsorts().crend(); ++it, i++) {
					newindices.insert(manager->getDeBruijnIndex(*it, i));
				}
			} else {
				newindices = ind;
			}

			auto extra = tabledEstimateCostAll((*quantset)->subformula(), newvars, newindices);
			d = (d + extra < maxdouble) ? d + extra : maxdouble;
			if (d == maxdouble) {
				break;
			}
		}
		kernelstorage[sign][kernel][vars][ind] = d;
		return d;
	}

	if (isArithmetic(kernel, manager)) {
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
		for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
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
				if (manager->solve(kernel, infinitevar) == NULL) {
					kernelstorage[sign][kernel][vars][ind] = maxdouble;
					return maxdouble;
				}
			} else {
				Assert(infiniteindex);
				if (manager->solve(kernel, infiniteindex) == NULL) {
					kernelstorage[sign][kernel][vars][ind] = maxdouble;
					return maxdouble;
				}
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
			kernelstorage[sign][kernel][vars][ind] = result;
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
					if (manager->solve(kernel, varsvector[n]) != NULL) {
						double currresult = maxresult / varunivsizes[n];
						if (currresult < bestresult) {
							bestresult = currresult;
						}
					}
				}
				for (unsigned int n = 0; n < indicesvector.size(); ++n) {
					if (manager->solve(kernel, indicesvector[n]) != NULL) {
						double currresult = maxresult / indexunivsizes[n];
						if (currresult < bestresult) {
							bestresult = currresult;
						}
					}
				}
				kernelstorage[sign][kernel][vars][ind] = bestresult;
				return bestresult;
			} else {
				kernelstorage[sign][kernel][vars][ind] = maxdouble;
				return maxdouble;
			}
		}
	} else if (typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		auto atomkernel = dynamic_cast<const FOBDDAtomKernel*>(kernel);
		auto symbol = atomkernel->symbol();
		PredInter* pinter = NULL;
		if (typeid(*symbol) == typeid(Predicate)) {
			pinter = structure->inter(dynamic_cast<Predicate*>(symbol));
		} else {
			pinter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		}
		const PredTable* pt = NULL;
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
				if (manager->contains(*it, *jt)) {
					input = false;
					break;
				}
			}
			if (input) {
				for (auto jt = ind.cbegin(); jt != ind.cend(); ++jt) {
					if ((*it)->containsDeBruijnIndex((*jt)->index())) {
						input = false;
						break;
					}
				}
			}
			pattern.push_back(input);
		}

		EstimateEnumerationCost tce;
		auto result = tce.run(pt, pattern);
		kernelstorage[sign][kernel][vars][ind] = result;
		return result;
	} else {
		Assert(isa<FOBDDQuantKernel>(*kernel));
		auto quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		set<const FOBDDDeBruijnIndex*> newindices;
		for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
			newindices.insert(manager->getDeBruijnIndex((*it)->sort(), (*it)->index() + 1));
		}
		newindices.insert(manager->getDeBruijnIndex(quantkernel->sort(), 0));
		auto result = tabledEstimateCostAll(quantkernel->bdd(), vars, newindices);
		kernelstorage[sign][kernel][vars][ind] = result;
		return result;
	}
}

/**
 * Estimated the cost of generating all answers to this bdd.
 */
double BddStatistics::estimateCostAll(const FOBDD* bdd, const varset& vars, const indexset& ind) {
	// Base case
	if (bdd == manager->truebdd()) {
		return toDouble(univNrAnswers(vars, ind, structure));
	} else if (bdd == manager->falsebdd()) {
		return 1;
	}

	if (bddstorage[bdd][vars].find(ind) != bddstorage[bdd][vars].cend()) {
		return bddstorage[bdd][vars][ind];
	}

	// Recursive case

	// get all variables not in the kernel -> bddvars
	// get all variables in the kernel and in vars -> kernelvars
	set<const FOBDDVariable*, CompareBDDVars> kernelvars, bddvars;
	auto allkernelvars = variables(bdd->kernel(), manager);
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		if (allkernelvars.find(*it) == allkernelvars.cend()) {
			bddvars.insert(*it);
		} else {
			kernelvars.insert(*it);
		}
	}

	// get all indices not in the kernel -> bddinds
	// get all indices in the kernel and in indices -> kernelinds
	set<const FOBDDDeBruijnIndex*> kernelindices, bddindices;
	auto allkernelindices = indices(bdd->kernel(), manager);
	for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
		if (allkernelindices.find(*it) == allkernelindices.cend()) {
			bddindices.insert(*it);
		} else {
			kernelindices.insert(*it);
		}
	}

	auto kernelans = tabledEstimateNrAnswers(bdd->kernel(), kernelvars, kernelindices);
	auto truecost = tabledEstimateCostAll(bdd->truebranch(), bddvars, bddindices);

	auto maxcost = getMaxElem<double>();

	// ONLY TRUE BRANCH: Only cost of kernel eval to true + kernel answers * true branch cost
	if (bdd->falsebranch() == manager->falsebdd()) {
		auto kernelcost = tabledEstimateCostAll(true, bdd->kernel(), kernelvars, kernelindices);
		auto result = kernelcost + (kernelans * truecost);
		if (result - kernelcost != kernelans * truecost) {
			bddstorage[bdd][vars][ind] = maxcost;
			return maxcost;
		}
		bddstorage[bdd][vars][ind] = result;
		return result;
	}

	auto kernelunivsize = univNrAnswers(kernelvars, kernelindices, structure);
	auto invkernelans = kernelunivsize - kernelans;
	auto falsecost = tabledEstimateCostAll(bdd->falsebranch(), bddvars, bddindices);

	// ONLY FALSE BRANCH: Only cost of kernel eval to false + NOT(kernel) answers * false branch cost
	if (bdd->truebranch() == manager->falsebdd()) {
		auto kernelcost = tabledEstimateCostAll(false, bdd->kernel(), kernelvars, kernelindices);
		auto result = kernelcost + (toDouble(invkernelans) * falsecost);
		if (result - kernelcost != (toDouble(invkernelans) * falsecost)) {
			bddstorage[bdd][vars][ind] = maxcost;
			return maxcost;
		}
		bddstorage[bdd][vars][ind] = result;
		return result;
	}

	// BOTH BRANCHES: Cost of single kernel eval * kernelunivsize + true cost * kernel answers + false cost * NOT(kernel) answers
	auto kernelcost = tabledEstimateCostAll(true, bdd->kernel(), { }, { });
	auto result = toDouble(kernelunivsize) * kernelcost + kernelans * truecost + toDouble(invkernelans) * falsecost;
	if (result - (toDouble(kernelunivsize) * kernelcost) != kernelans * truecost + toDouble(invkernelans) * falsecost) {
		bddstorage[bdd][vars][ind] = maxcost;
		return maxcost;
	}
	bddstorage[bdd][vars][ind] = result;
	return result;
}
