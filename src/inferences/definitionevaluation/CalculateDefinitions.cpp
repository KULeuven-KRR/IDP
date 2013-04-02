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

bool CalculateDefinitions::calculateDefinition(Definition* definition, AbstractStructure* structure, bool withxsb) {
	// TODO duplicate code with modelexpansion

	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
		clog << "Calculating definition: " <<  toString(definition) << "\n";
	}
	if (withxsb) {
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
			clog << "Calculating the above definition using XSB\n";
		}
		auto xsb_interface = XSBInterface::instance();
		xsb_interface->setStructure(structure);

		xsb_interface->loadDefinition(definition);
		auto symbols = definition->defsymbols();
		for (auto it = symbols.begin(); it != symbols.end(); ++it) {
			auto sorted = xsb_interface->queryDefinition(*it);
            auto internpredtable1 = new EnumeratedInternalPredTable(sorted);
            auto predtable1 = new PredTable(internpredtable1, structure->universe(*it));
            if(not isConsistentWith(predtable1, structure->inter(*it))){
            	xsb_interface->reset();
            	return false;
            }
            structure->inter(*it)->ctpt(predtable1);
			if(not structure->inter(*it)->isConsistent()){ // E.g. for functions
				xsb_interface->reset();
				return false;
			}
		}
		xsb_interface->reset();
		return structure->isConsistent();
	} else {
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

std::vector<AbstractStructure*> CalculateDefinitions::calculateKnownDefinitions(Theory* theory, AbstractStructure* structure) {
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
				auto definition = currentdefinition->first;
				auto hasrecursion = DefinitionUtils::hasRecursionOverNegation(definition);
				if(getOption(XSB) && hasrecursion) {
					Warning::warning("Currently, no support for definitions that have recursion over negation with XSB");
				}
				auto satisfiable = calculateDefinition(definition, structure, getOption(XSB) && not hasrecursion);
				if (not satisfiable) {
					if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
						clog << "The given structure is not a model of the definition.\n";
					}
					return std::vector<AbstractStructure*> { };
				}
				theory->remove(definition);
				definition->recursiveDelete();
				opens.erase(currentdefinition);
				fixpoint = false;
			}
		}
	}
	if (not structure->isConsistent()) {
		return std::vector<AbstractStructure*> { };
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Done calculating known definitions\n";
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 5) {
		clog << "Resulting structure:\n" << toString(structure) << "\n";
	}
	return {structure};
}

