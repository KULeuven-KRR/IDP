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

#include "ReplaceDefinitionsWithCompletion.hpp"

#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

using namespace std;

Theory* ReplaceDefinitionsWithCompletion::visit(Theory* theory) {
	for (auto def : theory->definitions()) {
		def->accept(this);
		for (auto sentence : _sentences){
			theory->add(sentence);
		}
		def->recursiveDelete();
	}
	theory->definitions().clear();
	return theory;
}

Definition* ReplaceDefinitionsWithCompletion::visit(Definition* def) {
	_headvars.clear();
	_symbol2sentences.clear();
	_sentences.clear();

	for (auto defsymbol : def->defsymbols()) {
		vector<Variable*> vv;
		for (auto sort : defsymbol->sorts()) {
			vv.push_back(new Variable(sort));
		}
		_headvars[defsymbol] = vv;
	}

	for (auto rule : def->rules()) {
		rule->accept(this);
	}

	for (auto symbol2sentences : _symbol2sentences) {
		auto symbol = symbol2sentences.first;
		const auto& sentences = symbol2sentences.second;

		Assert(not sentences.empty());

		auto body = sentences.size()==1?sentences[0]:new BoolForm(SIGN::POS, false, sentences, FormulaParseInfo());
		auto head = new PredForm(SIGN::POS, symbol, TermUtils::makeNewVarTerms(_headvars[symbol]), FormulaParseInfo());
		head->negate();
		Formula* sentence = new BoolForm(SIGN::POS, false, {head, body}, FormulaParseInfo());

		if(not _headvars[symbol].empty()){
			varset qv(_headvars[symbol].cbegin(), _headvars[symbol].cend());
			sentence = new QuantForm(SIGN::POS, QUANT::UNIV, qv, sentence, FormulaParseInfo());
		}

		_sentences.push_back(sentence);
	}

	return def;
}

Rule* ReplaceDefinitionsWithCompletion::visit(Rule* rule) {
	auto newrule = rule->clone();
	if(VocabularyUtils::isPredicate(newrule->head()->symbol(), STDPRED::EQ)){
		auto left = newrule->head()->subterms()[0];
		auto right = newrule->head()->subterms()[1];
		auto functerm = dynamic_cast<FuncTerm*>(left);
		auto terms = functerm->subterms();
		terms.push_back(right);
		newrule->head(new PredForm(SIGN::POS, functerm->function(), terms, {}));
	}
	newrule = DefinitionUtils::unnestNonVarHeadTerms(newrule, _structure);

	// Split quantified variables in head and body variables
	varset hv, bv;
	for (auto var : newrule->quantVars()) {
		if (newrule->head()->contains(var)) {
			hv.insert(var);
		} else {
			bv.insert(var);
		}
	}
	auto body = newrule->body();
	if(not bv.empty()){
		body = new QuantForm(SIGN::POS, QUANT::EXIST, bv, body, FormulaParseInfo((body->pi())));
	}
	newrule = new Rule(hv, newrule->head(), body, newrule->pi());

	// Add sentence body implies head
	varset vars;
	map<Variable*,Variable*> mappedvars;
	for(auto oldvar:newrule->quantVars()){
		auto newvar = new Variable(oldvar->sort());
		vars.insert(newvar);
		mappedvars[oldvar]=newvar;
	}
	auto left = newrule->body()->clone(mappedvars);
	left->negate();
	auto right = newrule->head()->clone(mappedvars);
	auto bf = new BoolForm(SIGN::POS, false, {left, right}, newrule->body()->pi());
	if(vars.empty()){
		_sentences.push_back(bf);
	}else{
		_sentences.push_back(new QuantForm(SIGN::POS, QUANT::UNIV, vars, bf ,newrule->body()->pi()));
	}

	// Create part of the sentence head implies body
	std::map<Variable*, Variable*> old2newheadvars;
	const auto& newheadvars = _headvars[newrule->head()->symbol()];
	for(uint i=0; i<newrule->head()->subterms().size(); ++i){
		auto vt = dynamic_cast<VarTerm*>(newrule->head()->subterms()[i]);
		Assert(vt!=NULL);
		old2newheadvars[vt->var()]=newheadvars[i];
	}
	auto newbody = newrule->body()->clone(old2newheadvars);
	_symbol2sentences[newrule->head()->symbol()].push_back(newbody);

	newrule->body()->recursiveDeleteKeepVars();
	newrule->head()->recursiveDeleteKeepVars();
	delete(newrule);

	return rule;
}
