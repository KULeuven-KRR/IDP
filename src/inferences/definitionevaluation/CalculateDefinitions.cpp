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
#include "structure/StructureComponents.hpp"

#include "groundtheories/SolverTheory.hpp"

#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/LazyGroundingManager.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

#ifdef WITHXSB
#include "inferences/querying/xsb/XSBInterface.hpp"
#endif

#include "options.hpp"
#include <iostream>

using namespace std;

CalculateDefinitions::CalculateDefinitions(Theory* t, Structure* s, bool satdelay, std::set<PFSymbol*> symbolsToQuery) :
	_theory(t), _structure(s), _symbolsToQuery(symbolsToQuery), _satdelay(satdelay), _tooExpensive(false) {
	if (_theory == NULL || _structure == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	if (_structure->vocabulary() != _theory->vocabulary()) {
		throw IdpException("Definition Evaluation requires that the theory and structure range over the same vocabulary.");
	}
}


// IMPORTANT: becomes owner of new theory and cloned definition!
CalculateDefinitions::CalculateDefinitions(const Definition* definition,
		Structure* structure, bool satdelay, std::set<PFSymbol*> symbolsToQuery) :
	_structure(structure), _symbolsToQuery(symbolsToQuery), _satdelay(satdelay), _tooExpensive(false) {
	auto theory = new Theory("wrapper_theory", structure->vocabulary(), ParseInfo());
	auto newdef = definition->clone();
	theory->add(newdef);
	_theory = theory;
}

DefinitionCalculationResult CalculateDefinitions::doCalculateDefinitions(
	const Definition* definition, Structure* structure, bool satdelay, std::set<PFSymbol *> symbolsToQuery) {
	CalculateDefinitions c(definition, structure, satdelay, symbolsToQuery);
	auto ret = c.calculateKnownDefinitions();
	c._theory->recursiveDelete();
	return ret;
}


DefinitionCalculationResult CalculateDefinitions::calculateDefinition(const Definition* definition) {
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
		clog << "Calculating definition: " << toString(definition) << "\n";
	}
	DefinitionCalculationResult result(_structure);
	result._hasModel = true;
#ifdef WITHXSB
	auto withxsb = CalculateDefinitions::determineXSBUsage(definition);
	if (withxsb) {
		if(_satdelay or getOption(SATISFIABILITYDELAY)) { // TODO implement checking threshold by size estimation (see issue #850)
			Warning::warning("Lazy threshold is not checked for definitions evaluated with XSB");
		}
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
			clog << "Calculating the above definition using XSB\n";
		}
		auto xsb_interface = XSBInterface::instance();
		xsb_interface->load(definition,_structure);
		auto symbols = definition->defsymbols();
		if(not _symbolsToQuery.empty()) {
			for(auto it = symbols.begin(); it != symbols.end();) {
				auto symbol = *(it++);
				if(_symbolsToQuery.find(symbol) == _symbolsToQuery.end()) {
					symbols.erase(symbol);
				}
			}
		}
		if (definitionDoesNotResultInTwovaluedModel(definition)) {
			result._hasModel=false;
			return result;
		}
		for (auto symbol : symbols) {
			auto sorted = xsb_interface->queryDefinition(symbol);
            auto internpredtable1 = new EnumeratedInternalPredTable(sorted);
            auto predtable1 = new PredTable(internpredtable1, _structure->universe(symbol));
            if(not isConsistentWith(predtable1, _structure->inter(symbol))){
            	delete(predtable1);
            	xsb_interface->reset();
            	result._hasModel=false;
            	return result;
            }
            _structure->inter(symbol)->ctpt(predtable1);
        	delete(predtable1);
            _structure->clean();
            if(isa<Function>(*symbol)) {
            	auto fun = dynamic_cast<Function*>(symbol);
            	if(not _structure->inter(fun)->approxTwoValued()){ // E.g. for functions
                	xsb_interface->reset();
                	result._hasModel=false;
                	return result;
    			}
            }
			if(not _structure->inter(symbol)->isConsistent()){
            	xsb_interface->reset();
            	result._hasModel=false;
            	return result;
			}
		}

    	xsb_interface->reset();
		if (not _structure->isConsistent()) {
        	result._hasModel=false;
        	return result;
		} else {
	    	result._hasModel=true;
		}
    	return result;
	}
