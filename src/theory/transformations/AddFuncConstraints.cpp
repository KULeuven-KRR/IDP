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
#include "utils/CPUtils.hpp"

using namespace std;

Theory* AddFuncConstraints::createTheory(const ParseInfo& pi) const {
	auto voc = new Vocabulary("_internal", pi);
	auto _theory = new Theory("_internal", voc, pi); // FIXME can the name conflict with other theories???
	for (auto it = _symbols.begin(); it != _symbols.end(); ++it) {
		auto function = *it;
		if (function->builtin()) {
			continue;
		}
		voc->add(function);

		//Atom: F(x)=y
		auto vars = VarUtils::makeNewVariables(function->sorts());
		auto terms = TermUtils::makeNewVarTerms(vars);
		auto atom = new PredForm(SIGN::POS, function, terms, FormulaParseInfo());
		auto y = vars.back();
		set<Variable*> yset = { y };
		vars.pop_back();
		set<Variable*> xset(vars.cbegin(), vars.cend());

		//{y|F(x)=y}
		auto natsort = get(STDSORT::NATSORT);
		auto one = createDomElem(1);
		auto oneterm = new DomainTerm(natsort, one, TermParseInfo());
		auto set = new QuantSetExpr(yset, atom, oneterm, SetParseInfo());

		auto comp = CompType::EQ;
		if (function->partial()) {
			comp = CompType::GEQ;
		}

		//#{y|F(x) = y} (= or =<) 1
		auto aggform = new AggForm(SIGN::POS, oneterm->clone(), comp, new AggTerm(new EnumSetExpr({set}, set->pi()), AggFunction::CARD, TermParseInfo()), FormulaParseInfo()); //double usage of oneterm ===> clone!
		Formula* final;
		if (function->sorts().size() == 1) {
			final = aggform;
		} else {
			//!x: #{y|F(x) = y} (= or =<) 1
			final = new QuantForm(SIGN::POS, QUANT::UNIV, xset, aggform, FormulaParseInfo());
		}
		_theory->add(final);
	}
	return _theory;
}

void AddFuncConstraints::visit(const FuncTerm* t) {
	auto f = t->function();
	if (not _cpsupport || (_vocabulary != NULL && not CPSupport::eligibleForCP(t, _vocabulary))) {
		_symbols.insert(f);
	}
	traverse(t);
}

void AddFuncConstraints::visit(const PredForm* pf) {
	if (isa<Function>(*(pf->symbol()))) {
		_symbols.insert(dynamic_cast<Function*>(pf->symbol()));
	}
	traverse(pf);
}

