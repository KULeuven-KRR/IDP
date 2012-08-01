/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "CalculateDefinitions.hpp"
#include "inferences/SolverConnection.hpp"
#include "fobdds/FoBddManager.hpp"

#include "theory/TheoryUtils.hpp"

#include "groundtheories/SolverTheory.hpp"

#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

using namespace std;

bool CalculateDefinitions::calculateDefinition(Definition* definition, AbstractStructure* structure) const {
	// TODO duplicate code with modelexpansion
	// Create solver and grounder
	auto data = SolverConnection::createsolver(1);
	Theory theory("", structure->vocabulary(), ParseInfo());
	theory.add(definition);
	auto symstructure = generateBounds(&theory, structure, getOption(BoolType::LIFTEDUNITPROPAGATION));
	auto grounder = GrounderFactory::create({&theory, structure, symstructure, true /*TODO CHECK*/}, data);

	grounder->toplevelRun();
	AbstractGroundTheory* grounding = dynamic_cast<SolverTheory*>(grounder->getGrounding());

	// Run solver
	auto mx = SolverConnection::initsolution(data, 1);
	mx->execute();
	if (getGlobal()->terminateRequested()) {
		throw IdpException("Solver was terminated");
	}

	// Collect solutions
	auto abstractsolutions = mx->getSolutions();
	if (abstractsolutions.empty()) {
		return false;
	} else {
		Assert(abstractsolutions.size() == 1);
		auto model = *(abstractsolutions.cbegin());
		SolverConnection::addLiterals(*model, grounding->translator(), structure);
		SolverConnection::addTerms(*model, grounding->translator(), structure);
		structure->clean();
	}
	// Cleanup
	grounding->recursiveDelete();
	delete (data);
	delete (mx);
	delete (grounder);
	delete (symstructure->manager());
	delete (symstructure);

	return structure->isConsistent();
}

std::vector<AbstractStructure*> CalculateDefinitions::calculateKnownDefinitions(Theory* theory, AbstractStructure* structure) const{
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
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
					if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
						clog << "The given structure is not a model of the definition.\n";
					}
					return std::vector<AbstractStructure*> { };
				}
				theory->remove(currentdefinition->first);
				currentdefinition->first->recursiveDelete();
				opens.erase(currentdefinition);
				fixpoint = false;
			}
		}
	}
	if(not structure->isConsistent()){
		return std::vector<AbstractStructure*> { };
	}
	return {structure};
}

