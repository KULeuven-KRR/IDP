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
#include "vocabulary/vocabulary.hpp"
#include "structure/Structure.hpp"
#include "structure/MainStructureComponents.hpp"
#include "structure/StructureComponents.hpp"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
#include "theory/theory.hpp"

#include "options.hpp"
#include <iostream>

class PFSymbol;
class Predicate;

using namespace std;

struct ApproxData {
	std::map<const Formula*, PredForm*> formula2ct;
	std::map<const Formula*, PredForm*> formula2cf;
	std::map<PFSymbol*, PredForm*> _pred2predCt;
	std::map<PFSymbol*, PredForm*> _pred2predCf;
	std::map<PFSymbol*, PFSymbol*> _predCt2InputPredCt;
	std::map<PFSymbol*, PFSymbol*> _predCf2InputPredCf;
	std::set<PFSymbol*> actions;
	bool _baseformulas_already_added;

	ApproxData(const std::set<PFSymbol*>& actions)
			: actions(actions),
			  _baseformulas_already_added(false){
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
	static void doGenerateApproximatingDefinition(const std::vector<Formula*>& sentences,
			Structure* s, const std::set<PFSymbol*>& freesymbols, Direction dir) {
		if(sentences.empty()) {
			return;
		}
		if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
			clog << "Calculating the approximating definitions with XSB...\n";
		}
		auto g = GenerateApproximatingDefinition(sentences, freesymbols);
		// FIXME what with new vocabulary?
		auto ret = g.getallRules(dir);
		Theory* approxdef_theory = new Theory("approxdef_theory", ParseInfo());
		approxdef_theory->add(ret->clone());
		Vocabulary* testv = new Vocabulary(s->vocabulary()->name());
		for(Rule* rule : ret->rules()) {
			testv->add(rule->head()->symbol());
		}
		for(auto ctf : g.data->_predCt2InputPredCt) {
			testv->add(ctf.second);
		}
		for(auto cff : g.data->_predCf2InputPredCf) {
			testv->add(cff.second);
		}

		Structure* approxdef_struct = new Structure("approxdef_struct", testv, ParseInfo());

		for(auto ctf : g.data->_pred2predCt) {
			PredInter* newinter = new PredInter(s->inter(ctf.first)->ct(),true);
			auto interToChange = approxdef_struct->inter(g.data->_predCt2InputPredCt[ctf.second->symbol()]);
			interToChange->ctpt(newinter->ct());
		}
		for(auto cff : g.data->_pred2predCf) {
			PredInter* newinter = new PredInter(s->inter(cff.first)->cf(),true);
			auto interToChange = approxdef_struct->inter(g.data->_predCf2InputPredCf[cff.second->symbol()]);
			interToChange->ctpt(newinter->ct());
		}
		for(auto sortinter : s->getSortInters()) {
			approxdef_struct->changeInter(sortinter.first,sortinter.second);
		}

		if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
			clog << "Calculating the following definitions with XSB:\n" << toString(approxdef_theory) << "\n";
			clog << "With the following input structure:\n" << toString(approxdef_struct) << "\n";
		}
		auto out = CalculateDefinitions::doCalculateDefinitions(approxdef_theory,approxdef_struct);

		for(auto ctf : g.data->_pred2predCt) {
			auto intertochange = s->inter(ctf.first);
			intertochange->ct(approxdef_struct->inter(ctf.second->symbol())->ct());
		}
		for(auto cff : g.data->_pred2predCf) {
			auto intertochange = s->inter(cff.first);
			intertochange->cf(approxdef_struct->inter(cff.second->symbol())->ct());
		}

		if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
			clog << "Calculating the approximating definitions with XSB resulted in the following structure:\n" <<
					toString(s) << "\n";
		}
	}

private:
	GenerateApproximatingDefinition(const std::vector<Formula*>& sentences, const std::set<PFSymbol*>& actions)
			: 	data(new ApproxData(actions)),
			  	_sentences(sentences) {
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
