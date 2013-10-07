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
#include "inferences/grounding/LazyGroundingManager.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

#ifdef WITHXSB
#include "inferences/querying/xsb/xsbinterface.hpp"
#endif

#include "options.hpp"
#include <iostream>

using namespace std;
bool CalculateDefinitions::calculateDefinition(Definition* definition, Structure* structure,
		bool satdelay, bool& tooExpensive, bool withxsb, std::set<PFSymbol*> symbolsToQuery) const {
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
		clog << "Calculating definition: " << toString(definition) << "\n";
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 5) {
		clog << "based on structure: " << toString(structure) << "\n\n"
						"and vocabulary: " << toString(structure->vocabulary()) << "\n";
	}
#ifdef WITHXSB
	if (withxsb) {
		if(satdelay or getOption(SATISFIABILITYDELAY)) { // TODO implement checking threshold by size estimation
			Warning::warning("Lazy threshold is not checked for definitions evaluated with XSB");
		}
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
			clog << "Calculating the above definition using XSB\n";
		}
		auto xsb_interface = XSBInterface::instance();
		xsb_interface->setStructure(structure);

		xsb_interface->loadDefinition(definition);
		auto symbols = definition->defsymbols();
		if(not symbolsToQuery.empty()) {
			for(auto it = symbols.begin(); it != symbols.end();) {
				auto symbol = *(it++);
				if(symbolsToQuery.find(symbol) == symbolsToQuery.end()) {
					symbols.erase(symbol);
				}
			}
		}
		for (auto symbol : symbols) {
			auto sorted = xsb_interface->queryDefinition(symbol);
            auto internpredtable1 = new EnumeratedInternalPredTable(sorted);
            auto predtable1 = new PredTable(internpredtable1, structure->universe(symbol));
            if(not isConsistentWith(predtable1, structure->inter(symbol))){
            	xsb_interface->reset();
            	return false;
            }
            structure->inter(symbol)->ctpt(predtable1);
            structure->clean();
            if(isa<Function>(*symbol)) {
            	auto fun = dynamic_cast<Function*>(symbol);
            	if(not structure->inter(fun)->approxTwoValued()){ // E.g. for functions
    				xsb_interface->reset();
    				return false;
    			}
            }
			if(not structure->inter(symbol)->isConsistent()){
				xsb_interface->reset();
				return false;
			}
		}
		xsb_interface->reset();
		return structure->isConsistent();
	}
#endif
	// Default: Evaluation using ground-and-solve
	auto data = SolverConnection::createsolver(1);
	Theory theory("", structure->vocabulary(), ParseInfo());
	theory.add(definition);
	bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
	bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS);
	auto symstructure = generateBounds(&theory, structure, propagate, LUP);
	auto grounder = GrounderFactory::create(GroundInfo(&theory, { structure, symstructure }, NULL, true), data);

	auto size = toDouble(grounder->getMaxGroundSize());
	size = size < 1 ? 1 : size;
	if ((satdelay or getOption(SATISFIABILITYDELAY)) and log(size) / log(2) > 2 * getOption(LAZYSIZETHRESHOLD)) {
		tooExpensive = true;
		delete (data);
		delete (grounder);
		return true;
	}

	bool unsat = grounder->toplevelRun();

	//It's possible that unsat is found (for example when we have a conflict with function constraints)
	if (unsat) {
		// Cleanup
		delete (data);
		delete (grounder);
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
	if (not abstractsolutions.empty()) {
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

	return not abstractsolutions.empty() && structure->isConsistent();
}

std::vector<Structure*> CalculateDefinitions::calculateKnownDefinitions(Theory* theory, Structure* structure,
		bool satdelay, std::set<PFSymbol*> symbolsToQuery) const {
	if (theory == NULL || structure == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	if (structure->vocabulary() != theory->vocabulary()) {
		throw IdpException("Definition Evaluation requires that the theory and structure range over the same vocabulary.");
	}

	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Calculating known definitions\n";
	}

	// Collect the open symbols of all definitions
	std::map<Definition*, std::set<PFSymbol*> > opens;
	for (auto it = theory->definitions().cbegin(); it != theory->definitions().cend(); ++it) {
		opens[*it] = DefinitionUtils::opens(*it);
	}
	if (getOption(BoolType::STABLESEMANTICS)) {
		bool foundone = false;
		auto def = opens.begin();
		while (def != opens.end()) {
			auto hasrecursion = DefinitionUtils::hasRecursionOverNegation((*def).first);
			//TODO in the future: put a smarter check here

			auto currentdefinition = def++;
			// REASON: set erasure does only invalidate iterators pointing to the erased elements
			// Remove opens that have a two-valued interpretation

			if (hasrecursion) {
				foundone = true;
				opens.erase(currentdefinition);
			}
		}
		if (foundone) {
			Warning::warning("Ignoring definitions for which we cannot detect totality because option stablesemantics is true.");
		}
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

				auto useXSB = getOption(XSB);
				if(getOption(XSB) && getOption(STABLESEMANTICS)) {
					Warning::warning("Cannot calculate definitions using XSB for the Stable Model Semantics");
					useXSB = false;
				}
				if(getOption(XSB) && hasrecursion) {
					Warning::warning("Currently, no support for definitions that have recursion over negation with XSB");
					useXSB = false;
				}

				bool tooexpensive = false;
				if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
					clog << "Evaluating " << toString(currentdefinition->first) << "\n";
				}
				auto has_recursive_aggregate = DefinitionUtils::hasRecursiveAggregate(definition);
				if(getOption(XSB) && has_recursive_aggregate) {
					Warning::warning("Currently, no support for definitions that have recursive aggregates");
					useXSB = false;
				}

				bool satisfiable = calculateDefinition(definition, structure, satdelay, tooexpensive, getOption(XSB) && not hasrecursion, symbolsToQuery);
				if (tooexpensive) {
					continue;
				}
				if (not satisfiable) {
					if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
						clog << "The given structure is not a model of the definition.\n";
					}
					return std::vector<Structure*> { };
				}
				opens.erase(currentdefinition);
				theory->remove(currentdefinition->first);
				fixpoint = false;
			}
		}
	}
	if (not structure->isConsistent()) {
		return std::vector<Structure*> { };
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Done calculating known definitions\n";
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 4) {
		clog << "Resulting structure:\n" << toString(structure) << "\n";
	}
	return {structure};
}

std::vector<Structure*> CalculateDefinitions::calculateKnownDefinition(
		Definition* definition, Structure* structure,
		std::set<PFSymbol*> symbolsToQuery) {
	Theory* theory = new Theory("wrapper_theory", structure->vocabulary(), ParseInfo());
	theory->add(definition);
	return calculateKnownDefinitions(theory,structure,symbolsToQuery);
}
