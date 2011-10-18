/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MODELEXPAND_HPP_
#define MODELEXPAND_HPP_

#include <vector>
#include <string>
#include <iostream>
#include "commandinterface.hpp"
#include "monitors/tracemonitor.hpp"
#include "commands/propagate.hpp"
#include "symmetry.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

class ModelExpandInference: public Inference {
public:
	ModelExpandInference(): Inference("modelexpand", false, true) {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory()->clone();
		AbstractStructure* structure = args[1].structure()->clone();
		Options* options = args[2].options();

		auto models = doModelExpansion(theory, structure, options);

		// Convert to internal arguments
		InternalArgument result;
		result._type = AT_TABLE;
		result._value._table = new std::vector<InternalArgument>();
		for(auto it = models.begin(); it != models.end(); ++it) {
			result._value._table->push_back(InternalArgument(*it));
		}
		if(options->getValue(BoolType::TRACE)) {
			InternalArgument randt;
			randt._type = AT_MULT;
			randt._value._table = new std::vector<InternalArgument>(1,result);
			InternalArgument trace;
			trace._type = AT_REGISTRY;
			//trace._value._string = monitor->index(); // FIXME what does this value mean exactly?
			randt._value._table->push_back(trace);
			result = randt;
		}

		return result;
	}

	std::vector<AbstractStructure*> doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, Options* options) const {
		TraceMonitor* monitor = tracemonitor();

		// Calculate known definitions
		if(typeid(*theory) == typeid(Theory)) {
			bool satisfiable = calculateKnownDefinitions(dynamic_cast<Theory*>(theory),structure,options);
			if(not satisfiable) { return std::vector<AbstractStructure*>{}; }
		}

		// Symbolic propagation
		PropagateInference propinference;
		std::map<PFSymbol*,InitBoundType> mpi = propinference.propagateVocabulary(theory,structure);
		FOPropagator* propagator = propinference.createPropagator(theory,mpi,options);
		propagator->run();
		SymbolicStructure* symstructure = propagator->symbolicstructure();

		// Create solver and grounder
		SATSolver* solver = createsolver(options);
		GrounderFactory grounderfactory(structure,options,symstructure);
		TopLevelGrounder* grounder = grounderfactory.create(theory,solver);
		grounder->run();
		AbstractGroundTheory* grounding = grounder->grounding();
		
		// Execute symmetry breaking
		if(options->getValue(IntType::SYMMETRY)!=0) {
			std::cerr << "Symmetry detection...\n";
			clock_t start = clock();
			auto ivsets = findIVSets(theory, structure);
			float time = (float) (clock() - start)/CLOCKS_PER_SEC;
			std::cerr << "Symmetry detection finished in: " << time << "\n";
			if(options->getValue(IntType::SYMMETRY)==1) {
				std::cerr << "Adding symmetry breaking clauses...\n";
				addSymBreakingPredicates(grounding, ivsets);
			} else if(options->getValue(IntType::SYMMETRY)==2) {
				std::cerr << "Using symmetrical clause learning...\n";
				for(auto ivsets_it=ivsets.begin(); ivsets_it!=ivsets.end(); ++ivsets_it) {
					std::vector<std::map<int,int> > breakingSymmetries = (*ivsets_it)->getBreakingSymmetries(grounding);
					for(auto bs_it = breakingSymmetries.begin(); bs_it != breakingSymmetries.end(); ++bs_it) {
						MinisatID::Symmetry symmetry;
						for(auto s_it = bs_it->begin(); s_it!=bs_it->end(); ++s_it) {
							MinisatID::Atom a1 = MinisatID::Atom(s_it->first);
							MinisatID::Atom a2 = MinisatID::Atom(s_it->second);
							std::pair<MinisatID::Atom,MinisatID::Atom> entry = std::pair<MinisatID::Atom,MinisatID::Atom>(a1,a2);
							symmetry.symmetry.insert(entry);
						}
						solver->add(symmetry);
					}
				}
			} else {
				std::cerr << "Unknown symmetry option...\n";
			}
		}

		// Run solver
		MinisatID::Solution* abstractsolutions = initsolution(options);
		if(options->getValue(BoolType::TRACE)){
			monitor->setTranslator(grounding->translator());
			monitor->setSolver(solver);
		}
		solver->solve(abstractsolutions);

		// Collect solutions
		structure = propagator->currstructure(structure);
		std::vector<AbstractStructure*> solutions;
		for(auto model = abstractsolutions->getModels().begin();
			model != abstractsolutions->getModels().end(); ++model) {
			AbstractStructure* newsolution = structure->clone();
			addLiterals(*model,grounding->translator(),newsolution);
			addTerms(*model,grounding->termtranslator(),newsolution);
			newsolution->clean();
			solutions.push_back(newsolution);
		}

		grounding->recursiveDelete();
		delete(solver);
		delete(abstractsolutions);

		return solutions;
	}

