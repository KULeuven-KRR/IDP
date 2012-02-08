/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FINDUNKNBOUND_HPP_
#define FINDUNKNBOUND_HPP_

#include "IncludeComponents.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

/**
 *	Given a formula F, find a literal L such that,
 *		for each interpretation I in which L^I=unknown,
 *			then either I[L=T] \models F or I[L=F] \models F.
 */
class FindUnknownBoundLiteral: public TheoryVisitor {
	VISITORFRIENDS()
private:
	bool start;
	bool allquantvars;
	const GroundTranslator* translator;
	const PredForm* resultingliteral;
	std::set<Variable*> quantvars;
public:
	template<typename T>
	const PredForm* execute(T t, const GroundTranslator* trans) {
		start = true;
		resultingliteral = NULL;
		translator = trans;
		t->accept(this);
		return resultingliteral;
	}

protected:
	virtual void traverse(const Formula* f) {
		for (size_t n = 0; n < f->subterms().size(); ++n) {
			f->subterms()[n]->accept(this);
		}
		for (size_t n = 0; n < f->subformulas().size() && resultingliteral==NULL; ++n) {
			f->subformulas()[n]->accept(this);
		}
	}
	virtual void visit(const PredForm* pf){
		if(translator->isAlreadyDelayedOnDifferentID(pf->symbol(), -1)){
			return;
		}
		allquantvars = true;
		for(auto i=pf->args().cbegin(); i<pf->args().cend(); ++i) {
			 (*i)->accept(this);
		}
		if(allquantvars){
			resultingliteral = pf;
		}
		return;
	}
	virtual void visit(const BoolForm* formula){
		if(not formula->conj()){
			traverse(formula);
		}
		return;
	}
	virtual void visit(const QuantForm* formula){
		if(start){
			quantvars = formula->quantVars();
		}else{
			return;
		}
		start = false;
		traverse(formula);
		return;
	}

	virtual void visit(const VarTerm* term){
		start = false;
		if(quantvars.find(term->var())==quantvars.cend()){
			allquantvars = false;
		}
		return;
	}
	virtual void visit(const FuncTerm*){
		allquantvars = false;
		start = false;
		return;
	}
	virtual void visit(const DomainTerm*){
		allquantvars = false;
		start = false;
		return;
	}
	virtual void visit(const AggTerm*){
		allquantvars = false;
		start = false;
		return;
	}

	// NOTE: DEFAULTS: just return
	virtual void visit(const EqChainForm*){
		start = false;
		return;
	}
	virtual void visit(const EquivForm*){
		start = false;
		return;
	}
	virtual void visit(const Theory*){
		start = false;
		return;
	}
	virtual void visit(const AbstractGroundTheory*){
		start = false;
		return;
	}
	virtual void visit(const GroundTheory<GroundPolicy>*){
		start = false;
		return;
	}
	virtual void visit(const AggForm*){
		start = false;
		return;
	}
	virtual void visit(const GroundDefinition*){
		start = false;
		return;
	}
	virtual void visit(const PCGroundRule*){
		start = false;
		start = false;
		return;
	}
	virtual void visit(const AggGroundRule*){
		start = false;
		return;
	}
	virtual void visit(const GroundSet*){
		start = false;
		return;
	}
	virtual void visit(const GroundAggregate*){
		start = false;
		return;
	}
	virtual void visit(const CPReification*){
		start = false;
		return;
	}
	virtual void visit(const Rule*){
		start = false;
		return;
	}
	virtual void visit(const Definition*){
		start = false;
		return;
	}
	virtual void visit(const FixpDef*){
		start = false;
		return;
	}
	virtual void visit(const CPVarTerm*){
		start = false;
		return;
	}
	virtual void visit(const CPWSumTerm*){
		start = false;
		return;
	}
	virtual void visit(const CPSumTerm*){
		start = false;
		return;
	}
	virtual void visit(const EnumSetExpr*){
		start = false;
		return;
	}
	virtual void visit(const QuantSetExpr*){
		start = false;
		return;
	}
};

#endif /* FINDUNKNBOUND_HPP_ */
