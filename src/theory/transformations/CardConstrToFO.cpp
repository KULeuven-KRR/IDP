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

#include "CardConstrToFO.hpp"
#include "theory/TheoryUtils.hpp"
#include "IncludeComponents.hpp"
#include <map>
#include "common.hpp"
#include "errorhandling/error.hpp"
#include "creation/cppinterface.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

using namespace TermUtils;

/**
* Note: Many grounding optimization techniques cannot take aggregates into account.
* A partial remedy is converting cardinality aggregates to FO sentences, which 
* ofcourse can be used by grounding optimization techniques.
* Hence these Card2FO algorithms were developed.
**/

Formula* CardConstrToFO::visit(EqChainForm* ef) {
	bool needsSplit = false;
	for (auto st : ef->subterms()) {
		if (isCard(st)) {
			needsSplit = true;
			break;
		}
	}
	if (needsSplit) {
		auto newformula = FormulaUtils::splitComparisonChains((Formula*)ef);
		return newformula->accept(this);
	} else {
		return traverse(ef);
	}
}

Formula* CardConstrToFO::visit(PredForm* pf) {
	if (not VocabularyUtils::isComparisonPredicate(pf->symbol())) {
		return traverse(pf);
	}

	auto left = pf->subterms()[0];
	auto right = pf->subterms()[1];
	Formula* newformula;
	if (isCard(left)) {
		newformula = new AggForm(pf->sign(), right->clone(), invertComp(FormulaUtils::getComparison(pf)), dynamic_cast<AggTerm*>(left)->clone(), pf->pi());
	} else if (isCard(right)) {
		newformula = new AggForm(pf->sign(), left->clone(), FormulaUtils::getComparison(pf), dynamic_cast<AggTerm*>(right)->clone(), pf->pi());
	}else{
		return traverse(pf);
	}
	delete (pf);
	return newformula->accept(this);
}

Formula* CardConstrToFO::visit(AggForm* form) {
	if (form->_aggterm->function() != CARD || form->_aggterm->set()->getSets().size() > 1) {
		return traverse(form);
	}
	auto term = form->getBound();
	if (term->type() != TermType::DOM || not SortUtils::isSubsort(term->sort(), get(STDSORT::INTSORT))) {
		return traverse(form);
	}
	auto c = form->comp();
	auto bound = dynamic_cast<DomainTerm*>(term)->value()->value()._int;
	// If _maxVarsToIntroduce==0 or _maxVarsToIntroduce >= bound * the number of variables in the aggregate, then the aggregate will be converted to an FO formula.
	if (_maxVarsToIntroduce!=0 && bound * form->getAggTerm()->set()->getSets().front()->quantVars().size() > _maxVarsToIntroduce) {
		return traverse(form);
	}
	if (c == CompType::GT) {
		bound--;
		c = CompType::GEQ;
	} else if (c == CompType::LT) {
		bound++;
		c = CompType::LEQ;
	}

	if (c == CompType::GEQ) {
		return solveLesser(bound, form->_aggterm);
	} else if (c == CompType::LEQ) {
		return solveGreater(bound, form->_aggterm);
	} else if (c == CompType::EQ) {
		auto a = solveGreater(bound, form->_aggterm);
		auto b = solveLesser(bound, form->_aggterm);
		return new BoolForm(form->sign(), true, a, b, FormulaParseInfo());
	} else { //comptype NEQ
		auto a = solveGreater(bound + 1, form->_aggterm);
		auto b = solveLesser(bound - 1, form->_aggterm);
		return new BoolForm(form->sign(), false, a, b, FormulaParseInfo());
	}
}

QuantForm* CardConstrToFO::solveGreater(size_t b, AggTerm* f) {
	auto vs = varset();
	auto conjuncts = std::vector<Formula*>();
	auto quantSetExpr = f->set()->getSets()[0];
	auto vectorQuantvars = std::vector<Variable*>(quantSetExpr->quantVars().cbegin(), quantSetExpr->quantVars().cend());
	auto allQuantVars = varset();

	auto quantVarsByTitle = std::vector<std::vector<Variable*>>();
	//sublijst i stelt alle variabelen voor die dezelfde positie in de formule aannemen en dus moeten verschillen
	for (size_t i = 0; i < vectorQuantvars.size(); i++) {
		quantVarsByTitle.push_back(std::vector<Variable*>());
		//sublijsten worden leeg geinitialiseerd
	}

	for (size_t i = 0; i < b; i++) { //er moeten b sets variabelen zijn die aan de formule voldoen
		auto substs = std::map<Variable*, Variable*>();
		for (size_t j = 0; j < vectorQuantvars.size(); j++) { //voor elke variabele in 1 set
			auto curV = new Variable(vectorQuantvars[j]->sort());
			substs.insert( { vectorQuantvars[j], curV });
			allQuantVars.insert(curV);
			quantVarsByTitle[j].push_back(curV);
		}
		conjuncts.push_back(FormulaUtils::substituteVarWithVar(quantSetExpr->getSubFormula()->clone(), substs));
	}
	for (size_t j = 0; j < quantVarsByTitle[0].size(); j++) { //binnen elke set variabelen moeten ze allemaal verschillend zijn
		for (size_t k = 0; k < j; k++) { //j en k zijn indexen van corresponderende variabelen die moeten verschillend zijn
			std::vector<Formula*> disjuncts = std::vector<Formula*>();
			for (size_t i = 0; i < quantVarsByTitle.size(); i++) {
				auto a = new VarTerm(quantVarsByTitle[i][j], TermParseInfo());
				auto b = new VarTerm(quantVarsByTitle[i][k], TermParseInfo());
				auto temp = &Gen::operator==(*a, *b);
				temp->negate(); //zorgt dat de j en kde instantie van de i-de variabele verschillend zijn
				disjuncts.push_back(temp); //er moet maar 1 variabele verschillen om verschillende oplossingen te zijn -> disjuncts
			}
			conjuncts.push_back(&(Gen::disj(disjuncts))); //1 verschil moet gelden voor elke 2 variabelen -> conjuncts
		}
	}
	return new QuantForm(SIGN::POS, QUANT::EXIST, allQuantVars, &(Gen::conj(conjuncts)), FormulaParseInfo());
}

QuantForm* CardConstrToFO::solveLesser(size_t b, AggTerm* f) {
	auto returnV = solveGreater(b + 1, f);
	returnV->negate();
	return returnV;
}
