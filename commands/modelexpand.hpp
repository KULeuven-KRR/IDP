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

class ModelExpandInference: public Inference {
public:
	ModelExpandInference(): Inference("mx", false, true) {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory()->clone();
		AbstractStructure* structure = args[1].structure()->clone();
		Options* options = args[2].options();
		TraceMonitor* monitor = tracemonitor();

		// Calculate known definitions
		if(typeid(*theory) == typeid(Theory)) {
			bool satisfiable = calculateKnownDefinitions(dynamic_cast<Theory*>(theory),structure,options);
			if(!satisfiable) return unsat();
		}

		// Symbolic propagation
		PropagateInference propinference;
		std::map<PFSymbol*,InitBoundType> mpi = propinference.propagatevocabulary(theory,structure);
		FOPropagator* propagator = propinference.createpropagator(theory,mpi,options);
		propagator->run();

		// Create solver and grounder
		SATSolver* solver = createsolver(options);
		GrounderFactory grounderfactory(structure,options);
		TopLevelGrounder* grounder = grounderfactory.create(theory,solver);

		// Run grounder
		grounder->run();
		SolverTheory* grounding = dynamic_cast<SolverTheory*>(grounder->grounding());

		// Add information that is abstracted in the grounding
		grounding->addFuncConstraints();
		grounding->addFalseDefineds();

		// Run solver
		MinisatID::Solution* abstractsolutions = initsolution(options);
		if(options->trace()){
			monitor->setTranslator(grounding->translator());
			monitor->setSolver(solver);
		}
		solver->solve(abstractsolutions);

		// Collect solutions
		std::vector<AbstractStructure*> solutions;
		for(auto model = abstractsolutions->getModels().begin();
			model != abstractsolutions->getModels().end(); ++model) {
			AbstractStructure* newsolution = structure->clone();
			addLiterals(*model,grounding->translator(),newsolution);
			addTerms(*model,grounding->termtranslator(),newsolution);
			newsolution->clean();
			solutions.push_back(newsolution);
		}

		// Convert to internal arguments
		InternalArgument result;
		result._type = AT_TABLE;
		result._value._table = new std::vector<InternalArgument>();
		for(auto it = solutions.begin(); it != solutions.end(); ++it) {
			result._value._table->push_back(InternalArgument(*it));
		}
		if(options->trace()) {
			InternalArgument randt;
			randt._type = AT_MULT;
			randt._value._table = new std::vector<InternalArgument>(1,result);
			InternalArgument trace;
			trace._type = AT_REGISTRY;
			trace._value._string = monitor->index();
			randt._value._table->push_back(trace);
			result = randt;
		}

		// Cleanup
		grounding->recursiveDelete();
		delete(solver);
		delete(abstractsolutions);

		return result;
	}

private:

	InternalArgument unsat() const {
		// Return an empty table of solutions
		InternalArgument result;
		result._type = AT_TABLE;
		result._value._table = new std::vector<InternalArgument>();
		return result;
	}

	bool calculateDefinition(Definition* definition, AbstractStructure* structure, Options* options) const {
		// Create solver and grounder
		SATSolver* solver = createsolver(options);
		GrounderFactory grounderfactory(structure,options);
		Theory theory("",structure->vocabulary(),ParseInfo()); theory.add(definition);
		TopLevelGrounder* grounder = grounderfactory.create(&theory,solver);

		// Run grounder
		grounder->run();
		SolverTheory* grounding = dynamic_cast<SolverTheory*>(grounder->grounding());

		// Add information that is abstracted in the grounding
		grounding->addFuncConstraints();
		grounding->addFalseDefineds();

		// Run solver
		MinisatID::Solution* abstractsolutions = initsolution(options);
		solver->solve(abstractsolutions);

		// Collect solutions
		if(abstractsolutions->getModels().empty()) return false;
		else {
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
		for(auto it = theory->definitions().begin(); it != theory->definitions().end(); ++it) 
			opens[*it] = DefinitionUtils::opens(*it);

		// Calculate the interpretation of the defined atoms from definitions that do not have
		// three-valued open symbols
		bool fixpoint = false;
		while(!fixpoint) {
			fixpoint = true;
			for(auto it = opens.begin(); it != opens.end(); ) {
				auto currentdefinition = it++;
				// Remove opens that have a two-valued interpretation
				for(auto symbol = currentdefinition->second.begin(); symbol != currentdefinition->second.end(); ) {
					auto currentsymbol = symbol++;
					if(structure->inter(*currentsymbol)->approxtwovalued()) {
						currentdefinition->second.erase(currentsymbol);
					}
				}
				// If no opens are left, calculate the interpretation of the defined atoms
				if(currentdefinition->second.empty()) {
					bool satisfiable = calculateDefinition(currentdefinition->first,structure,options);
					if(!satisfiable) return false;
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
		modes.nbmodels = options->nrmodels();
		modes.verbosity = options->satverbosity();
		modes.remap = false;
		return new SATSolver(modes);
	}

	MinisatID::Solution* initsolution(Options* options) const {
		MinisatID::ModelExpandOptions opts;
		opts.nbmodelstofind = options->nrmodels();
		opts.printmodels = MinisatID::PRINT_NONE;
		opts.savemodels = MinisatID::SAVE_ALL;
		opts.search = MinisatID::MODELEXPAND;
		return new MinisatID::Solution(opts);
	}

	void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) const {
		for(auto literal = model->literalinterpretations.begin();
			literal != model->literalinterpretations.end(); ++literal) {
			int atomnr = literal->getAtom().getValue();
			PFSymbol* symbol = translator->symbol(atomnr);
			if(symbol) {
				const ElementTuple& args = translator->args(atomnr);
				if(typeid(*symbol) == typeid(Predicate)) {
					Predicate* pred = dynamic_cast<Predicate*>(symbol);
					if(literal->hasSign()) init->inter(pred)->makeFalse(args);
					else init->inter(pred)->makeTrue(args);
				}
				else {
					Function* func = dynamic_cast<Function*>(symbol);
					if(literal->hasSign()) init->inter(func)->graphinter()->makeFalse(args);
					else init->inter(func)->graphinter()->makeTrue(args);
				}
			}
		}
	}

	void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init) const {
//		std::cerr << "Adding terms based on var-val pairs from CP solver, pairs are { ";
		for(auto cpvar = model->variableassignments.begin(); cpvar != model->variableassignments.end(); ++cpvar) {
//			std::cerr << cpvar->variable << '=' << cpvar->value;
			Function* function = termtranslator->function(cpvar->variable);
			if(function) {
				const std::vector<GroundTerm>& gtuple = termtranslator->args(cpvar->variable);
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
//				std::cerr << '=' << function->name() << tuple;
				init->inter(function)->graphinter()->makeTrue(tuple);
			}
//			std::cerr << ' ';
		}
//		std::cerr << '}' << "\n";
	}
};

#endif
