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
#include "theory/TheoryUtils.hpp"

#include "options.hpp"
#include <iostream>

class PFSymbol;
class Predicate;

using namespace std;

struct ApproxData {
	std::map<const Formula*, PredForm*> formula2ct;
	std::map<const Formula*, PredForm*> formula2cf;
	std::map<PFSymbol*, PFSymbol*> _pred2predCt;
	std::map<PFSymbol*, PFSymbol*> _pred2predCf;
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
		std::vector<Formula*>* transformedSentences = performTransformations(sentences,s);
		auto g = GenerateApproximatingDefinition(*transformedSentences, freesymbols, s);
		auto approxing_def = g.getallRules(dir);
		auto approxdef_theory = g.constructTheory(approxing_def);
		auto approxdef_voc = g.constructVocabulary(s,approxing_def);
		auto approxdef_struct = g.constructStructure(s, approxdef_theory, approxdef_voc);
		if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
			clog << "Calculating the following definitions with XSB:\n" << toString(approxdef_theory) << "\n";
			clog << "With the following input structure:\n" << toString(approxdef_struct) << "\n";
		}
		if (DefinitionUtils::hasRecursionOverNegation(approxing_def)) {
			if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
				//TODO: either go back to normal method or FIX XSB to support recneg!
				clog << "Approximating definition had recursion over negation, not calculating it\n";
			}
			return;
		}
		auto output_structure = CalculateDefinitions::doCalculateDefinitions(approxdef_theory,approxdef_struct, g.getSymbolsToQuery());

		if(not output_structure.empty() && g.isConsistent(output_structure.at(0))) {
			g.updateStructure(s,output_structure.at(0));
			if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
				clog << "Calculating the approximating definitions with XSB resulted in the following structure:\n" <<
						toString(s) << "\n";
			}
		}
	}

private:
	GenerateApproximatingDefinition(const std::vector<Formula*>& sentences, const std::set<PFSymbol*>& actions, const AbstractStructure* s)
			: 	data(new ApproxData(actions)),
			  	_sentences(sentences) {
		for(auto sentence : sentences) {
			setFormula2PredFormMap(sentence, s);
		}

	}
	~GenerateApproximatingDefinition() {
		delete (data);
	}

	Definition* getallRules(Direction dir);

	std::vector<Rule*> getallDownRules();
	std::vector<Rule*> getallUpRules();

	void setFormula2PredFormMap(Formula*, const AbstractStructure*);
	std::pair<PredForm*,PredForm*> createGeneralPredForm(Formula*);

	static std::vector<Formula*>* performTransformations(const std::vector<Formula*>&, AbstractStructure*);
	Theory* constructTheory(Definition*);
	Vocabulary* constructVocabulary(AbstractStructure*, Definition*);
	AbstractStructure* constructStructure(AbstractStructure*, Theory*, Vocabulary*);
	std::set<PFSymbol*> getSymbolsToQuery();
	void updateStructure(AbstractStructure*, AbstractStructure*);
	bool isConsistent(AbstractStructure*);
};
