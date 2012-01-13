/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "inferences/CalculateDefinitions.hpp"
#include "inferences/InferenceSolverConnection.hpp"
#include "inferences/propagation/OptimalPropagation.hpp"

#include "utils/TheoryUtils.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

using namespace std;

bool CalculateDefinitions::calculateDefinition(Definition* definition, AbstractStructure* structure) const {
	// Create solver and grounder
	auto solver = InferenceSolverConnection::createsolver();
	Theory theory("", structure->vocabulary(), ParseInfo());
	theory.add(definition);

	auto symstructure = generateNaiveApproxBounds(&theory, structure);
	GrounderFactory grounderfactory(structure, symstructure);
	Grounder* grounder = grounderfactory.create(&theory, solver);

	grounder->toplevelRun();
	AbstractGroundTheory* grounding = dynamic_cast<GroundTheory<SolverPolicy>*>(grounder->getGrounding());

	// Run solver
	MinisatID::Solution* abstractsolutions = InferenceSolverConnection::initsolution();
	solver->solve(abstractsolutions);
	if (getGlobal()->terminateRequested()) {
		throw IdpException("Solver was terminated");
	}

	// Collect solutions
	if (abstractsolutions->getModels().empty()) {
		return false;
	} else {
		Assert(abstractsolutions->getModels().size() == 1);
		auto model = *(abstractsolutions->getModels().cbegin());
		InferenceSolverConnection::addLiterals(model, grounding->translator(), structure);
		InferenceSolverConnection::addTerms(model, grounding->termtranslator(), structure);
		structure->clean();
	}

	// Cleanup
	grounding->recursiveDelete();
	delete (solver);
	delete (abstractsolutions);
	delete (grounder);
	delete (symstructure);

	return structure->isConsistent();
}

AbstractStructure* CalculateDefinitions::calculateKnownDefinitions(Theory* theory, const AbstractStructure* originalStructure) const {
	auto structure = originalStructure->clone();
	if (getOption(IntType::GROUNDVERBOSITY) >= 1) {
		clog << "Calculating known definitions\n";
	}
	// Collect the open symbols of all definitions
	std::map<Definition*, std::set<PFSymbol*> > opens;
	for (auto it = theory->definitions().cbegin(); it != theory->definitions().cend(); ++it) {
		opens[*it] = DefinitionUtils::opens(*it);
	}

	// Calculate the interpretation of the defined atoms from definitions that do not have
	// three-valued open symbols
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto it = opens.begin(); it != opens.end();) {
			auto currentdefinition = it++; // REASON: set erasure does only invalidate iterators pointing to the erased elements
			// Remove opens that have a two-valued interpretation
			for (auto symbol = currentdefinition->second.begin(); symbol != currentdefinition->second.end();) {
				auto currentsymbol = symbol++; // REASON: set erasure does only invalidate iterators pointing to the erased elements
				if (structure->inter(*currentsymbol)->approxTwoValued()) {
					currentdefinition->second.erase(currentsymbol);
				}
			}
			// If no opens are left, calculate the interpretation of the defined atoms
			if (currentdefinition->second.empty()) {
				bool satisfiable = calculateDefinition(currentdefinition->first, structure);
				if (not satisfiable) {
					return new InconsistentStructure(structure->name(), structure->pi());
				}
				theory->remove(currentdefinition->first);
				opens.erase(currentdefinition);
				fixpoint = false;
			}
		}
	}
	if (not structure->isConsistent()) {
		if (getOption(IntType::GROUNDVERBOSITY) >= 1) {
			std::clog << "Calculating definitions resulted in inconsistent model. \n" << "Theory is unsatisfiable.\n";
		}
	}
	return structure;
}

