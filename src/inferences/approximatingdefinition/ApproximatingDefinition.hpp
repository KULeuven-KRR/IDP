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
#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

struct mappings {
	// Mapping of original theory formulas to their (possibly tseitin) predforms
	std::map<const Formula*, PredForm*> _formula2ct;
	std::map<const Formula*, PredForm*> _formula2cf;

	// Mapping predicates to their CT and CF representations
	std::map<PFSymbol*, PFSymbol*> _pred2predCt;
	std::map<PFSymbol*, PFSymbol*> _pred2predCf;

	// Mapping CT and CF representation of predicates to the predicates
	// that represent the input that was given for
	std::map<PFSymbol*, PFSymbol*> _predCt2InputPredCt;
	std::map<PFSymbol*, PFSymbol*> _predCf2InputPredCf;


	mappings() {
	}


};

class ApproximatingDefinition {

public:

	enum class TruthPropagation {
		TRUE, FALSE
	};
	enum class Direction {
		UP, DOWN
	};

	struct DerivationTypes {
		std::set< std::pair<TruthPropagation,Direction> > _derivationtypes;

		void addDerivationType(TruthPropagation tp, Direction d);
		bool hasDerivation(TruthPropagation tp, Direction dir);
		bool hasDerivation(Direction dir);
	};

	enum class RuleType {
		CHEAP,
		FORALL
	};

	bool hasRuleType(RuleType rt) {
		return (_rule_types.find(rt) != _rule_types.end());
	}

	ApproximatingDefinition(DerivationTypes* represented_derivations,
			std::set<RuleType> rule_types,
			const Theory* original_theory) :
		_represented_derivations(represented_derivations),
		_rule_types(rule_types),
		_original_theory(original_theory),
		_approximating_vocabulary(new Vocabulary("approx_voc")),
		_approximating_definition(),
		_mappings()
	{};

	~ApproximatingDefinition() {
		delete(_approximating_definition);
		delete(_approximating_vocabulary);
		delete(_mappings);
		delete(_represented_derivations);
	}

	// Inspectors
	bool hasDerivation(TruthPropagation, Direction);
	bool hasDerivation(Direction);

	const Theory* originalTheory() {
		return _original_theory;
	}
	Definition* approximatingDefinition() {
		// since there is only one definition, we return the first one
		return _approximating_definition;
	}
	Vocabulary* approximatingVocabulary() {
		return _approximating_vocabulary;
	}
	mappings* getMappings() {
		return _mappings;
	}

	// Mutators
	void setApproximatingDefinition(Definition* def) {
		_approximating_definition = def;
	}
	void setVocabulary(Vocabulary* voc) {
		_approximating_vocabulary = voc;
	}
	void setMappings(mappings* m) {
		_mappings = m;
	}

	// High-level operations
	Vocabulary* getSymbolsToQuery();
	Structure* inputStructure(Structure*);
	bool isConsistent(Structure*);
	void updateStructure(Structure*, Structure*);

private:
	DerivationTypes* _represented_derivations;
	std::set<RuleType> _rule_types;
	const Theory* _original_theory;
	Vocabulary* _approximating_vocabulary;
	Definition* _approximating_definition;
	mappings* _mappings;
};
