/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ESTIMATIONS_HPP_
#define ESTIMATIONS_HPP_

#include <map>
#include "FoBddVariable.hpp"
#include "structure/TableSize.hpp"

class AbstractStructure;
class FOBDDManager;
class FOBDDDeBruijnIndex;
class FOBDDKernel;
class FOBDD;

typedef std::set<const FOBDDVariable*, CompareBDDVars> varset;
typedef std::set<const FOBDDDeBruijnIndex*> indexset;

class BddStatistics {
private:
	const AbstractStructure* structure;
	FOBDDManager* manager;

	BddStatistics(const AbstractStructure* structure, FOBDDManager* manager)
			: 	structure(structure),
				manager(manager) {

	}

public:
	// Takes a bddkernel or a bdd
	template<class BDD>
	static double estimateNrAnswers(const BDD* bdd, const varset& vars, const indexset& indices, const AbstractStructure* structure,
			FOBDDManager* manager) {
		auto stats = BddStatistics(structure, manager);
		return stats.estimatedNrAnswers(bdd, vars, indices);
	}

	/**
	 * Takes a bddkernel or a bdd
	 * Returns an estimate of the cost of generating all answers to this bdd in the given structure.
	 */
	template<class BDD>
	static double estimateCostAll(const BDD* bdd, const varset& vars, const indexset& indices, const AbstractStructure* structure,
			FOBDDManager* manager) {
		auto stats = BddStatistics(structure, manager);
		return stats.estimatedCostAll(bdd, vars, indices);
	}

	/**
	 * Takes a bddkernel or a bdd
	 * Returns the probability [0,1] that this bdd evaluates to true in the given structure
	 */
	template<class BDD>
	static double estimateChance(const BDD* bdd, const AbstractStructure* structure, FOBDDManager* manager) {
		auto stats = BddStatistics(structure, manager);
		auto result = stats.estimatedChance(bdd);
		Assert(result>=0 && result<=1);
		return result;
	}

private:
	//NOTE: estimation-algorithms have not been reviewed yet
	double estimatedNrAnswers(const FOBDDKernel*, const varset& vars, const indexset& indices);
	double estimatedNrAnswers(const FOBDD*, const varset& vars, const indexset& indices);
	double estimatedCostAll(bool, const FOBDDKernel*, const varset& vars, const indexset& indices);
	double estimatedCostAll(const FOBDD*, const varset& vars, const indexset& indices);

	std::map<const FOBDDKernel*, double> kernelAnswers(const FOBDD*);
	double estimatedChance(const FOBDDKernel*);
	double estimatedChance(const FOBDD*);

	std::map<const FOBDDKernel*, tablesize> kernelUnivs(const FOBDD* bdd);
};

#endif /* ESTIMATIONS_HPP_ */
