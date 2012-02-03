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

MinisatID::WrappedPCSolver* createsolver(int nbmodels) {
	auto options = GlobalData::instance()->getOptions();
	MinisatID::SolverOption modes;
	modes.nbmodels = nbmodels;
	modes.verbosity = options->getValue(IntType::SATVERBOSITY);

	modes.polarity = MinisatID::POL_STORED;
	if(getOption(BoolType::MXRANDOMPOLARITYCHOICE)/* || getOption(BoolType::GROUNDLAZILY)*/){ // TODO test
		modes.polarity = MinisatID::POL_RAND;
	}

	if (options->getValue(BoolType::GROUNDLAZILY)) {
		modes.lazy = true;
	}

	startInference(); // NOTE: have to tell the solver to reset its instance
	CHECKTERMINATION
	return new SATSolver(modes);
}

void setTranslator(MinisatID::WrappedPCSolver* solver, GroundTranslator* translator){
	callbackprinting cbprint(translator, &GroundTranslator::print);
	solver->setTranslator(cbprint);
}

MinisatID::Solution* initsolution() {
	auto options = GlobalData::instance()->getOptions();
	MinisatID::ModelExpandOptions opts;
	opts.nbmodelstofind = options->getValue(IntType::NBMODELS);
	opts.printmodels = MinisatID::PRINT_NONE;
	opts.savemodels = MinisatID::SAVE_ALL;
	opts.inference = MinisatID::MODELEXPAND;
	return new MinisatID::Solution(opts);
}

void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) {
	for (auto literal = model->literalinterpretations.cbegin(); literal != model->literalinterpretations.cend(); ++literal) {
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

void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init) {
	for (auto cpvar = model->variableassignments.cbegin(); cpvar != model->variableassignments.cend(); ++cpvar) {
		Function* function = termtranslator->function(cpvar->variable);
		if (function == NULL) {
			continue;
		}
		const auto& gtuple = termtranslator->args(cpvar->variable);
		ElementTuple tuple;
		for (auto it = gtuple.cbegin(); it != gtuple.cend(); ++it) {
			if (it->isVariable) {
				int value = model->variableassignments[it->_varid].value;
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
