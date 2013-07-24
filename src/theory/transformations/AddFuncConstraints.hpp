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

#ifndef ADDFUNCCON_HPP_
#define ADDFUNCCON_HPP_

#include "visitors/TheoryVisitor.hpp"
#include <set>
#include <cstdio>

#include "IncludeComponents.hpp"

class Function;
class Vocabulary;

// TODO increase granularity to ground terms which occur in the theory
class AddFuncConstraints: public DefaultTraversingTheoryVisitor {
private:
	std::set<Function*> _functions;
	Vocabulary* _voc;
	bool _alsoCPableFunctions;

public:
	template<class T>
	void execute(T obj, Vocabulary* voc, std::map<Function*, Formula*>& funcconstraints, bool alsoCPableFunctions) {
		Assert(obj!=NULL);

		_voc = voc;
		_functions.clear();
		_alsoCPableFunctions = alsoCPableFunctions;

		obj->accept(this);

		for (auto i = _functions.cbegin(); i != _functions.cend(); ++i) {
			if (funcconstraints.find(*i) == funcconstraints.cend() && not (*i)->builtin()) {
				funcconstraints[*i] = createFuncConstraints(*i);
			}
		}
	}

private:
	void add(Function* function){
		if (_alsoCPableFunctions || not CPSupport::eligibleForCP(function, _voc)) {
			_functions.insert(function);
		}
	}
	void visit(const PredForm* pf) {
		traverse(pf);
		if (pf->symbol()->isFunction()) {
			add(dynamic_cast<Function*>(pf->symbol()));
		}
	}
	void visit(const FuncTerm* f) {
		traverse(f);
		add(f->function());
	}

	Formula* createFuncConstraints(Function* function) {
		if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 2) {
			clog << "Adding function constraint for " << print(function) << "\n";
		}
		//Atom: F(x)=y
		auto vars = VarUtils::makeNewVariables(function->sorts());
		auto terms = TermUtils::makeNewVarTerms(vars);
		auto atom = new PredForm(SIGN::POS, function, terms, FormulaParseInfo());
		auto y = vars.back();
		varset yset = { y };
		vars.pop_back();
		varset xset(vars.cbegin(), vars.cend());

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
		auto aggform = new AggForm(SIGN::POS, oneterm->clone(), comp, new AggTerm(new EnumSetExpr( { set }, set->pi()), AggFunction::CARD, TermParseInfo()),
				FormulaParseInfo()); //double usage of oneterm ===> clone!
		Formula* final;
		if (function->sorts().size() == 1) {
			final = aggform;
		} else {
			//!x: #{y|F(x) = y} (= or =<) 1
			final = new QuantForm(SIGN::POS, QUANT::UNIV, xset, aggform, FormulaParseInfo());
		}
		return final;
	}
};

#endif /* ADDFUNCCON_HPP_ */
