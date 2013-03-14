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

#include "CalculateDefinitions.hpp"
#include "inferences/SolverConnection.hpp"
#include "fobdds/FoBddManager.hpp"

#include "theory/TheoryUtils.hpp"

#include "groundtheories/SolverTheory.hpp"

#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/querying/xsb/xsbinterface.hpp"

#include "options.hpp"
#include <iostream>

using namespace std;

bool CalculateDefinitions::calculateDefinition(Definition* definition, AbstractStructure* structure) const {
	if (getOption(XSB)) {
//		cerr <<"Structure before: " <<toString(structure) <<"\n";
		auto xsb_interface = XSBInterface::instance();
		xsb_interface->setStructure(structure);
		// TODO duplicate code with modelexpansion
		// Create solver and grounder
		xsb_interface->loadDefinition(definition);
		//xsbprogrambuilder.load_from_structure
		auto symbols = definition->defsymbols();
		for (auto it = symbols.begin(); it != symbols.end(); ++it) {
			auto sorted = xsb_interface->queryDefinition(*it);
            auto internpredtable1 = new EnumeratedInternalPredTable(sorted);
            auto predtable1 = new PredTable(internpredtable1, structure->universe(*it));

            structure->inter(*it)->ctpt(predtable1);
		}
//		cerr <<"\nStructure after:\n" << toString(structure) <<"\n";
		return structure->isConsistent();
	} else {
		// TODO duplicate code with modelexpansion
		// Create solver and grounder
		auto data = SolverConnection::createsolver(1);
		Theory theory("", structure->vocabulary(), ParseInfo());
		theory.add(definition);
		bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
		bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS);
		auto symstructure = generateBounds(&theory, structure, propagate, LUP);
		auto grounder = GrounderFactory::create({&theory, {structure, symstructure}, true /*TODO CHECK*/}, data);

		bool unsat = grounder->toplevelRun();

		//It's possible that unsat is found (for example when we have a conflict with function constraints)
		if (unsat) {
			// Cleanup
			delete (data);
			delete (grounder);
			delete (symstructure);
			return false;
		}

		Assert(not unsat);
		AbstractGroundTheory* grounding = dynamic_cast<SolverTheory*>(grounder->getGrounding());

		// Run solver
		auto mx = SolverConnection::initsolution(data, 1);
		mx->execute();
		if (getGlobal()->terminateRequested()) {
			throw IdpException("Solver was terminated");
		}

		// Collect solutions
		auto abstractsolutions = mx->getSolutions();
		if(not abstractsolutions.empty()){
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
		delete (symstructure);

		return not abstractsolutions.empty() && structure->isConsistent();
	}
}

// Note: this method should only be called if the XSB option is on
bool CalculateDefinitions::calculateAllDefinitions(std::set<Definition*> definitions, AbstractStructure* structure) const {
//	cerr <<"Structure before: " <<toString(structure) <<"\n";
	auto totaldef = new Definition();
	for (auto it = definitions.cbegin(); it != definitions.cend();++it) {
		auto def = (*it);
		totaldef->add(def->rules());
	}
	auto xsb_interface = XSBInterface::instance();
	xsb_interface->setStructure(structure);
	auto definition = totaldef;
	xsb_interface->loadDefinition(definition);
	auto symbols = definition->defsymbols();
	for (auto it = symbols.cbegin(); it != symbols.cend(); ++it) {
		auto sorted = xsb_interface->queryDefinition(*it);
		auto internpredtable1 = new EnumeratedInternalPredTable(sorted);
		auto predtable1 = new PredTable(internpredtable1, structure->universe(*it));

		structure->inter(*it)->ctpt(predtable1);
	}
	structure->clean();
	xsb_interface->reset();
//	cerr <<"\nStructure after:\n" << toString(structure) <<"\n";
	return structure->isConsistent();
}

std::set<Definition*> CalculateDefinitions::getAllCalculatableDefinitions(Theory* theory, AbstractStructure* structure) const {
	// Calculate the interpretation of the defined atoms from definitions that do not have
	// three-valued open symbols


	// Collect the open symbols of all definitions
	std::map<Definition*, std::set<PFSymbol*> > opens;
	for (auto it = theory->definitions().cbegin(); it != theory->definitions().cend(); ++it) {
		if(DefinitionUtils::hasRecursionOverNegation(*it)) {
			throw notyetimplemented("XSB support for definitions that have recursion over negation");
		}
		else {
			opens[*it] = DefinitionUtils::opens(*it);
		}
	}

	bool fixpoint = false;
	std::set<Definition*> calculatableDefs;
	while (not fixpoint) {
		fixpoint = true;
		for (auto it = opens.begin(); it != opens.end();) {
			auto currentdefinition = it++; // REASON: set erasure does only invalidate iterators pointing to the erased elements

			// Remove opens that have a two-valued interpretation
			// Loop over all symbols to see which ones are not given / cannot be calculated
			for (auto symbol = currentdefinition->second.cbegin(); symbol != currentdefinition->second.cend();) {
				auto currentsymbol = symbol++; // REASON: set erasure does only invalidate iterators pointing to the erased elements
				if (structure->inter(*currentsymbol)->approxTwoValued()) {
					currentdefinition->second.erase(currentsymbol);
				} else {
					// Else: check if the symbol is defined by one of the definitions we know we can fully calculate.
					for (auto it2 = calculatableDefs.cbegin(); it2 != calculatableDefs.cend();) {
						auto def = it2++;
						if ((*def)->defsymbols().find(*currentsymbol) != (*def)->defsymbols().end()) {
							currentdefinition->second.erase(currentsymbol);
							break;
						}
					}
				}
			}
			// If no opens are left, add this definition to the definitions that can be calculated
			if (currentdefinition->second.empty()) {
				calculatableDefs.insert(currentdefinition->first);
				opens.erase(currentdefinition->first);
				fixpoint = false;
			}
		}
	}
	return calculatableDefs;
}

std::vector<AbstractStructure*> CalculateDefinitions::calculateKnownDefinitions(Theory* theory, AbstractStructure* structure) const {
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Calculating known definitions\n";
	}

	if (getOption(XSB)) {
		// Calculate the interpretation of the defined atoms from definitions that do not have
		// three-valued open symbols
		std::set<Definition*> calculatableDefs = getAllCalculatableDefinitions(theory,structure);
		bool satisfiable = calculateAllDefinitions(calculatableDefs, structure);
		if (not satisfiable) {
			if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
				clog << "The given structure is not a model of the definition.\n";
			}
			return std::vector<AbstractStructure*> { };
		}
		for (auto it2 = calculatableDefs.begin(); it2 != calculatableDefs.end();) {
			auto def = it2++;
			theory->remove(*def);
			(*def)->recursiveDelete();
		}
		if (not structure->isConsistent()) {
			return std::vector<AbstractStructure*> { };
		}
	} else {
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
		if (not structure->isConsistent()) {
			return std::vector<AbstractStructure*> { };
		}
	}
	return {structure};
}

