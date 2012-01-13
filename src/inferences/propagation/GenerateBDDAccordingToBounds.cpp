/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "common.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "GenerateBDDAccordingToBounds.hpp"

#include "utils/TheoryUtils.hpp"

using namespace std;

TruthType swapTF(TruthType type) {
	switch (type) {
	case TruthType::POSS_FALSE:
		return TruthType::POSS_TRUE;
	case TruthType::POSS_TRUE:
		return TruthType::POSS_FALSE;
	case TruthType::CERTAIN_FALSE:
		return TruthType::CERTAIN_TRUE;
	case TruthType::CERTAIN_TRUE:
		return TruthType::CERTAIN_FALSE;
	}
}

const FOBDD* GenerateBDDAccordingToBounds::evaluate(Formula* f, TruthType type) {
	_type = type;
	_result = NULL;
	f->accept(this);
	return _result;
}

bool needFalse(TruthType value) {
	return value == TruthType::CERTAIN_FALSE || value == TruthType::POSS_FALSE;
}

bool needPossible(TruthType value) {
	return value == TruthType::POSS_TRUE || value == TruthType::POSS_FALSE;
}

void GenerateBDDAccordingToBounds::visit(const PredForm* atom) {
	FOBDDFactory factory(_manager);

	if (_ctbounds.find(atom->symbol()) == _ctbounds.cend()) {
		auto bdd = factory.turnIntoBdd(atom);
		if (needFalse(_type)) {
			bdd = _manager->negation(bdd);
		}
		_result = bdd;
	} else {
		bool getct = (_type == TruthType::CERTAIN_TRUE || _type == TruthType::POSS_FALSE);
		if (isNeg(atom->sign())) {
			getct = not getct;
		}
		auto bdd = getct ? _ctbounds[atom->symbol()] : _cfbounds[atom->symbol()];
		map<const FOBDDVariable*, const FOBDDArgument*> mva;
		const auto& vars = _vars[atom->symbol()];
		for (unsigned int n = 0; n < vars.size(); ++n) {
			mva[vars[n]] = factory.turnIntoBdd(atom->subterms()[n]);
		}
		bdd = _manager->substitute(bdd, mva);
		if (needPossible(_type)) { // Negate because we have CERTAIN bounds
			bdd = _manager->negation(bdd);
		}
		_result = bdd;
	}
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
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	for (auto it = bddvars.cbegin(); it != bddvars.cend(); ++it) {
		copyvars.insert(optimizemanager.getVariable((*it)->variable()));
	}
	optimizemanager.optimizequery(copybdd, copyvars, indices, structure);

	// 2. Remove certain leaves
	auto pruned = optimizemanager.make_more_false(copybdd, copyvars, indices, structure, mcpa);

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
		output << "   ";
		(it->first)->put(output);
		output << endl << tabs();
		output << "      vars:";
		for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
			output << ' ';
			_manager->put(output, *jt);
		}
		output << '\n';
		output << tabs();
		output << "      ct:" << endl << tabs();
		pushtab();
		_manager->put(output, _ctbounds.find(it->first)->second);
		output << "      cf:" << endl << tabs();
		_manager->put(output, _cfbounds.find(it->first)->second);
		poptab();
	}
	return output;
}
