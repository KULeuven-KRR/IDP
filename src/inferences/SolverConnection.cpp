/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "SolverConnection.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"

using namespace std;

namespace SolverConnection {

typedef cb::Callback1<std::string, int> callbackprinting;

PCSolver* createsolver(int nbmodels) {
	auto options = GlobalData::instance()->getOptions();
	MinisatID::SolverOption modes;
	modes.nbmodels = nbmodels;
	modes.verbosity = options->getValue(IntType::SATVERBOSITY);

	modes.randomseed = getOption(IntType::RANDOMSEED);

	modes.polarity = MinisatID::Polarity::STORED;
	if(getOption(BoolType::MXRANDOMPOLARITYCHOICE)){
		modes.polarity = MinisatID::Polarity::RAND;
	}

	if (options->getValue(BoolType::GROUNDLAZILY)) {
		modes.lazy = true;
	}

	auto solver = new PCSolver(modes);
	//solver->resetTerminationFlag(); // NOTE: have to tell the solver to reset its instance
	//CHECKTERMINATION
	return solver;
}

void setTranslator(PCSolver* solver, GroundTranslator* translator){
	callbackprinting cbprint(translator, &GroundTranslator::print);
	solver->setCallBackTranslator(cbprint);
}

PCModelExpand* initsolution(PCSolver* solver, int nbmodels) {
	auto options = GlobalData::instance()->getOptions();
	MinisatID::ModelExpandOptions opts;
	opts.nbmodelstofind = nbmodels;
	opts.printmodels = MinisatID::Models::NONE;
	opts.savemodels = MinisatID::Models::ALL;
	opts.inference = MinisatID::Inference::MODELEXPAND;
	return new PCModelExpand(solver, opts);
}

PCModelExpand* initpropsolution(PCSolver* solver, int nbmodels) {
	auto options = GlobalData::instance()->getOptions();
	MinisatID::ModelExpandOptions opts;
	opts.nbmodelstofind = nbmodels;
	opts.printmodels = MinisatID::Models::NONE;
	opts.savemodels = MinisatID::Models::ALL;
	opts.inference = MinisatID::Inference::PROPAGATE; // TODO should become propagate inference!
	return new PCModelExpand(solver, opts);
}

void addLiterals(const MinisatID::Model& model, GroundTranslator* translator, AbstractStructure* init) {
	for (auto literal = model.literalinterpretations.cbegin(); literal != model.literalinterpretations.cend(); ++literal) {
		int atomnr = literal->getAtom().getValue();

		if (translator->isInputAtom(atomnr)) {
			PFSymbol* symbol = translator->getSymbol(atomnr);
			const ElementTuple& args = translator->getArgs(atomnr);
			if (typeid(*symbol) == typeid(Predicate)) {
				Predicate* pred = dynamic_cast<Predicate*>(symbol);
				if (literal->hasSign()) {
					init->inter(pred)->makeFalse(args);
				} else {
					init->inter(pred)->makeTrue(args);
				}
			} else {
				Function* func = dynamic_cast<Function*>(symbol);
				if (literal->hasSign()) {
					init->inter(func)->graphInter()->makeFalse(args);
				} else {
					init->inter(func)->graphInter()->makeTrue(args);
				}
			}
		}
	}
}

void addTerms(const MinisatID::Model& model, GroundTermTranslator* termtranslator, AbstractStructure* init) {
	for (auto cpvar = model.variableassignments.cbegin(); cpvar != model.variableassignments.cend(); ++cpvar) {
		Function* function = termtranslator->function(cpvar->variable);
		if (function == NULL) {
			continue;
		}
		const auto& gtuple = termtranslator->args(cpvar->variable);
		ElementTuple tuple;
		for (auto it = gtuple.cbegin(); it != gtuple.cend(); ++it) {
			if (it->isVariable) {
				int value = model.variableassignments[it->_varid].value;
				tuple.push_back(createDomElem(value));
			} else {
				tuple.push_back(it->_domelement);
			}
		}
		tuple.push_back(createDomElem(cpvar->value));
		init->inter(function)->graphInter()->makeTrue(tuple);
	}
}
}
