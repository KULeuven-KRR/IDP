/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ADDFUNCCON_HPP_
#define ADDFUNCCON_HPP_

#include "visitors/TheoryVisitor.hpp"
#include <set>
#include <cstdio>

#include "IncludeComponents.hpp"

class Function;
class Vocabulary;

class AddFuncConstraints: public DefaultTraversingTheoryVisitor {
private:
	std::set<Function*> _functions;
	bool _cpsupport;
	Vocabulary* _voc;
public:
	template<class T>
	void execute(T obj, Vocabulary* voc, std::map<Function*, Formula*>& funcconstraints, bool cpsupport) {
		Assert(obj!=NULL);

		_voc = voc;
		_cpsupport = cpsupport;
		_functions.clear();

		obj->accept(this);

		for (auto i = _functions.cbegin(); i != _functions.cend(); ++i) {
			if (funcconstraints.find(*i) == funcconstraints.cend() && not (*i)->builtin()) {
				funcconstraints[*i] = createFuncConstraints(*i);
			}
		}
	}

private:
	void visit(const PredForm* pf) {
		traverse(pf);
		if (isa<Function>(*pf->symbol())) {
			_functions.insert(dynamic_cast<Function*>(pf->symbol()));
		}
	}
	void visit(const FuncTerm* f) {
		traverse(f);
		if (not _cpsupport || not CPSupport::eligibleForCP(f->function(), _voc)) {
			_functions.insert(f->function());
		}
	}

	Formula* createFuncConstraints(Function* function) {
		if (verbosity() > 1) {
			cerr << "Adding function constraint for " << toString(function) << "\n";
		}
		//Atom: F(x)=y
		auto vars = VarUtils::makeNewVariables(function->sorts());
		auto terms = TermUtils::makeNewVarTerms(vars);
		auto atom = new PredForm(SIGN::POS, function, terms, FormulaParseInfo());
		auto y = vars.back();
		std::set<Variable*> yset = { y };
		vars.pop_back();
		std::set<Variable*> xset(vars.cbegin(), vars.cend());

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
