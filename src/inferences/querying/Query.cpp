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

#include "Query.hpp"

#include "IncludeComponents.hpp"
#include "theory/Query.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "generators/InstGenerator.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "theory/TheoryUtils.hpp"

PredTable* Querying::solveQuery(Query* q, Structure const * const structure) const {
	std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure;
	if(not structure->approxTwoValued()){
		symbolicstructure = generateNonLiftedBounds(new Theory("",structure->vocabulary(),ParseInfo()), structure);
	}
	return solveQuery(q,structure,symbolicstructure);
}

PredTable* Querying::solveQuery(Query* q, Structure const * const structure, std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure) const {
	if(not VocabularyUtils::isSubVocabulary(q->vocabulary(), structure->vocabulary())){
		throw IdpException("The structure of the query does not interpret all symbols in the query.");
	}
	// translate the formula to a bdd
	std::shared_ptr<FOBDDManager> manager;
	const FOBDD* bdd = NULL;
	auto newquery = q->query()->clone();
	newquery = FormulaUtils::calculateArithmetic(newquery,structure);
	if (not structure->approxTwoValued()) {
		// Note: first graph, because generateBounds is currently incorrect in case of three-valued function terms.
		newquery = FormulaUtils::graphFuncsAndAggs(newquery,structure,{}, true,false);
		bdd = symbolicstructure->evaluate(newquery, TruthType::CERTAIN_TRUE, structure);
		manager = symbolicstructure->obtainManager();
	} else {
		//When working two-valued, we can simply turn formula to BDD
		manager = FOBDDManager::createManager();
		FOBDDFactory factory(manager);
		bdd = factory.turnIntoBdd(newquery);
	}
	newquery->recursiveDelete();

	return solveBdd(q->variables(), manager, bdd, structure);
}

PredTable* Querying::solveBdd(const std::vector<Variable*>& vars, std::shared_ptr<FOBDDManager> manager, const FOBDD* bdd, Structure const * const structure) const {
	Assert(bdd != NULL);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "Query-BDD:" << "\n" << print(bdd) << "\n";
	}
	Assert(manager != NULL);

	varset setvars(vars.cbegin(), vars.cend());
	auto bddvars = manager->getVariables(setvars);
	fobddindexset bddindices;

	Assert(bdd != NULL);

	// create a generator
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	std::map<Variable*,const DomElemContainer*> varsToDomElemContainers;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		data.pattern.push_back(Pattern::OUTPUT);
		auto dec = varsToDomElemContainers.find(*it);
		if (dec == varsToDomElemContainers.cend()) {
			auto res = new const DomElemContainer();
			varsToDomElemContainers[*it] = res;
			data.vars.push_back(res);

		} else {
			data.vars.push_back(dec->second);
		}
		data.bddvars.push_back(manager->getVariable(*it));
		data.universe.addTable(structure->inter((*it)->sort()));
	}
	BDDToGenerator btg(manager);

	InstGenerator* generator = btg.create(data);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "Query-Generator:" << "\n" << print(generator) << "\n";
	}

// Create an empty table
	std::vector<SortTable*> vst;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		vst.push_back(structure->inter((*it)->sort()));
	}
	Universe univ(vst);
	auto result = TableUtils::createPredTable(univ);
	// execute the query
	ElementTuple currtuple(vars.size());
	//cerr <<"Generator: " <<print(generator) <<"\n";
	for (generator->begin(); not generator->isAtEnd(); generator->operator ++()) {
		for (unsigned int n = 0; n < vars.size(); ++n) {
			currtuple[n] = data.vars[n]->get();
		}
		result->add(currtuple);
	}
	delete generator;
	return result;
}

PredTable* Querying::solveBDDQuery(const FOBDD* bdd, Structure const * const structure) const {
	// translate the formula to a bdd
	auto manager= bdd->manager();
	Assert(bdd != NULL);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "FOBDD-Query-BDD:" << "\n" << print(bdd) << "\n";
	}
	Assert(manager != NULL);

	auto bddvars = variables(bdd,manager);
	//TODO: This does not work!

	fobddindexset bddindices;

	Assert(bdd != NULL);

	// create a generator
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	std::map<Variable*,const DomElemContainer*> varsToDomElemContainers;
	for (auto it: bddvars) {
		auto var = it->variable();
		data.pattern.push_back(Pattern::OUTPUT);
		auto dec = varsToDomElemContainers.find(var);
		if (dec == varsToDomElemContainers.cend()) {
			auto res = new const DomElemContainer();
			varsToDomElemContainers[var] = res;
			data.vars.push_back(res);

		} else {
			data.vars.push_back(dec->second);
		}
		data.bddvars.push_back(manager->getVariable(var));
		data.universe.addTable(structure->inter((var)->sort()));
	}
	BDDToGenerator btg(manager);

	InstGenerator* generator = btg.create(data);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "FOBDD-Query-Generator:" << "\n" << print(generator) << "\n";
	}

// Create an empty table
	std::vector<SortTable*> vst;
	for (auto it:bddvars) {
		auto var = it->variable();
		vst.push_back(structure->inter((var)->sort()));
	}
	Universe univ(vst);
	auto result = TableUtils::createPredTable(univ);
	// execute the query
	ElementTuple currtuple(bddvars.size());
	//cerr <<"Generator: " <<print(generator) <<"\n";
	for (generator->begin(); not generator->isAtEnd(); generator->operator ++()) {
		for (unsigned int n = 0; n < bddvars.size(); ++n) {
			currtuple[n] = data.vars[n]->get();
		}
		result->add(currtuple);
	}
	delete generator;
	return result;
}
