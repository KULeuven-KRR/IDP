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

#include "SolverConnection.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include <cmath>

using namespace std;

namespace SolverConnection {

MinisatID::VarID convert(VarId varid) {
	return {varid.id};
}

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

MinisatID::Weight createWeight(double weight) {
	double test;
	if (modf(weight, &test) != 0) {
		throw notyetimplemented("Real number support in the search algorithm");
	}
	return int(weight);
}

typedef std::function<std::string(int)> callbackprintlit;
typedef std::function<std::string(const GroundTerm&)> callbackprintterm;

class CallBackTranslator: public PCPrinter {
private:
	callbackprintlit cblit;
	callbackprintterm cbterm;
public:
	CallBackTranslator(callbackprintlit cblit, callbackprintterm cbterm)
			: cblit(cblit), cbterm(cbterm) {

	}

	virtual bool hasTranslation(const MinisatID::VarID&) const {
		return true;
	}
	virtual bool hasTranslation(const MinisatID::Lit&) const {
		return true;
	}

	virtual std::string toString(MinisatID::VarID id) const{
		std::stringstream ss;
		ss << cbterm(GroundTerm(VarId{id.id}));
		return ss.str();
	}

	virtual std::string toString(const MinisatID::Lit& lit) const {
		std::stringstream ss;
		Lit l = var(lit);
		if (lit.hasSign()) {
			l = -l;
		}
		ss << cblit(l);
		return ss.str();
	}
};

MinisatID::Space* createsolver(int nbmodels) {
	MinisatID::SolverOption modes;
	modes.nbmodels = nbmodels;
	modes.verbosity = getOption(IntType::VERBOSE_SOLVING);
	modes.solvingstats = getOption(IntType::VERBOSE_SOLVING_STATISTICS) > 0;

	modes.randomseed = getOption(IntType::RANDOMSEED);

	modes.polarity = MinisatID::Polarity::STORED;
	modes.initactivity = MinisatID::InitActivity::DEFAULT;

	if (getOption(BoolType::STABLESEMANTICS)) {
		modes.defsem = MinisatID::DEFSEM::DEF_STABLE;
	} else {
		modes.defsem = MinisatID::DEFSEM::DEF_WELLF;
	}

	if (useLazyGrounding()) {
		modes.lazy = true;
	}
	modes.lazyheur = getOption(LAZYHEURISTIC);
	modes.watchedrelevance = getOption(WATCHEDRELEVANCE);
	modes.expandimmediately = getOption(BoolType::EXPANDIMMEDIATELY);

	if (getOption(BoolType::MXRANDOMPOLARITYCHOICE)) {
		modes.polarity = MinisatID::Polarity::RAND;
	}
	if (getOption(BoolType::MXRANDOMINITACTIVITY)) {
		modes.initactivity = MinisatID::InitActivity::RAND;
	}
    
	switch(getGlobal()->getOptions()->solverHeuristic()){
	case SolverHeuristic::CLASSIC:
		modes.heuristic = MinisatID::Heuristic::CLASSIC;
		break;
	case SolverHeuristic::VMTF:
		modes.heuristic = MinisatID::Heuristic::VMTF;
		break;
	default:
		modes.heuristic = MinisatID::Heuristic::CLASSIC;
		break;
	}

	return new MinisatID::Space(modes);
}

void setTranslator(MinisatID::Space* solver, GroundTranslator* translator) {
	auto trans = new CallBackTranslator([translator](const Lit& lit){return translator->printLit(lit);}, [translator](const GroundTerm& term){return translator->printTerm(term);});
	solver->setTranslator(trans);
	// FIXME trans is not deleted anywhere
}

MinisatID::ModelExpand* initsolution(MinisatID::Space* solver, int nbmodels, const litlist& assumptions) {
	auto print = getOption(IntType::VERBOSE_SOLVING)>1;
	MinisatID::ModelExpandOptions opts(nbmodels, print ? MinisatID::Models::ALL : MinisatID::Models::NONE, MinisatID::Models::ALL);
	return solver->createModelExpand(solver, opts, createList(assumptions));
}

PCUnitPropagate* initpropsolution(MinisatID::Space* solver) {
	return new PCUnitPropagate(solver, { });
}

MinisatID::ModelIterationTask* createIteratorSolution(PCSolver* solver, int nbmodels, const litlist& assumptions) {
	auto print = getOption(IntType::VERBOSE_SOLVING)>1;
	MinisatID::ModelExpandOptions opts(nbmodels, print ? MinisatID::Models::ALL : MinisatID::Models::NONE, MinisatID::Models::ALL);
	auto l = createList(assumptions);
	return new MinisatID::ModelIterationTask(solver, opts, l);
}

void addLiterals(const MinisatID::Model& model, GroundTranslator* translator, Structure* init) {
	for (auto literal : model.literalinterpretations) {
		CHECKTERMINATION;
		int atomnr = var(literal);

		if (translator->isInputAtom(atomnr)) {
			auto symbol = translator->getSymbol(atomnr);
			const auto& args = translator->getArgs(atomnr);
			if (literal.hasSign()) {
				init->inter(symbol)->makeFalseAtLeast(args);
			} else {
				init->inter(symbol)->makeTrueAtLeast(args);
			}
#ifndef NDEBUG
			if (not init->isConsistent()) {
				std::cerr << "mx made " << print(symbol) << " inconsistent when adding the element " << print(args) << endl;
			}
			Assert(init->isConsistent());
#endif
		}
	}
}

VarId getVar(MinisatID::VarID id) {
	VarId var;
	var.id = id.id;
	return var;
}

void addTerms(const MinisatID::Model& model, GroundTranslator* translator, Structure* init) {
	// Convert vector of variableassignments to a map
	map<VarId, int> variable2valuemap; // NOTE: only to int
	for (auto cpvar : model.variableassignments) {
		CHECKTERMINATION;
		if(cpvar.hasValue()){
			variable2valuemap[getVar(cpvar.getVariable())] = cpvar.getValue();
	}
	}
	// Add terms to the output structure
	std::set<Function*> hasnondenoting;
	for (auto cpvar : model.variableassignments) {
		CHECKTERMINATION;
		auto var = getVar(cpvar.getVariable());
		if(not translator->hasVarIdMapping(var)){
			continue;
		}
		auto function = translator->getFunction(var);
		if (not init->vocabulary()->contains(function)) {
			//Note: Only consider functions that are in the user's vocabulary, ignore internal ones.
			continue;
		}
		ElementTuple tuple;
		for (auto elem: translator->getArgs(var)) {
			if (elem.isVariable) {
				auto valueit = variable2valuemap.find(elem._varid);
				Assert(valueit!=variable2valuemap.cend()); // Otherwise, its application should not result in a value
				auto value = valueit->second;
				Assert(value<0 || not translator->hasVarIdMapping({(uint)value})); // NOTE: code works for one nesting of function terms, but not any deeper!!!
				tuple.push_back(createDomElem(value));
			} else {
				tuple.push_back(elem._domelement);
			}
		}
		if(cpvar.hasValue()){
			tuple.push_back(createDomElem(cpvar.getValue()));
			init->inter(function)->graphInter()->makeTrueAtLeast(tuple);
		}else{
			if(getOption(NBMODELS)==1){
				hasnondenoting.insert(function);
			}else{
				// TODO need a method to notify an function interpretation that it has no image for some value
				//init->inter(function)->notifyUndefined(tuple);
				tuple.push_back(NULL);
				for(auto e = init->inter(function)->universe().tables().back()->sortBegin(); not e.isAtEnd(); ++e){
					tuple.back() = *e;
					init->inter(function)->graphInter()->makeFalseAtLeast(tuple);
				}
			}
		}
	}
	if(getOption(NBMODELS)==1){
		for(auto f: hasnondenoting){
			makeUnknownsFalse(init->inter(f)->graphInter());
		}
	}
}

MinisatID::literallist createList(const litlist& origlist) {
	MinisatID::literallist list;
	for (auto lit : origlist) {
		list.push_back(createLiteral(lit));
	}
	return list;
}

MinisatID::Atom createAtom(const int lit) {
	return MinisatID::Atom(abs(lit));
}

MinisatID::Lit createLiteral(const int lit) {
	auto atom = createAtom(lit); // ignores sign
	return lit > 0 ? MinisatID::mkPosLit(atom) : MinisatID::mkNegLit(atom);
}

}
