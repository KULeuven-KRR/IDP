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

class ModelExpandInference: public Inference {
public:
	ModelExpandInference(): Inference("mx") {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
		add(AT_TRACEMONITOR); // FIXME do not add always: create second inference
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory();
		AbstractStructure* structure = args[1].structure();
		Options* options = args[2].options();
		TraceMonitor* monitor = args[3]._value.tracemonitor_;

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
		delete(monitor);

		return result;
	}

private:
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
		std::cerr << "Adding terms based on var-val pairs from CP solver, pairs are { ";
		for(auto cpvar = model->variableassignments.begin(); cpvar != model->variableassignments.end(); ++cpvar) {
			std::cerr << cpvar->variable << '=' << cpvar->value;
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
				std::cerr << '=' << function->name() << tuple;
				init->inter(function)->graphinter()->makeTrue(tuple);
			}
			std::cerr << ' ';
		}
		std::cerr << '}' << std::endl;
	}
};

#endif
