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
#include "visitors/TheoryVisitor.hpp"

class PFSymbol;
class Predicate;

struct ApproxData {
	std::map<const Formula*, PredForm*> formula2ct;
	std::map<const Formula*, PredForm*> formula2cf;
	std::set<PFSymbol*> actions;

	ApproxData(const std::set<PFSymbol*>& actions)
			: actions(actions) {
	}
};

class GenerateApproximatingDefinition {
private:
	ApproxData* data;
	std::vector<Formula*> _sentences;

public:
	enum class Direction {
		UP, DOWN, BOTH
	};

	static Definition* doGenerateApproximatingDefinition(const std::vector<Formula*>& sentences, const std::set<PFSymbol*>& freesymbols, Direction dir){
		auto g = GenerateApproximatingDefinition(sentences, freesymbols);
		// FIXME what with new vocabulary?
		return g.getallRules(dir);
	}

private:
	GenerateApproximatingDefinition(const std::vector<Formula*>& sentences, const std::set<PFSymbol*>& actions)
			: 	data(new ApproxData(actions)),
			  	_sentences(sentences) {
		// TODO do transformations on the sentences
		// TODO do tseitin introduction + generate new vocabulary
		for(auto sentence : sentences) {
			setFormula2PredFormMap(sentence);
		}

	}
	~GenerateApproximatingDefinition() {
		delete (data);
	}

	Definition* getallRules(Direction dir);

	std::vector<Rule*> getallDownRules();
	std::vector<Rule*> getallUpRules();

	void setFormula2PredFormMap(Formula*);
};
