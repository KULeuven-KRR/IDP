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
#include "EstimateBDDInferenceCost.hpp"

using namespace std;

/**
 * Return a mapping from the non-nested kernels of the given bdd to their estimated number of answers
 */
map<const FOBDDKernel*, double> BddStatistics::kernelAnswers(const FOBDD* bdd) {
	map<const FOBDDKernel*, double> result;
	auto kernels = nonnestedkernels(bdd, manager);
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		auto vars = variables(*it, manager);
		auto ind = indices(*it, manager);
		result[*it] = estimatedNrAnswers(*it, vars, ind);
	}
	return result;
}

/**
 * Returns a mapping from the nonnested kernels of the BDD to the maximum number of answers
 */
map<const FOBDDKernel*, tablesize> BddStatistics::kernelUnivs(const FOBDD* bdd) {
	map<const FOBDDKernel*, tablesize> result;
	auto kernels = nonnestedkernels(bdd, manager);
	for (auto it = kernels.cbegin(); it != kernels.cend(); ++it) {
		auto vars = variables(*it, manager);
		auto ind = indices(*it, manager);
		result[*it] = univNrAnswers(vars, ind, structure);
	}
	return result;
}

/**
 * Estimates the chance that this kernel evaluates to true
 */
double BddStatistics::estimatedChance(const FOBDDKernel* kernel) {
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
				double bddchance = estimatedChance(quantkernel->bdd());
				return bddchance == 0 ? 0 : 1;
			} else {
				// FIXME implement correctly
				return 0.5;
			}
		}

		// collect the paths that lead to node 'false'
		auto pathstofalse = manager->pathsToFalse(quantkernel->bdd());

		// collect all kernels and their estimated number of answers
		auto subkernels = kernelAnswers(quantkernel->bdd());
		auto subunivs = kernelUnivs(quantkernel->bdd());

		srand(getOption(IntType::RANDOMSEED));
		double sum = 0; // stores the sum of the chances obtained by each experiment
		int sumcount = 0; // stores the number of succesfull experiments
		for (unsigned int experiment = 0; experiment < 10; ++experiment) { // do 10 experiments
			// An experiment consists of trying to reach N times node 'false',
			// where N is the size of the domain of the quantified variable.

			auto nbAnswersOfKernels = subkernels;
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
double BddStatistics::estimatedChance(const FOBDD* bdd) {
	if (bdd == manager->falsebdd()) {
		return 0;
	} else if (bdd == manager->truebdd()) {
		return 1;
	} else {
		double kernchance = estimatedChance(bdd->kernel());
		double falsechance = estimatedChance(bdd->falsebranch());
		double truechance = estimatedChance(bdd->truebranch());
		return (kernchance * truechance) + ((1 - kernchance) * falsechance);
	}
}

/**
 * \brief Returns an estimate of the number of answers to the query { vars | kernel } in the given structure
 */
double BddStatistics::estimatedNrAnswers(const FOBDDKernel* kernel, const set<const FOBDDVariable*, CompareBDDVars>& vars,
		const set<const FOBDDDeBruijnIndex*>& indices) {
	// TODO: improve this if functional dependency is known
	// TODO For example aggkernels typically have only one answer, for the left variable.
	double maxdouble = getMaxElem<double>();
	double kernelchance = estimatedChance(kernel);
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
double BddStatistics::estimatedNrAnswers(const FOBDD* bdd, const set<const FOBDDVariable*, CompareBDDVars>& vars,
		const set<const FOBDDDeBruijnIndex*>& indices) {
	double maxdouble = getMaxElem<double>();
	double bddchance = estimatedChance(bdd);
	tablesize univanswers = univNrAnswers(vars, indices, structure);
	if (univanswers._type == TST_INFINITE || univanswers._type == TST_UNKNOWN) {
		return (bddchance > 0 ? maxdouble : 0);
	} else
		return bddchance * univanswers._size;
}

template<typename BddNode>
bool isArithmetic(const BddNode* k, FOBDDManager* manager) {
	CheckIsArithmeticFormula ac(manager);
	return ac.isArithmeticFormula(k);
}

double BddStatistics::estimatedCostAll(bool sign, const FOBDDKernel* kernel, const varset& vars, const indexset& ind) {
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
				for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
					newindices.insert(manager->getDeBruijnIndex((*it)->sort(), (*it)->index() + nbquantvars));
				}
				int i = 0;
				for (auto it = (*quantset)->quantvarsorts().crbegin(); it != (*quantset)->quantvarsorts().crend(); ++it, i++) {
					newindices.insert(manager->getDeBruijnIndex(*it, i));
				}
			} else {
				newindices = ind;
			};
			for (int i = 0; i < set->size(); i++) {
				double extra = estimatedCostAll((*quantset)->subformula(), newvars, newindices);
				d = (d + extra < maxdouble) ? d + extra : maxdouble;
				if (d == maxdouble) {
					break;
				}
			}
		}
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
				if (manager->solve(kernel, infinitevar) == NULL)
					return maxdouble;
			} else {
				Assert(infiniteindex);
				if (manager->solve(kernel, infiniteindex) == NULL)
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
				return bestresult;
			} else {
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

		EstimateBDDInferenceCost tce;
		double result = tce.run(pt, pattern);
		return result;
	} else {
		Assert(isa<FOBDDQuantKernel>(*kernel));
		// NOTE: implement a better estimator if backjumping on bdds is implemented
		auto quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);
		set<const FOBDDDeBruijnIndex*> newindices;
		for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
			newindices.insert(manager->getDeBruijnIndex((*it)->sort(), (*it)->index() + 1));
		}
		newindices.insert(manager->getDeBruijnIndex(quantkernel->sort(), 0));
		double result = estimatedCostAll(quantkernel->bdd(), vars, newindices);
		return result;
	}
}

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