private:
	bool calculateDefinition(Definition* definition, AbstractStructure* structure, Options* options) const {
		// Create solver and grounder
		SATSolver* solver = createsolver(options);
		GrounderFactory grounderfactory(structure,options);
		Theory theory("",structure->vocabulary(),ParseInfo()); theory.add(definition);
		TopLevelGrounder* grounder = grounderfactory.create(&theory,solver);

		grounder->run();
		AbstractGroundTheory* grounding = dynamic_cast<GroundTheory<SolverPolicy>*>(grounder->grounding());

		// Run solver
		MinisatID::Solution* abstractsolutions = initsolution(options);
		solver->solve(abstractsolutions);

		// Collect solutions
		if(abstractsolutions->getModels().empty()) {
			return false;
		} else {
			assert(abstractsolutions->getModels().size() == 1);
			auto model = *(abstractsolutions->getModels().begin());
			addLiterals(model,grounding->translator(),structure);
			addTerms(model,grounding->termtranslator(),structure);
			structure->clean();
		}

		// Cleanup
		grounding->recursiveDelete();
		delete(solver);
		delete(abstractsolutions);

		return true;
	}

	bool calculateKnownDefinitions(Theory* theory, AbstractStructure* structure, Options* options) const {
		// Collect the open symbols of all definitions
		std::map<Definition*,std::set<PFSymbol*> > opens;
		for(auto it = theory->definitions().begin(); it != theory->definitions().end(); ++it) { 
			opens[*it] = DefinitionUtils::opens(*it);
		}

		// Calculate the interpretation of the defined atoms from definitions that do not have
		// three-valued open symbols
		bool fixpoint = false;
		while(not fixpoint) {
			fixpoint = true;
			for(auto it = opens.begin(); it != opens.end(); ) {
				auto currentdefinition = it++;
				// Remove opens that have a two-valued interpretation
				for(auto symbol = currentdefinition->second.begin(); symbol != currentdefinition->second.end(); ) {
					auto currentsymbol = symbol++;
					if(structure->inter(*currentsymbol)->approxTwoValued()) {
						currentdefinition->second.erase(currentsymbol);
					}
				}
				// If no opens are left, calculate the interpretation of the defined atoms
				if(currentdefinition->second.empty()) {
					bool satisfiable = calculateDefinition(currentdefinition->first,structure,options);
					if(not satisfiable) { return false; }
					opens.erase(currentdefinition);
					theory->remove(currentdefinition->first);
					fixpoint = false;
				}
			}
		} 
		return true;
	}

	SATSolver* createsolver(Options* options) const {
		MinisatID::SolverOption modes;
		modes.nbmodels = options->getValue(IntType::NRMODELS);
		modes.verbosity = options->getValue(IntType::SATVERBOSITY);
		modes.polarity = options->getValue(BoolType::MXRANDOMPOLARITYCHOICE)?MinisatID::POL_RAND:MinisatID::POL_STORED;

		if(options->getValue(BoolType::GROUNDLAZILY)){
			modes.lazy = true;
		}

//		modes.remap = false; // FIXME no longer allowed, because solver needs the remapping for extra literals.
		return new SATSolver(modes);
	}

	MinisatID::Solution* initsolution(Options* options) const {
		MinisatID::ModelExpandOptions opts;
		opts.nbmodelstofind = options->getValue(IntType::NRMODELS);
		opts.printmodels = MinisatID::PRINT_NONE;
		opts.savemodels = MinisatID::SAVE_ALL;
		opts.search = MinisatID::MODELEXPAND;
		return new MinisatID::Solution(opts);
	}

	void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) const {
		for(auto literal = model->literalinterpretations.begin();
			literal != model->literalinterpretations.end(); ++literal) {
			int atomnr = literal->getAtom().getValue();

			if(translator->isInputAtom(atomnr)) {
				PFSymbol* symbol = translator->getSymbol(atomnr);
				const ElementTuple& args = translator->getArgs(atomnr);
				if(typeid(*symbol) == typeid(Predicate)) {
					Predicate* pred = dynamic_cast<Predicate*>(symbol);
					if(literal->hasSign()) { init->inter(pred)->makeFalse(args); }
					else { init->inter(pred)->makeTrue(args); }
				}
				else {
					Function* func = dynamic_cast<Function*>(symbol);
					if(literal->hasSign()) { init->inter(func)->graphInter()->makeFalse(args); }
					else { init->inter(func)->graphInter()->makeTrue(args); }
				}
			}
		}
	}

	void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init) const {
		for(auto cpvar = model->variableassignments.begin(); cpvar != model->variableassignments.end(); ++cpvar) {
			Function* function = termtranslator->function(cpvar->variable);
			if(function==NULL){
				continue;
			}
			const auto& gtuple = termtranslator->args(cpvar->variable);
			ElementTuple tuple;
			for(auto it = gtuple.begin(); it != gtuple.end(); ++it) {
				if(it->_isvarid) {
					int value = model->variableassignments[it->_varid].value;
					tuple.push_back(DomainElementFactory::instance()->create(value));
				} else {
					tuple.push_back(it->_domelement);
				}
			}
			tuple.push_back(DomainElementFactory::instance()->create(cpvar->value));
			init->inter(function)->graphInter()->makeTrue(tuple);
		}
	}
};

#endif