#endif
	// Default: Evaluation using ground-and-solve
	auto data = SolverConnection::createsolver(1);
	auto theory = new Theory("", _structure->vocabulary(), ParseInfo());
	theory->add(definition->clone());
	bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
	bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS);
	auto symstructure = generateBounds(theory, _structure, propagate, LUP);
	auto grounder = GrounderFactory::create(GroundInfo(theory, { _structure, symstructure }, NULL, true), data);

	auto size = toDouble(grounder->getMaxGroundSize());
	size = size < 1 ? 1 : size;
	if ((_satdelay or getOption(SATISFIABILITYDELAY)) and log(size) / log(2) > 2 * getOption(LAZYSIZETHRESHOLD)) {
		      _tooExpensive = true;
		delete (data);
		delete (grounder);
    	result._hasModel=true;
    	theory->recursiveDelete();
		return result;
	}

	bool unsat = grounder->toplevelRun();

	//It's possible that unsat is found (for example when we have a conflict with function constraints)
	if (unsat) {
		// Cleanup
		delete (data);
		delete (grounder);
    	result._hasModel=false;
    	theory->recursiveDelete();
		return result;
	}

	Assert(not unsat);
	AbstractGroundTheory* grounding = dynamic_cast<SolverTheory*>(grounder->getGrounding());

	// Run solver
	data->finishParsing();
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
		SolverConnection::addLiterals(*model, grounding->translator(), _structure);
		SolverConnection::addTerms(*model, grounding->translator(), _structure);
		      _structure->clean();
	}
	for (auto symbol : definition->defsymbols()) {
		if (isa<Function>(*symbol)) {
			auto fun = dynamic_cast<Function*>(symbol);
			if (not _structure->inter(fun)->approxTwoValued()) { // Check for functions that are defined badly
				result._hasModel = false;
			}
		}
	}

	// Cleanup
	grounding->recursiveDelete();
	theory->recursiveDelete();
	delete (data);
	delete (mx);
	delete (grounder);

	result._hasModel &= (not abstractsolutions.empty() && _structure->isConsistent());
	return result;
}

DefinitionCalculationResult CalculateDefinitions::calculateKnownDefinitions() {
	if(_theory->definitions().empty()){
		DefinitionCalculationResult result(_structure);
		result._hasModel = true;
		return result;
	}

	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Calculating known definitions\n";
	}

#ifdef WITHXSB
	if (getOption(XSB)) {
		DefinitionUtils::joinDefinitionsForXSB(_theory, _structure);
	}
#endif

	   _theory = FormulaUtils::improveTheoryForInference(_theory, _structure, false, false);
	if (not _symbolsToQuery.empty()) {
		updateSymbolsToQuery(_symbolsToQuery, _theory->definitions());
	}

	// Collect the open symbols of all definitions
	auto opens = DefinitionUtils::opens(_theory->definitions());

	if (getOption(BoolType::STABLESEMANTICS)) {
		CalculateDefinitions::removeNonTotalDefnitions(opens);
	}

	DefinitionCalculationResult result(_structure);
	result._hasModel = true;

	// Calculate the interpretation of the defined atoms from definitions that do not have
	// three-valued open symbols
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto it = opens.begin(); it != opens.end();) {
			auto currentdefinition = it++; // REASON: set erasure does only invalidate iterators pointing to the erased elements

			// Remove opens that have a two-valued interpretation
			auto toRemove = DefinitionUtils::approxTwoValuedOpens(currentdefinition->first, _structure);
			for (auto symbol : toRemove) {
				if (currentdefinition->second.find(symbol) != currentdefinition->second.end()) {
					currentdefinition->second.erase(symbol);
				}
			}

			// If no opens are left, calculate the interpretation of the defined atoms
			if (currentdefinition->second.empty()) {
				auto definition = currentdefinition->first;

				if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
					clog << "Evaluating " << toString(currentdefinition->first) << "\n";
					clog << "Using structure " << toString(_structure) << "\n";
				}
				bool tooexpensive = false;
				auto defCalcResult = calculateDefinition(definition);
				if (tooexpensive) {
					continue;
				}
				result._calculated_model = defCalcResult._calculated_model; // Update current structure

				if (not defCalcResult._hasModel) { // If the definition did not have a model, quit execution (don't set fixpoint to false)
					if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
						clog << "The given structure is not a model of the definition\n" << toString(definition) << "\n";
					}
					result._hasModel = false;
				} else { // If it did have a model, update result and continue
					fixpoint = false;
					opens.erase(currentdefinition);
					               _theory->remove(definition);
					definition->recursiveDelete();
				}
			}
		}
	}
	if (not result._hasModel or not result._calculated_model->isConsistent()) {
		// When returning a result that has no model, the other arguments are as follows:
		// _calculated_model:  the structure resulting from the last unsuccessful definition calculation
		// _calculated_definitions: all definitions that have been successfully calculated
		result._hasModel = false;
		return result;
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Done calculating known definitions\n";
		clog << "Resulting structure:\n" << toString(_structure) << "\n";
	}
	result._hasModel = true;
	return result;
}


