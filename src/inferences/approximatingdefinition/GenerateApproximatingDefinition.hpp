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
	std::set<PFSymbol*> actions;
	bool _baseformulas_already_added;
	std::map<Predicate*, const PredForm*> _basePredsCT2InputPreds;
	std::map<Predicate*, const PredForm*> _basePredsCF2InputPreds;

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
//		std::cout << "THEORY: " << toString(testt) << "\n";
		Vocabulary* testv = new Vocabulary("testvoc");
		for(Rule* rule : ret->rules()) {
			testv->add(rule->head()->symbol());
		}
		for(auto ctf : g.data->_basePredsCT2InputPreds) {
			testv->add(ctf.first);
		}
		for(auto cff : g.data->_basePredsCF2InputPreds) {
			testv->add(cff.first);
		}
//		std::cout << "VOCABULARY: " << toString(testv) << "\n";

		Structure* tests = new Structure("teststruct", testv, ParseInfo());

		for(auto ctf : g.data->_basePredsCT2InputPreds) {
			PredInter* newinter = new PredInter(s->inter(ctf.second->symbol())->ct(),true);
			tests->changeInter(ctf.first,newinter);
		}
		for(auto cff : g.data->_basePredsCF2InputPreds) {
			PredInter* newinter = new PredInter(s->inter(cff.second->symbol())->cf(),true);
			tests->changeInter(cff.first,newinter);
		}
		for(auto sortinter : s->getSortInters()) {
			tests->changeInter(sortinter.first,sortinter.second);
		}
//		std::cout << "STRUCTURE: " << toString(tests) << "\n";
		auto out = CalculateDefinitions::doCalculateDefinitions(testt,tests);

//		std::cout << "------------------------done calculating definitions\n";

		for(auto ctf : g.data->_basePredsCT2InputPreds) {
			auto intertochange = s->inter(ctf.second->symbol());
			auto univ = tests->inter((*g.data->formula2ct[ctf.second]).symbol())->universe();
			auto interpredtable = tests->inter((*g.data->formula2ct[ctf.second]).symbol())->ct();
			for( auto it = interpredtable->begin(); not it.isAtEnd(); ++it) {
				auto mlkjsdf = (*it);
				if(ctf.second->sign() == SIGN::NEG) {
					intertochange->makeFalse(mlkjsdf,true);
				} else {
					intertochange->makeTrue(mlkjsdf,true);
				}
			}
		}
		for(auto cff : g.data->_basePredsCF2InputPreds) {
			auto intertochange = s->inter(cff.second->symbol());
			auto univ = tests->inter((*g.data->formula2cf[cff.second]).symbol())->universe();
			auto interpredtable = tests->inter((*g.data->formula2cf[cff.second]).symbol())->ct();
			for( auto it = interpredtable->begin(); not it.isAtEnd(); ++it) {
				auto mlkjsdf = (*it);
				if(cff.second->sign() == SIGN::NEG) {
					intertochange->makeTrue(mlkjsdf,true);
				} else {
					intertochange->makeFalse(mlkjsdf,true);
				}
			}
		}
//		std::cout << "RESULT AFTER APPLYING APPROXIMATING DEFINITIONS:\n" << toString(s) << "END\n";
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
