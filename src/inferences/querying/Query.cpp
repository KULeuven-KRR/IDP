/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "Query.hpp"

#include "IncludeComponents.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "generators/InstGenerator.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "theory/TheoryUtils.hpp"

PredTable* Querying::solveQuery(Query* q, AbstractStructure* structure) const {
	// translate the formula to a bdd
	FOBDDManager* manager;
	const FOBDD* bdd;
	auto newquery = FormulaUtils::calculateArithmetic(q->query());

	if (not structure->approxTwoValued()) {
		auto generateBDDaccToBounds = generateNaiveApproxBounds(NULL, structure);
		bdd = generateBDDaccToBounds->evaluate(newquery, TruthType::CERTAIN_TRUE);
		manager = generateBDDaccToBounds->manager();
	} else {
		//When working two-valued, we can simply turn formula to BDD
		manager = new FOBDDManager();
		FOBDDFactory factory(manager);
		bdd = factory.turnIntoBdd(newquery);

	}

	Assert(bdd != NULL);
	Assert(manager != NULL);
	std::set<Variable*> vars(q->variables().cbegin(), q->variables().cend());
	auto bddvars = manager->getVariables(vars);
	std::set<const FOBDDDeBruijnIndex*> bddindices;

	// optimize the query
	manager->optimizeQuery(bdd, bddvars, bddindices, structure);
	Assert(bdd != NULL);

	// create a generator
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	for (auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
		data.pattern.push_back(Pattern::OUTPUT);
		data.vars.push_back(new const DomElemContainer());
		data.bddvars.push_back(manager->getVariable(*it));
		data.universe.addTable(structure->inter((*it)->sort()));
	}
	BDDToGenerator btg(manager);

	InstGenerator* generator = btg.create(data);

	// Create an empty table
	EnumeratedInternalPredTable* interntable = new EnumeratedInternalPredTable();
	std::vector<SortTable*> vst;
	for (auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
		vst.push_back(structure->inter((*it)->sort()));
	}
	Universe univ(vst);
	PredTable* result = new PredTable(interntable, univ);

	// execute the query
	ElementTuple currtuple(q->variables().size());
	for (generator->begin(); not generator->isAtEnd(); generator->operator ++()) {
		for (unsigned int n = 0; n < q->variables().size(); ++n) {
			currtuple[n] = data.vars[n]->get();
		}
		result->add(currtuple);
	}
	delete(manager);
	return result;
}
