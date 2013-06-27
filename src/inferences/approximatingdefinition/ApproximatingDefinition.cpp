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

#include "ApproximatingDefinition.hpp"

void ApproximatingDefinition::DerivationTypes::addDerivationType(
		TruthPropagation tp, Direction dir) {
	_derivationtypes.insert(std::pair<TruthPropagation, Direction>(tp,dir));
}

bool ApproximatingDefinition::DerivationTypes::hasDerivation(
		TruthPropagation tp, Direction dir) {
	for (auto derivation : _derivationtypes) {
		if(derivation.first == tp && derivation.second==dir) {
			return true;
		}
	}
	return false;
}
bool ApproximatingDefinition::DerivationTypes::hasDerivation(Direction dir) {
	for (auto derivation : _derivationtypes) {
		if(derivation.second==dir) {
			return true;
		}
	}
	return false;
}

std::set<PFSymbol*> ApproximatingDefinition::getSymbolsToQuery() {
	auto ret = std::set<PFSymbol*>();

	ret.insert(_mappings->_true_predform->symbol());
	ret.insert(_mappings->_false_predform->symbol());

	for(auto ctf : _mappings->_pred2predCt) {
		ret.insert(ctf.second);
	}
	for(auto cff : _mappings->_pred2predCf) {
		ret.insert(cff.second);
	}
	return ret;
}

Structure* ApproximatingDefinition::inputStructure(AbstractStructure* structure) {

	auto ret = new Structure("approxdef_struct", _approximating_vocabulary, ParseInfo());

	for(auto sortinter : structure->getSortInters()) {
		ret->inter(sortinter.first)->internTable(sortinter.second->internTable());
	}

	for(auto ctf : _mappings->_pred2predCt) {
		auto newinter = new PredInter(structure->inter(ctf.first)->ct(),true);
		auto interToChange = ret->inter(_mappings->_predCt2InputPredCt[ctf.second]);
		interToChange->ctpt(newinter->ct());
	}
	for(auto cff : _mappings->_pred2predCf) {
		auto newinter = new PredInter(structure->inter(cff.first)->cf(),true);
		auto interToChange = ret->inter(_mappings->_predCf2InputPredCf[cff.second]);
		interToChange->ctpt(newinter->ct());
	}
	// Only one definition in the theory
	auto opens = DefinitionUtils::opens(_approximating_definition);
	for (auto opensymbol : opens) {
		if (structure->vocabulary()->contains(opensymbol) &&
				structure->inter(opensymbol)->approxTwoValued()) {
			ret->inter(opensymbol)->ctpt(structure->inter(opensymbol)->ct());
		}
	}
	return ret;
}

bool ApproximatingDefinition::isConsistent(AbstractStructure* s) {
	// If the built-in "false" formula is made true, the theory has to be inconsistent
	if (not s->inter(_mappings->_false_predform->symbol())->ct()->empty()) {
		std::stringstream ss;
		ss << "The approximating definition detected that built-in FALSE "
			  "had to be true in order for the theory to be consistent.\n"
			  "Hence, the theory is inconsistent.";
		Warning::warning(ss.str());
		return false;
	}
	for (auto i : _original_theory->sentences()) {
		auto sentence_cf = _mappings->_formula2cf[i];
		// If one of the top-level sentences is detected to be certainly false,
		// the theory has to be inconsistent
		if(s->vocabulary()->contains(sentence_cf->symbol()) &&
				not s->inter(sentence_cf->symbol())->ct()->empty()){
			std::stringstream ss;
			ss << "The approximating definition detected formula " << toString(i) << " to be certainly false.\n";
			Warning::warning(ss.str());
			return false;
		}
	}
	return true;
}

void ApproximatingDefinition::updateStructure(AbstractStructure* s, AbstractStructure* approxdef_struct) {
	for(auto ctf : _mappings->_pred2predCt) {
		if(isa<Function>(*(ctf.first))) {
			auto symbAsFunction = dynamic_cast<Function*>(ctf.first);
			auto newPI = new PredInter(approxdef_struct->inter(ctf.second)->ct(),true);
			newPI->cf(approxdef_struct->inter(_mappings->_pred2predCf[ctf.first])->ct());
			auto newfi = new FuncInter(newPI);
			s->changeInter(symbAsFunction,newfi);
		} else {
			s->inter(ctf.first)->ct(approxdef_struct->inter(ctf.second)->ct());
			s->inter(ctf.first)->cf(approxdef_struct->inter(_mappings->_pred2predCf[ctf.first])->ct());
		}
	}
}
