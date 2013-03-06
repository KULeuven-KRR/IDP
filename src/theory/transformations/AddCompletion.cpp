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

#include "AddCompletion.hpp"

#include "IncludeComponents.hpp"

using namespace std;

Theory* AddCompletion::visit(Theory* theory) {
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

Definition* AddCompletion::visit(Definition* def) {
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

		if(not _headvars.empty()){
			varset qv(_headvars[symbol].cbegin(), _headvars[symbol].cend());
			sentence = new QuantForm(SIGN::POS, QUANT::UNIV, qv, sentence, FormulaParseInfo());
		}

		_sentences.push_back(sentence);
	}

	return def;
}

Rule* AddCompletion::visit(Rule* rule) {
	// Add sentence body implies head
	varset vars;
	map<Variable*,Variable*> mappedvars;
	for(auto oldvar:rule->quantVars()){
		auto newvar = new Variable(oldvar->sort());
		vars.insert(newvar);
		mappedvars[oldvar]=newvar;
	}
	auto left = rule->body()->clone(mappedvars);
	left->negate();
	auto right = rule->head()->clone(mappedvars);
	_sentences.push_back(new QuantForm(SIGN::POS, QUANT::UNIV, vars, new BoolForm(SIGN::POS, false, {left, right}, rule->body()->pi()),rule->body()->pi()));

	// Create part of the sentence head implies body
	vector<Formula*> equalities;
	auto newheadvars = _headvars[rule->head()->symbol()];
	auto freevars = rule->quantVars();
	map<Variable*, Variable*> mvv;

	for (size_t n = 0; n < rule->head()->subterms().size(); ++n) {
		auto newheadvar = newheadvars[n];
		Term* t = rule->head()->subterms()[n];
		if (typeid(*t) != typeid(VarTerm)) {
			auto bvt = new VarTerm(newheadvar, TermParseInfo());
			auto p = get(STDPRED::EQ, newheadvar->sort());
			auto pf = new PredForm(SIGN::POS, p, {bvt, t->clone()}, FormulaParseInfo());
			equalities.push_back(pf);
		} else {
			auto v = *(t->freeVars().cbegin());
			if (mvv.find(v) == mvv.cend()) {
				mvv[v] = newheadvar;
				freevars.erase(v);
			} else {
				auto bvt1 = new VarTerm(newheadvar, TermParseInfo());
				auto bvt2 = new VarTerm(mvv[v], TermParseInfo());
				auto p = get(STDPRED::EQ, v->sort());
				auto pf = new PredForm(SIGN::POS, p, {bvt1,bvt2}, FormulaParseInfo());
				equalities.push_back(pf);
			}
		}
	}
	Formula* b = rule->body()->clone(mvv);
	if (not equalities.empty()) {
		equalities.push_back(b);
		b = new BoolForm(SIGN::POS, true, equalities, FormulaParseInfo());
	}
	if (not freevars.empty()) {
		b = new QuantForm(SIGN::POS, QUANT::EXIST, freevars, b, FormulaParseInfo());
	}
	auto c = b->clone(mvv);
	//Not complete (some variables might be useless), but better than no memorymanagement. TODO improve
	b->recursiveDeleteKeepVars();
	_symbol2sentences[rule->head()->symbol()].push_back(c);

	return rule;
}
