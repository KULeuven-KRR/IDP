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

class PFSymbol;
class Predicate;

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
			Structure* s, const std::set<PFSymbol*>& freesymbols, Direction dir){
		if(sentences.empty()) {
			return;
		}
		auto g = GenerateApproximatingDefinition(sentences, freesymbols);
		// FIXME what with new vocabulary?
		auto ret = g.getallRules(dir);
		Theory* testt = new Theory("testtheory", ParseInfo());
		testt->add(ret->clone());
		std::cout << "THEORY: " << toString(testt) << "\n";
		Vocabulary* testv = new Vocabulary(s->vocabulary()->name());
		for(Rule* rule : ret->rules()) {
			std::cout << "adding: " << rule->head()->symbol() << "\t or: " << toString(rule->head()->symbol()) << " to voc.\n";
			testv->add(rule->head()->symbol());
		}
		for(auto ctf : g.data->_predCt2InputPredCt) {
			testv->add(ctf.second);
		}
		for(auto cff : g.data->_predCf2InputPredCf) {
			testv->add(cff.second);
		}

		// TODO: is this vocabulary complete for all cases?
		std::cout << "VOCABULARY: " << toString(testv) << "\n";

		Structure* tests = new Structure("teststruct", testv, ParseInfo());

		for(auto ctf : g.data->_pred2predCt) {
			PredInter* newinter = new PredInter(s->inter(ctf.first)->ct(),true);
			auto interToChange = tests->inter(g.data->_predCt2InputPredCt[ctf.second->symbol()]);
			interToChange->ctpt(newinter->ct());
		}
		for(auto cff : g.data->_pred2predCf) {
			PredInter* newinter = new PredInter(s->inter(cff.first)->cf(),true);
			auto interToChange = tests->inter(g.data->_predCf2InputPredCf[cff.second->symbol()]);
			interToChange->ctpt(newinter->ct());
		}
		for(auto sortinter : s->getSortInters()) {
			tests->changeInter(sortinter.first,sortinter.second);
		}

		std::cout << "STRUCTURE: " << toString(tests) << "\n";
		auto out = CalculateDefinitions::doCalculateDefinitions(testt,tests);
		std::cout << "...done: " << toString(tests) << "\n";

		for(auto ctf : g.data->_pred2predCt) {
			std::cout << "changing: " << toString(ctf.first) << " and: \STRUCTURE: " << toString(s) << "\n";
			auto intertochange = s->inter(ctf.first);
			intertochange->ct(tests->inter(ctf.second->symbol())->ct());
		}
		for(auto cff : g.data->_pred2predCf) {
			auto intertochange = s->inter(cff.first);
			intertochange->cf(tests->inter(cff.second->symbol())->ct());
		}
		std::cout << "RESULT AFTER APPLYING APPROXIMATING DEFINITIONS:\n" << toString(s) << "END\n";
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
