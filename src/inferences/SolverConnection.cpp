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

#include <cmath>

using namespace std;

namespace SolverConnection {

MinisatID::AggType convert(AggFunction agg) {
	MinisatID::AggType type = MinisatID::AggType::CARD;
	switch (agg) {
	case AggFunction::CARD:
		type = MinisatID::AggType::CARD;
		break;
	case AggFunction::SUM:
		type = MinisatID::AggType::SUM;
		break;
	case AggFunction::PROD:
		type = MinisatID::AggType::PROD;
		break;
	case AggFunction::MIN:
		type = MinisatID::AggType::MIN;
		break;
	case AggFunction::MAX:
		type = MinisatID::AggType::MAX;
		break;
	}
	return type;
}
MinisatID::EqType convert(CompType rel) {
	switch (rel) {
	case CompType::EQ:
		return MinisatID::EqType::EQ;
	case CompType::NEQ:
		return MinisatID::EqType::NEQ;
	case CompType::GEQ:
		return MinisatID::EqType::GEQ;
	case CompType::LEQ:
		return MinisatID::EqType::LEQ;
	case CompType::GT:
		return MinisatID::EqType::G;
	case CompType::LT:
		return MinisatID::EqType::L;
	}
	//To avoid compiler warnings
	Assert(false);
	return MinisatID::EqType::EQ;
}

MinisatID::Atom createAtom(const int lit) {
	return MinisatID::Atom(abs(lit));
}

MinisatID::Literal createLiteral(const int lit) {
	return MinisatID::mkLit(abs(lit), lit < 0);
}

MinisatID::literallist createList(const litlist& origlist) {
	MinisatID::literallist list;
	for (auto i = origlist.cbegin(); i < origlist.cend(); i++) {
		list.push_back(createLiteral(*i));
	}
	return list;
}


MinisatID::Weight createWeight(double weight) {
	double test;
	if (modf(weight, &test) != 0) {
		throw notyetimplemented("MinisatID does not support doubles yet.");
	}
	return int(weight);
}

typedef cb::Callback1<std::string, int> callbackprinting;

class CallBackTranslator: public PCPrinter{
private:
	callbackprinting cb;
public:
	CallBackTranslator(callbackprinting cb): cb(cb){

	}

	virtual bool hasTranslation(const MinisatID::Lit&) const {
		return true;
	}

	virtual std::string toString(const MinisatID::Lit& lit) const{
		std::stringstream ss;
		auto l = var(lit);
		if(lit.hasSign()){
			l = -l;
		}
		ss <<cb(l);
		return ss.str();
	}
};



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

	return new PCSolver(modes);
}

void setTranslator(PCSolver* solver, GroundTranslator* translator){
	auto trans = new CallBackTranslator(callbackprinting(translator, &GroundTranslator::print));
	solver->setTranslator(trans);
	// FIXME trans is not deleted anywhere
}

PCModelExpand* initsolution(PCSolver* solver, int nbmodels) {
	MinisatID::ModelExpandOptions opts(nbmodels, MinisatID::Models::NONE, MinisatID::Models::ALL);
	return new PCModelExpand(solver, opts, {});
}

PCUnitPropagate* initpropsolution(PCSolver* solver) {
	return new PCUnitPropagate(solver, {});
}

void addLiterals(const MinisatID::Model& model, GroundTranslator* translator, AbstractStructure* init) {
	for (auto literal = model.literalinterpretations.cbegin(); literal != model.literalinterpretations.cend(); ++literal) {
		int atomnr = var(*literal);

		if (translator->isInputAtom(atomnr)) {
			PFSymbol* symbol = translator->getSymbol(atomnr);
			const ElementTuple& args = translator->getArgs(atomnr);
			if (isa<Predicate>(*symbol)) {
				Predicate* pred = dynamic_cast<Predicate*>(symbol);
				if (literal->hasSign()) {
					init->inter(pred)->makeFalse(args);
				} else {
					init->inter(pred)->makeTrue(args);
				}
			} else {
				Assert(isa<Function>(*symbol));
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
	// Convert vector of variableassignments to a map
	map<VarId,int> variable2valuemap;
	for (auto cpvar = model.variableassignments.cbegin(); cpvar != model.variableassignments.cend(); ++cpvar) {
		variable2valuemap[cpvar->variable] = cpvar->value;
	}
	// Add terms to the output structure
	for (auto cpvar = model.variableassignments.cbegin(); cpvar != model.variableassignments.cend(); ++cpvar) {
		auto function = termtranslator->function(cpvar->variable);
		if (function == NULL) {
			continue;
		}
		const auto& gtuple = termtranslator->args(cpvar->variable);
		ElementTuple tuple;
		for (auto it = gtuple.cbegin(); it != gtuple.cend(); ++it) {
			if (it->isVariable) {
				int value = variable2valuemap[it->_varid];
				tuple.push_back(createDomElem(value));
			} else {
				tuple.push_back(it->_domelement);
			}
		}
		tuple.push_back(createDomElem(cpvar->value));
	//	cerr <<"Adding tuple " <<toString(tuple) <<" to " <<toString(function) <<"\n";
		init->inter(function)->graphInter()->makeTrue(tuple);
	}
}
}
