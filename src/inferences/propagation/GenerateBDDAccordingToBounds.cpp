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

#include "IncludeComponents.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "GenerateBDDAccordingToBounds.hpp"

#include "theory/TheoryUtils.hpp"
#include "utils/ListUtils.hpp"

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

GenerateBDDAccordingToBounds::~GenerateBDDAccordingToBounds() {
}

const FOBDD* GenerateBDDAccordingToBounds::evaluate(Formula* f, TruthType type, const Structure* structure) {
	auto oldstructure = _structure;
	_structure = structure;
	_type = type;
	_result = NULL;
	f->accept(this);
	_structure = oldstructure;
	return _result;
}

void GenerateBDDAccordingToBounds::visit(const PredForm* atom) {
	//NOTE: all the commented code in this method is old code.
	//This code can be used if the symbolic structure is not "applied to structure" after propagation.
	//For example, in the case of lazy grounding, this might be useful.
	FOBDDFactory factory(_manager);

	auto symbol = atom->symbol();
	if (symbol->builtin()) {
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

		if ((_symbolsThatCannotBeReplacedByBDDs == NULL || not _symbolsThatCannotBeReplacedByBDDs->contains(symbol)) && contains(_ctbounds, symbol)
				&& contains(_cfbounds, symbol)) {
			auto bdd = getct ? _ctbounds.at(symbol) : _cfbounds.at(symbol);
			map<const FOBDDVariable*, const FOBDDTerm*> mva;
			const auto& vars = _vars[symbol];
			for (unsigned int n = 0; n < vars.size(); ++n) {
				mva[vars[n]] = factory.turnIntoBdd(atom->subterms()[n]);
			}
			_result = _manager->substitute(bdd, mva);
		} else {
			bool shouldUseThreeValuedSymbol = true;
			if(_structure != NULL){
				if(_structure->inter(symbol)->approxTwoValued()){
					//In case our symbol is already twovalued, don't query over the three-valued vocabulary.
					shouldUseThreeValuedSymbol = false;
				}
			}

			if (shouldUseThreeValuedSymbol) {
				auto newsymbol = getct ? clone->symbol()->derivedSymbol(SymbolType::ST_CT) : clone->symbol()->derivedSymbol(SymbolType::ST_CF);
				clone->symbol(newsymbol);
			} else{
				if(not getct){
					clone->negate();
				}
			}
			_result = factory.turnIntoBdd(clone);

		}

		if (needPossible(_type)) {
			_result = _manager->negation(_result);
		}

		//Now, the entire BDD for atom has been built. We add extra sort information:
		//The entire atom can (will) be false if something goes out-of-bounds
		//If NOT switchsign && needFalse, we add the disjunction: something goes out-of-bounds
		//If switchsign && NOT needFalse, IDEM
		//Otherwise, add the conjunction: nothing is out of bounds
		auto sorts = symbol->sorts();
		auto terms = atom->subterms();
		auto disj = (switchsign != needFalse(_type));
		for (size_t i = 0; i < sorts.size(); i++) {
			auto sorti = sorts[i];
			auto termi = terms[i];
			auto termisort = termi->sort();
			auto ancestors = termisort->ancestors();
			if (termisort == sorti || contains(ancestors, sorti)) {
				continue;
			}
			auto outofpredtype = new PredForm(SIGN::NEG, sorti->pred(), { termi }, FormulaParseInfo());
			auto insorttpe = new PredForm(SIGN::POS, termisort->pred(), { termi }, FormulaParseInfo());
			auto outofboundsbf = new BoolForm(SIGN::POS,true,outofpredtype,insorttpe,FormulaParseInfo());
			auto outofbounds = factory.turnIntoBdd(outofboundsbf);
			_result = disj ? _manager->disjunction(outofbounds, _result) : _manager->conjunction(_manager->negation(outofbounds), _result);
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
		auto newbdd = evaluate(*it, rectype, _structure);
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
	auto subbdd = evaluate(quantform->subformula(), rectype, _structure);
	auto vars = _manager->getVariables(quantform->quantVars());
	_result = universal ? _manager->univquantify(vars, subbdd) : _manager->existsquantify(vars, subbdd);
}

void GenerateBDDAccordingToBounds::visit(const EqChainForm* eqchainform) {
	Formula* cloned = eqchainform->clone();
	cloned = FormulaUtils::splitComparisonChains(cloned);
	_result = evaluate(cloned, _type, _structure);
	cloned->recursiveDelete();
}

void GenerateBDDAccordingToBounds::visit(const EquivForm* equivform) {
	Formula* cloned = equivform->clone();
	cloned = FormulaUtils::removeEquivalences(cloned);
	_result = evaluate(cloned, _type, _structure);
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

ostream& GenerateBDDAccordingToBounds::put(ostream& output) const {
	for (auto it = _vars.cbegin(); it != _vars.cend(); ++it) {

		(it->first)->put(output);
		pushtab();
		output << nt();
		output << "vars:";
		for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
			output << ' ';
			output << print(*jt);
		}
		output << nt();
		pushtab();
		output << "ct:" << nt();
		output << print(_ctbounds.find(it->first)->second);
		poptab();
		output << nt() << "cf:";
		pushtab();
		output << nt();
		output << print(_cfbounds.find(it->first)->second);
		poptab();
		poptab();
		output << nt();
	}
	return output;
}