void CalculateDefinitions::removeNonTotalDefnitions(std::map<Definition*,
		std::set<PFSymbol*> >& opens) {
	bool foundone = false;
	auto def = opens.begin();
	while (def != opens.end()) {
		auto hasrecursion = DefinitionUtils::approxHasRecursionOverNegation((*def).first);
		//TODO in the future: put a smarter check here

		auto currentdefinition = def++;
		// REASON: set erasure does only invalidate iterators pointing to the erased elements

		if (hasrecursion) {
			foundone = true;
			opens.erase(currentdefinition);
		}
	}
	if (foundone) {
		Warning::warning("Ignoring definitions for which we cannot detect totality because option stablesemantics is true.");
	}
}

void CalculateDefinitions::updateSymbolsToQuery(std::set<PFSymbol*>& symbols, std::vector<Definition*> defs) const {
	std::set<PFSymbol*> opens;
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto def : defs) {
			for (auto symbol : symbols) {
				if (def->defsymbols().find(symbol) != def->defsymbols().end()) {
					auto dependenciesOfSymbol = DefinitionUtils::getDirectDependencies(def,symbol);
					for (auto dependency : dependenciesOfSymbol) {
						if (dependency == symbol) { continue; }
						for (auto def2 : defs) {
							if (def2 == def) {
								continue;
							}
							if (def2->defsymbols().find(dependency) != def2->defsymbols().end()) {
								// There is an "external" definition that defines the required
								// dependency, so it must also be queried (and inserted into IDP)
								if (symbols.insert(dependency).second) { // .second return value indicates whether it was a "new" element in the set
									fixpoint = false;
								}
							}
						}
					}
				}
			}
		}
	}
}

std::set<PFSymbol *> CalculateDefinitions::determineInputStarSymbols(Theory* t) const {
	set<PFSymbol*> inputstar;
	bool fixpoint = false;
	while(not fixpoint) {
		fixpoint = true;
		for (auto def : t->definitions()) {
			auto oldsize = inputstar.size();
			addNewInputStar(def,inputstar);
			if (oldsize <  inputstar.size()) {
				fixpoint = false;
			}
		}
	}
	return inputstar;
}


void CalculateDefinitions::addNewInputStar(const Definition* d, std::set<PFSymbol*>& inputstar) const {
	addAll(inputstar, DefinitionUtils::approxTwoValuedOpens(d,_structure));
	set<PFSymbol*> potentialNewSymbols = DefinitionUtils::defined(d);
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto it = potentialNewSymbols.begin(); it != potentialNewSymbols.end();) {
			auto defsymbol = *(it++); // potential set erasure
			auto deps = DefinitionUtils::getDirectDependencies(d, defsymbol);
			if (isSubset(deps,inputstar)) { // i.e., all dependencies are inputstar
				inputstar.insert(defsymbol);
				fixpoint = false;
				potentialNewSymbols.erase(defsymbol);
			}
		}
	}
}

bool CalculateDefinitions::definitionDoesNotResultInTwovaluedModel(const Definition* definition) const {
	auto possRecNegSymbols = DefinitionUtils::approxRecurionsOverNegationSymbols(definition);
	auto xsb_interface = XSBInterface::instance();
	for (auto symbol : possRecNegSymbols) {
		if(xsb_interface->hasUnknowns(symbol)) {
			xsb_interface->reset();
			return true;
		}
	}
	return false;
}


#ifdef WITHXSB
bool CalculateDefinitions::determineXSBUsage(const Definition* definition) {

	auto has_recursive_aggregate = DefinitionUtils::approxContainsRecDefAggTerms(definition);
	if(getOption(XSB) && has_recursive_aggregate) {
		Warning::warning("Currently, no support for definitions that have recursive aggregates");
	}

	return getOption(XSB) && not has_recursive_aggregate;
}
#endif

