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

		// Create solver and grounder
		SATSolver* solver = createsolver(options);
		GrounderFactory grounderfactory(structure,options);
		TopLevelGrounder* grounder = grounderfactory.create(theory,solver);

		// Run grounder
		grounder->run();
		SolverTheory* grounding = dynamic_cast<SolverTheory*>(grounder->grounding());
		
		// Execute symmetry breaking
		if(options->symmetry()!=0){
			//Break symmetry
			std::vector<const IVSet*> ivsets = findIVSets(theory, structure);
			if(options->symmetry()==1){
				/** method 1 **/
				addSymBreakingPredicates(grounding, ivsets);
			}else if(options->symmetry()==2){
				/** method 2 **/
				for(std::vector<const IVSet*>::const_iterator ivsets_it=ivsets.begin(); ivsets_it!=ivsets.end(); ++ivsets_it){
					std::vector<std::list<int> > symmetries = (*ivsets_it)->getInterchangeableLiterals(grounding);
					
					std::vector<std::vector<MinisatID::Literal> > satLiterals;
					for(std::vector<std::list<int> >::const_iterator symmetries_it=symmetries.begin(); symmetries_it!=symmetries.end(); ++symmetries_it){
						std::vector<MinisatID::Literal> satLiteral;
						for(std::list<int>::const_iterator symmetries_it2=symmetries_it->begin(); symmetries_it2!=symmetries_it->end(); ++symmetries_it2){
							satLiteral.push_back(MinisatID::Literal(*symmetries_it2,false));
						}
						satLiterals.push_back(satLiteral);
					}
					MinisatID::SymmetryLiterals temp;
					temp.symmgroups=satLiterals;
					solver->add(temp);
				}
			}else if(options->symmetry()==3){
				/** method 3 **/
				for(std::vector<const IVSet*>::const_iterator ivsets_it=ivsets.begin(); ivsets_it!=ivsets.end(); ++ivsets_it){
					std::vector<std::map<int,int> > literalsSymmetries = (*ivsets_it)->getLiteralsSymmetries(grounding);
					for(std::vector<std::map<int,int> >::const_iterator ls_it = literalsSymmetries.begin(); ls_it != literalsSymmetries.end(); ++ls_it){
						MinisatID::Symmetry symmetry;
						for(std::map<int,int>::const_iterator s_it = ls_it->begin(); s_it!=ls_it->end(); ++s_it){
							MinisatID::Atom a1 = MinisatID::Atom(s_it->first);
							MinisatID::Atom a2 = MinisatID::Atom(s_it->second);
							std::pair<MinisatID::Atom,MinisatID::Atom> entry = std::pair<MinisatID::Atom,MinisatID::Atom>(a1,a2);
							symmetry.symmetry.insert(entry);
						}
						solver->add(symmetry);
					}
				}
			}
		}
		
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
		std::cerr << '}' << "\n";
	}
};

#endif
