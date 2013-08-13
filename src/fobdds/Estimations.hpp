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

#pragma once

#include <map>
#include <iostream>
#include "FoBddVariable.hpp"
#include "FoBddIndex.hpp"
#include "structure/TableSize.hpp"

class Structure;
class FOBDDManager;
class FOBDDDeBruijnIndex;
class FOBDDKernel;
class FOBDD;

typedef fobddindexset indexset;

class BddStatistics {
private:
	const Structure* structure;
	std::shared_ptr<FOBDDManager> manager;

	// Tabling support
	std::map<const FOBDD*, std::map<fobddvarset, std::map<indexset, double> > > bddstorage;
	std::map<bool, std::map<const FOBDDKernel*, std::map<fobddvarset, std::map<indexset, double> > > > kernelstorage;

	BddStatistics(const Structure* structure, std::shared_ptr<FOBDDManager> manager)
			: 	structure(structure),
				manager(manager) {

	}

public:
	// Takes a bddkernel or a bdd
	template<class BDD>
	static double estimateNrAnswers(const BDD* bdd, const fobddvarset& vars, const indexset& indices, const Structure* structure, std::shared_ptr<FOBDDManager> manager) {
		Assert(manager!=NULL);
		Assert(structure!=NULL);
		auto stats = BddStatistics(structure, manager);
		auto result = stats.tabledEstimateNrAnswers(bdd, vars, indices);
		return result;
	}

	/**
	 * Takes a bddkernel or a bdd
	 * Returns an estimate of the cost of generating all answers to this bdd in the given structure.
	 */
	template<class BDD>
	static double estimateCostAll(const BDD* bdd, const fobddvarset& vars, const indexset& indices, const Structure* structure, std::shared_ptr<FOBDDManager> manager) {
/*		std::cerr << "Estimating costs for " <<print(bdd) <<"\n";
		for(auto i=vars.cbegin(); i!=vars.cend(); ++i) {
			std::cerr <<print(*i) <<" ";
		}
		for(auto i=indices.cbegin(); i!=indices.cend(); ++i) {
			std::cerr <<print(*i) <<" ";
		}
		std::cerr <<"\nCost is ===> ";*/
		Assert(bdd!=NULL);
		Assert(manager!=NULL);
		Assert(structure!=NULL);
		auto stats = BddStatistics(structure, manager);
		auto result = stats.tabledEstimateCostAll(bdd, vars, indices);
		//std::cerr <<result <<"\n";
		return result;
	}

	/**
	 * Takes a bddkernel or a bdd
	 * Returns the probability [0,1] that this bdd evaluates to true in the given structure
	 */
	template<class BDD>
	static double estimateChance(const BDD* bdd, const Structure* structure, std::shared_ptr<FOBDDManager> manager) {
		Assert(manager!=NULL);
		Assert(structure!=NULL);
		auto stats = BddStatistics(structure, manager);
		auto result = stats.tabledEstimateChance(bdd);
		Assert(result>=0 && result<=1);
		return result;
	}

private:
	/*
	 * Calculates the chance that term1 equals a random term of sort sort.
	 */
	double calculateEqualityChance(const FOBDDTerm* term1, Sort* sort, bool ineq);
	std::map<const FOBDD*, std::map<fobddvarset, std::map<indexset, double> > > bddanswers;
	std::map<const FOBDDKernel*, std::map<fobddvarset, std::map<indexset, double> > > kernelanswers;
	double tabledEstimateNrAnswers(const FOBDDKernel* object, const fobddvarset& vars, const indexset& indices){
		auto it = kernelanswers[object][vars].find(indices);
		double result = 0;
		if(it==kernelanswers[object][vars].cend()){
			result = estimateNrAnswers(object, vars, indices);
		}else{
			result = it->second;
		}
		return result;
	}
	double tabledEstimateNrAnswers(const FOBDD* object, const fobddvarset& vars, const indexset& indices){
		auto it = bddanswers[object][vars].find(indices);
		double result = 0;
		if(it==bddanswers[object][vars].cend()){
			result = estimateNrAnswers(object, vars, indices);
		}else{
			result = it->second;
		}
		return result;
	}
	double estimateNrAnswers(const FOBDD* bdd, const fobddvarset& vars, const indexset& indices);
	double estimateNrAnswers(const FOBDDKernel* kernel, const fobddvarset& vars, const indexset& indices);

private:
	std::map<const FOBDD*, std::map<fobddvarset, std::map<indexset, double> > > bddcosts;
	std::map<bool, std::map<const FOBDDKernel*, std::map<fobddvarset, std::map<indexset, double> > > > kernelcosts;
	double tabledEstimateCostAll(bool sign, const FOBDDKernel* object, const fobddvarset& vars, const indexset& indices){
		auto it = kernelcosts[sign][object][vars].find(indices);
		double result = 0;
		if(it==kernelcosts[sign][object][vars].cend()){
			result = estimateCostAll(sign, object, vars, indices);
		}else{
			result = it->second;
		}
		return result;
	}
	double tabledEstimateCostAll(const FOBDD* object, const fobddvarset& vars, const indexset& indices);
	double estimateCostAll(bool, const FOBDDKernel*, const fobddvarset& vars, const indexset& indices);
	double estimateCostAll(const FOBDD*, const fobddvarset& vars, const indexset& indices);

private:
	std::map<const FOBDD*, double> bddchances;
	std::map<const FOBDDKernel*, double> kernelchances;
	double tabledEstimateChance(const FOBDDKernel* object){
		auto it = kernelchances.find(object);
		double result = 0;
		if(it==kernelchances.cend()){
			result = estimateChance(object);
		}else{
			result = it->second;
		}
		return result;
	}
	double tabledEstimateChance(const FOBDD* object){
		auto it = bddchances.find(object);
		double result = 0;
		if(it==bddchances.cend()){
			result = estimateChance(object);
		}else{
			result = it->second;
		}
		return result;
	}
	double estimateChance(const FOBDDKernel* object);
	double estimateChance(const FOBDD* object);

	std::map<const FOBDDKernel*, double> kernelAnswers(const FOBDD*);
	std::map<const FOBDDKernel*, tablesize> kernelUnivs(const FOBDD* bdd);
};
