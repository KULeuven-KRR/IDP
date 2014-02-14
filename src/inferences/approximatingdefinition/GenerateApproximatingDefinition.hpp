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

#pragma once

#include "common.hpp"
#include "ApproximatingDefinition.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "vocabulary/vocabulary.hpp"
#include "structure/Structure.hpp"
#include "structure/MainStructureComponents.hpp"
#include "structure/StructureComponents.hpp"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
#include "theory/theory.hpp"
#include "theory/TheoryUtils.hpp"

#include "options.hpp"
#include <iostream>

class PFSymbol;
class Predicate;

using namespace std;

struct ApproxDefGeneratorData {
	set<PFSymbol*> _freesymbols;
	bool _baseformulas_already_added;
	mappings* _mappings;
	ApproximatingDefinition::DerivationTypes* _derivations;
	std::set<ApproximatingDefinition::RuleType> _rule_types;

	ApproxDefGeneratorData(const set<PFSymbol*>& freesymbols,
			ApproximatingDefinition::DerivationTypes* derivations,
			std::set<ApproximatingDefinition::RuleType> rule_types) :
		_freesymbols(freesymbols),
		_baseformulas_already_added(false),
		_mappings(new mappings()),
		_derivations(derivations),
		_rule_types(rule_types){
	}

	ApproxDefGeneratorData(const ApproxDefGeneratorData* other) :
		_freesymbols(other->_freesymbols),
		_baseformulas_already_added(other->_baseformulas_already_added),
		_mappings(other->_mappings),
		_derivations(other->_derivations){

	}
};

class GenerateApproximatingDefinition {
private:
	// Actions are the symbols that need not be replaced
	ApproxDefGeneratorData* _approxdefgeneratordata;
	const vector<Formula*> _sentences;

public:

	static ApproximatingDefinition* doGenerateApproximatingDefinition(
			const AbstractTheory* orig_theory,
			ApproximatingDefinition::DerivationTypes* derivations,
			std::set<ApproximatingDefinition::RuleType> rule_types,
			const Structure* structure = NULL,
			const set<PFSymbol*>& freesymbols = set<PFSymbol*>());

private:

	GenerateApproximatingDefinition(const vector<Formula*>& sentences,
			const set<PFSymbol*>& actions,
			ApproximatingDefinition::DerivationTypes* derivations,
			std::set<ApproximatingDefinition::RuleType> rule_types);
	~GenerateApproximatingDefinition() {}

	Definition* getDefinition();
	Definition* getBasicDefinition(); // Get the definition that forms the basis for all approximating definitions
	vector<Rule*> getallDownRules();
	vector<Rule*> getallUpRules();

	void setFormula2PredFormMap(Formula*);
	pair<PredForm*,PredForm*> createGeneralPredForm(Formula*);

	static const vector<Formula*> performTransformations(const vector<Formula*>&,
			const Structure* structure = NULL);
	Vocabulary* constructVocabulary(Vocabulary*,Definition*);

};
