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

#include "AddIfCompletion.hpp"

#include "IncludeComponents.hpp"

using namespace std;

void AddIfCompletion::visit(const Theory* theory){
	for(auto def: theory->definitions()){
		auto result = getOnlyIfFormulas(*def);
		for(auto r: result){
			_result.push_back(r);
		}
	}
}

std::vector<OnlyIfFormula> AddIfCompletion::getOnlyIfFormulas(const AbstractTheory& theory) {
	_result.clear();
	theory.accept(this);
	return _result;
}

std::vector<OnlyIfFormula> AddIfCompletion::getOnlyIfFormulas(const Definition& def) {
	_headvars.clear();
	_interres.clear();
	for (const auto& defsymbol : def.defsymbols()) {
		vector<Variable*> vv;
		for (auto sort : defsymbol->sorts()) {
			vv.push_back(new Variable(sort));
		}
		_headvars[defsymbol] = vv;
	}
	for (auto rule : def.rules()) {
		addFor(*rule);
	}

	std::vector<OnlyIfFormula> mapping;
	for (const auto& symb2forms : _interres) {
		const auto& symbol = symb2forms.first;
		const auto& formulas = symb2forms.second;
		Assert(not formulas.empty());
		auto b = formulas[0];
		if (formulas.size() > 1){
			b = new BoolForm(SIGN::POS, false, formulas, FormulaParseInfo());
		}
		auto h = new PredForm(SIGN::POS, symbol, TermUtils::makeNewVarTerms(_headvars[symbol]), FormulaParseInfo());
		b->negate();
		auto ev = new BoolForm(SIGN::POS, false, h, b, FormulaParseInfo());
		if (symbol->sorts().empty()) {
			mapping.push_back({h, ev, def.getID()});
		} else {
			varset qv(_headvars[symbol].cbegin(), _headvars[symbol].cend());
			auto qf = new QuantForm(SIGN::POS, QUANT::UNIV, qv, ev, FormulaParseInfo());
			mapping.push_back({h, qf, def.getID()});
		}
	}

	return mapping;
}

void AddIfCompletion::addFor(const Rule& rule) {
	vector<Formula*> vf;
	auto vv = _headvars[rule.head()->symbol()];
	auto freevars = rule.quantVars();
	map<Variable*, Variable*> mvv;

	for (size_t n = 0; n < rule.head()->subterms().size(); ++n) {
		auto t = rule.head()->subterms()[n];
		if (typeid(*t) != typeid(VarTerm)) {
			auto bvt = new VarTerm(vv[n], TermParseInfo());
			auto p = get(STDPRED::EQ, vv[n]->sort());
			auto pf = new PredForm(SIGN::POS, p, {bvt, t->clone()}, FormulaParseInfo());
			vf.push_back(pf);
		} else {
			auto  v = *(t->freeVars().cbegin());
			if (mvv.find(v) == mvv.cend()) {
				mvv[v] = vv[n];
				freevars.erase(v);
			} else {
				auto bvt1 = new VarTerm(vv[n], TermParseInfo());
				auto bvt2 = new VarTerm(mvv[v], TermParseInfo());
				auto p = get(STDPRED::EQ, v->sort());
				auto pf = new PredForm(SIGN::POS, p, {bvt1,bvt2}, FormulaParseInfo());
				vf.push_back(pf);
			}
		}
	}
	auto b = rule.body()->clone(mvv);
	if (not vf.empty()) {
		vf.push_back(b);
		b = new BoolForm(SIGN::POS, true, vf, FormulaParseInfo());
	}
	if (not freevars.empty()) {
		b = new QuantForm(SIGN::POS, QUANT::EXIST, freevars, b, FormulaParseInfo());
	}
	auto c = b->clone(mvv);
	//Not complete (some variables might be useless), but better than no memorymanagement. TODO improve
	b->recursiveDeleteKeepVars();
	_interres[rule.head()->symbol()].push_back(c);
}
