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
#include "symmetry.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

class ModelExpandInference: public Inference {
public:
	ModelExpandInference(): Inference("mx", false, true) {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory();
		AbstractStructure* structure = args[1].structure();
		Options* options = args[2].options();
		TraceMonitor* monitor = tracemonitor();

		// Create solver and grounder and ground the theory
		SATSolver* solver = createsolver(options);
		GrounderFactory grounderfactory(structure,options);
		TopLevelGrounder* grounder = grounderfactory.create(theory,solver);
		grounder->run();
		AbstractGroundTheory* grounding = grounder->grounding();
		
		// Execute symmetry breaking
		if(options->getValue(IntType::SYMMETRY)!=0){
			std::cerr << "Symmetry detection...\n";
			clock_t start = clock();
			auto ivsets = findIVSets(theory, structure);
			float time = (float) (clock() - start)/CLOCKS_PER_SEC;
			std::cerr << "Symmetry detection finished in: " << time << "\n";
			if(options->getValue(IntType::SYMMETRY)==1){
				std::cerr << "Adding symmetry breaking clauses...\n";
				addSymBreakingPredicates(grounding, ivsets);
			}else if(options->getValue(IntType::SYMMETRY)==2){
				std::cerr << "Using symmetrical clause learning...\n";
				for(auto ivsets_it=ivsets.begin(); ivsets_it!=ivsets.end(); ++ivsets_it){
					std::vector<std::map<int,int> > breakingSymmetries = (*ivsets_it)->getBreakingSymmetries(grounding);
					for(auto bs_it = breakingSymmetries.begin(); bs_it != breakingSymmetries.end(); ++bs_it){
						MinisatID::Symmetry symmetry;
						for(auto s_it = bs_it->begin(); s_it!=bs_it->end(); ++s_it){
							MinisatID::Atom a1 = MinisatID::Atom(s_it->first);
							MinisatID::Atom a2 = MinisatID::Atom(s_it->second);
							std::pair<MinisatID::Atom,MinisatID::Atom> entry = std::pair<MinisatID::Atom,MinisatID::Atom>(a1,a2);
							symmetry.symmetry.insert(entry);
						}
						solver->add(symmetry);
					}
				}
			}else{
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
		if(options->getValue(BoolType::TRACE)) {
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
	SATSolver* createsolver(Options* options) const {
		MinisatID::SolverOption modes;
		modes.nbmodels = options->getValue(IntType::NRMODELS);
		modes.verbosity = options->getValue(IntType::SATVERBOSITY);
		modes.polarity = options->getValue(BoolType::MXRANDOMPOLARITYCHOICE)?MinisatID::POL_RAND:MinisatID::POL_STORED;

		if(options->getValue(BoolType::GROUNDLAZILY)){
			modes.lazy = true;
		}

//		modes.remap = false;
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
