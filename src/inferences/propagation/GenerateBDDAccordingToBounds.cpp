/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
#include "vocabulary/vocabulary.hpp"
#include "IncludeComponents.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "GenerateBDDAccordingToBounds.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;

TruthType swapTF(TruthType type) {
	TruthType result = TruthType::CERTAIN_TRUE;
	switch (type) {
	case TruthType::POSS_FALSE:
		result = TruthType::POSS_TRUE;
		break;
	case TruthType::POSS_TRUE:
		result = TruthType::POSS_FALSE;
		break;
	case TruthType::CERTAIN_FALSE:
		result = TruthType::CERTAIN_TRUE;
		break;
	case TruthType::CERTAIN_TRUE:
		result = TruthType::CERTAIN_FALSE;
		break;
	}
	return result;
}

bool needFalse(TruthType value) {
	return value == TruthType::CERTAIN_FALSE || value == TruthType::POSS_FALSE;
}

bool needPossible(TruthType value) {
	return value == TruthType::POSS_TRUE || value == TruthType::POSS_FALSE;
}

GenerateBDDAccordingToBounds::~GenerateBDDAccordingToBounds(){
	if(_ownsmanager){
		delete(_manager);
	}
}

const FOBDD* GenerateBDDAccordingToBounds::evaluate(Formula* f, TruthType type) {

	_type = type;
	_result = NULL;
	f->accept(this);
	/*if (not needPossible(type)) {
	 std::cerr << "INPUT" << toString(f) << endl;
	 std::cerr << (needPossible(type) ? "P" : "C") << (needFalse(type) ? "F" : "T") << endl;
	 std::cerr << "OUTPUT" << toString(_result) << endl;
	 }*/
	return _result;
}

void GenerateBDDAccordingToBounds::visit(const PredForm* atom) {

	//NOTE: all the commented code in this method is old code.
	//This code can be used if the symbolic structure is not "applied to structure" after propagation.
	//For example, in the case of lazy grounding, this might be useful.
	FOBDDFactory factory(_manager);

	if (atom->symbol()->builtin()) {
		_result = factory.turnIntoBdd(atom);
		if (needFalse(_type)) {
			_result = _manager->negation(_result);
		}
	} else {

		bool getct = (_type == TruthType::CERTAIN_TRUE || _type == TruthType::POSS_FALSE);
		bool switchsign = isNeg(atom->sign());
		auto clone = atom->clone();
		if (switchsign) {
			getct = not getct;
			clone->negate();
		}
		auto symbol = getct ? clone->symbol()->derivedSymbol(SymbolType::ST_CT) : clone->symbol()->derivedSymbol(SymbolType::ST_CF);
		clone->symbol(symbol);
		_result = factory.turnIntoBdd(clone);

		if (needPossible(_type)) {
			_result = _manager->negation(_result);
		}
	}

	//SAVENESS FOR PARTIAL FUNCTIONS
	if (atom->symbol()->isFunction()) {
		auto f = dynamic_cast<Function*>(atom->symbol());
		if (f->partial() || is(atom->symbol(), STDFUNC::DIVISION) || is(atom->symbol(), STDFUNC::MODULO)) {
			auto newatom = atom->clone();
			if(newatom->sign()==SIGN::NEG){
				newatom->negate();
			}
			auto arity = newatom->subterms().size();
			auto lastsubterm = newatom->subterms()[arity-1];
			auto newvar = new Variable(lastsubterm->sort());
			auto varterm = new VarTerm(newvar, TermParseInfo());
			newatom->subterm(arity-1, varterm);
			auto newformula = new QuantForm(SIGN::POS, QUANT::EXIST, { newvar }, newatom, newatom->pi());
			auto hasimage = factory.turnIntoBdd(newformula);
			//Partial functions are always dangerous. Therefore, we play safe here. If we need certain, we make a stronger condition, if we need possible bounds,
			//we weaken the condition
			if(needPossible(_type)){
				_result = _manager->disjunction(_result, hasimage);
			}
			else{
				_result = _manager->conjunction(_result, hasimage);
			}
			newformula->recursiveDelete();
		}
	}

	/*//THE OLD CODE MAYBE USEFUL WHEN LAZY GROUNDING
	 if (_ctbounds.find(atom->symbol()) == _ctbounds.cend()) {
	 if (atom->symbol()->builtin()) {
	 _result = factory.turnIntoBdd(atom);
	 if (needFalse(_type)) {
	 _result = _manager->negation(_result);
	 }
	 } else {

	 _result = _manager->falsebdd();
	 if (needPossible(_type)) { // NEGATE because we have used CERTAIN bounds
	 _result = _manager->negation(_result);
	 }
	 }
	 } else {

	 bool getct = (_type == TruthType::CERTAIN_TRUE || _type == TruthType::POSS_FALSE);
	 if (isNeg(atom->sign())) {
	 getct = not getct;
	 }
	 auto bdd = getct ? _ctbounds[atom->symbol()] : _cfbounds[atom->symbol()];
	 map<const FOBDDVariable*, const FOBDDTerm*> mva;
	 const auto& vars = _vars[atom->symbol()];
	 for (unsigned int n = 0; n < vars.size(); ++n) {
	 mva[vars[n]] = factory.turnIntoBdd(atom->subterms()[n]);
	 }
	 _result = _manager->substitute(bdd, mva);
	 if (needPossible(_type)) {
	 _result = _manager->negation(_result);
	 }
	 }*/
}

