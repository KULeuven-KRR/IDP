/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "AddFuncConstraints.hpp"
#include "IncludeComponents.hpp"

using namespace std;

Theory* AddFuncConstraints::visit(Theory* t) {
	TheoryMutatingVisitor::visit(t);
	for (auto it = _symbols.begin(); it != _symbols.end(); ++it) {
		Function* function = *it;
		if (not function->builtin()) { //TODO: CP support restrictions

			//Atom: F(x)=y
			auto vars = VarUtils::makeNewVariables(function->sorts());
			auto terms = TermUtils::makeNewVarTerms(vars);
			auto atom = new PredForm(SIGN::POS, function, terms, FormulaParseInfo());
			auto y = vars.back();
			set<Variable*> yset = { y };
			vars.pop_back();
			set<Variable*> xset(vars.cbegin(), vars.cend());

			//{y|F(x)=y}
			auto natsort = VocabularyUtils::natsort();
			auto one = createDomElem(1);
			auto oneterm = new DomainTerm(natsort, one, TermParseInfo());
			auto set = new QuantSetExpr(yset, atom, oneterm, SetParseInfo());

			auto comp = CompType::EQ;
			if (function->partial()) {
				comp = CompType::GEQ;
			}

			//#{y|F(x) = y} (= or =<) 1
			auto aggform = new AggForm(SIGN::POS, oneterm, comp, new AggTerm(set, AggFunction::CARD, TermParseInfo()), FormulaParseInfo());
			Formula* final;
			if (function->sorts().size() == 1) {
				final = aggform;
			} else {
				//!x: #{y|F(x) = y} (= or =<) 1
				final = new QuantForm(SIGN::POS, QUANT::UNIV, xset, aggform, FormulaParseInfo());
			}
			t->add(final);
		}
	}
	return t;
}

Term* AddFuncConstraints::visit(FuncTerm* t) {
	auto f = t->function();
		_symbols.insert(f);
	return traverse(t);
}

Formula* AddFuncConstraints::visit(PredForm* pf) {
	if (sametypeid<Function>(*(pf->symbol()))) {
		_symbols.insert(dynamic_cast<Function*>(pf->symbol()));
	}
	return traverse(pf);
}