double BddStatistics::estimatedCostAll(const FOBDD* bdd, const varset& vars, const indexset& ind) {
	double maxdouble = getMaxElem<double>();
	if (bdd == manager->truebdd()) {
		tablesize univsize = univNrAnswers(vars, ind, structure);
		if (univsize._type == TST_INFINITE || univsize._type == TST_UNKNOWN) {
			return maxdouble;
		} else {
			return double(univsize._size);
		}

	} else if (bdd == manager->falsebdd()) {
		return 1;
	} else {
		// split variables
		auto kernelvars = variables(bdd->kernel(), manager);
		auto kernelindices = indices(bdd->kernel(), manager);
		set<const FOBDDVariable*, CompareBDDVars> bddvars;
		set<const FOBDDDeBruijnIndex*> bddindices;
		for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
			if (kernelvars.find(*it) == kernelvars.cend()) {
				bddvars.insert(*it);
			}
		}
		for (auto it = ind.cbegin(); it != ind.cend(); ++it) {
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
			if (ind.find(*it) == ind.cend()) {
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
		if (bdd->falsebranch() == manager->falsebdd()) {
			double kernelcost = estimatedCostAll(true, bdd->kernel(), kernelvars, kernelindices);
			double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices);
			double truecost = estimatedCostAll(bdd->truebranch(), bddvars, bddindices);
			if (kernelcost < maxdouble && kernelans < maxdouble && truecost < maxdouble && kernelcost + (kernelans * truecost) < maxdouble) {
				return kernelcost + (kernelans * truecost);
			} else {
				return maxdouble;
			}
		} else if (bdd->truebranch() == manager->falsebdd()) {
			double kernelcost = estimatedCostAll(false, bdd->kernel(), kernelvars, kernelindices);
			double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices);
			tablesize kernelunivsize = univNrAnswers(kernelvars, kernelindices, structure);
			double invkernans =
					(kernelunivsize._type == TST_INFINITE || kernelunivsize._type == TST_UNKNOWN) ? maxdouble : double(kernelunivsize._size) - kernelans;
			double falsecost = estimatedCostAll(bdd->falsebranch(), bddvars, bddindices);
			if (kernelcost + (invkernans * falsecost) < maxdouble) {
				return kernelcost + (invkernans * falsecost);
			} else {
				return maxdouble;
			}
		} else {
			tablesize kernelunivsize = univNrAnswers(kernelvars, kernelindices, structure);
			set<const FOBDDVariable*, CompareBDDVars> emptyvars;
			set<const FOBDDDeBruijnIndex*> emptyindices;
			double kernelcost = estimatedCostAll(true, bdd->kernel(), emptyvars, emptyindices);
			double truecost = estimatedCostAll(bdd->truebranch(), bddvars, bddindices);
			double falsecost = estimatedCostAll(bdd->falsebranch(), bddvars, bddindices);
			double kernelans = estimatedNrAnswers(bdd->kernel(), kernelvars, kernelindices);
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