void GenerateBDDAccordingToBounds::visit(const BoolForm* boolform) {
	bool conjunction = boolform->isConjWithSign();
	if (needFalse(_type)) {
		conjunction = not conjunction;
	}
	auto rectype = boolform->sign() == SIGN::POS ? _type : swapTF(_type);

	const FOBDD* currbdd = NULL;
	if (conjunction) {
		currbdd = _manager->truebdd();
	} else {
		currbdd = _manager->falsebdd();
	}

	for (auto it = boolform->subformulas().cbegin(); it != boolform->subformulas().cend(); ++it) {
		auto newbdd = evaluate(*it, rectype);
		currbdd = conjunction ? _manager->conjunction(currbdd, newbdd) : _manager->disjunction(currbdd, newbdd);
	}
	_result = currbdd;
}

void GenerateBDDAccordingToBounds::visit(const QuantForm* quantform) {
	bool universal = quantform->isUnivWithSign();
	if (needFalse(_type)) {
		universal = not universal;
	}
	auto rectype = quantform->sign() == SIGN::POS ? _type : swapTF(_type);
	auto subbdd = evaluate(quantform->subformula(), rectype);
	auto vars = _manager->getVariables(quantform->quantVars());
	_result = universal ? _manager->univquantify(vars, subbdd) : _manager->existsquantify(vars, subbdd);
}

void GenerateBDDAccordingToBounds::visit(const EqChainForm* eqchainform) {
	Formula* cloned = eqchainform->clone();
	cloned = FormulaUtils::splitComparisonChains(cloned);
	_result = evaluate(cloned, _type);
	cloned->recursiveDelete();
}

void GenerateBDDAccordingToBounds::visit(const EquivForm* equivform) {
	Formula* cloned = equivform->clone();
	cloned = FormulaUtils::removeEquivalences(cloned);
	_result = evaluate(cloned, _type);
	cloned->recursiveDelete();
}

void GenerateBDDAccordingToBounds::visit(const AggForm*) {
// TODO: better evaluation function?
	if (_type == TruthType::POSS_TRUE || _type == TruthType::POSS_FALSE) {
		_result = _manager->truebdd();
	} else {
		_result = _manager->falsebdd();
	}
}

const FOBDD* GenerateBDDAccordingToBounds::prunebdd(const FOBDD* bdd, const vector<const FOBDDVariable*>& bddvars, AbstractStructure* structure, double mcpa) {
// 1. Optimize the query
	FOBDDManager optimizemanager;
	auto copybdd = optimizemanager.getBDD(bdd, _manager);
	set<const FOBDDVariable*, CompareBDDVars> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	for (auto it = bddvars.cbegin(); it != bddvars.cend(); ++it) {
		copyvars.insert(optimizemanager.getVariable((*it)->variable()));
	}
	optimizemanager.optimizeQuery(copybdd, copyvars, indices, structure);

// 2. Remove certain leaves
	auto pruned = optimizemanager.makeMoreFalse(copybdd, copyvars, indices, structure, mcpa);

// 3. Replace result
	return _manager->getBDD(pruned, &optimizemanager);
}

void GenerateBDDAccordingToBounds::filter(AbstractStructure* structure, double max_cost_per_answer) {
	for (auto it = _ctbounds.begin(); it != _ctbounds.end(); ++it) {
		it->second = prunebdd(it->second, _vars[it->first], structure, max_cost_per_answer);
	}
	for (auto it = _cfbounds.begin(); it != _cfbounds.end(); ++it) {
		it->second = prunebdd(it->second, _vars[it->first], structure, max_cost_per_answer);
	}
}

ostream& GenerateBDDAccordingToBounds::put(ostream& output) const {
	for (auto it = _vars.cbegin(); it != _vars.cend(); ++it) {

		(it->first)->put(output);
		pushtab();
		output << nt();
		output << "vars:";
		for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
			output << ' ';
			output << toString(*jt);
		}
		output << nt();
		pushtab();
		output << "ct:" << nt();
		output << toString(_ctbounds.find(it->first)->second);
		poptab();
		output << nt() << "cf:";
		pushtab();
		output << nt();
		output << toString(_cfbounds.find(it->first)->second);
		poptab();
		poptab();
		output << nt();
	}
	return output;
}
