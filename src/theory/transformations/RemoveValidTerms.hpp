/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

#include "visitors/TheoryMutatingVisitor.hpp"
#include "IncludeComponents.hpp"

class RemoveValidTerms: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:

public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}

protected:
	bool isTrue(const Formula* f) const {
		auto bf = dynamic_cast<const BoolForm*>(f);
		return bf!=NULL && bf->subformulas().size()==0 && bf->isConjWithSign();
	}
	bool isFalse(const Formula* f) const {
		auto bf = dynamic_cast<const BoolForm*>(f);
		return bf!=NULL && bf->subformulas().size()==0 && not bf->isConjWithSign();
	}

	bool isIdentical(bool eq, Term* left, Term* right){
		if(left->type()!=right->type()){
			return false;
		}
		switch(left->type()){
		case TermType::VAR:
			return dynamic_cast<VarTerm*>(left)->var()==dynamic_cast<VarTerm*>(right)->var();
		case TermType::FUNC:{
			auto lft = dynamic_cast<FuncTerm*>(left);
			auto rft = dynamic_cast<FuncTerm*>(right);
			if(lft->function()!=rft->function() || (lft->function()->partial() && not eq)){
				return false;
			}
			bool allidentical = true;
			for(uint i=0; i<lft->args().size(); ++i){
				if(not isIdentical(eq, lft->subterms()[i], rft->subterms()[i])){
					allidentical = false; break;
				}
			}
			return allidentical;}
		case TermType::DOM:
			return dynamic_cast<DomainTerm*>(left)->value()==dynamic_cast<DomainTerm*>(right)->value();
		case TermType::AGG:
			return false; // TODO smarter checking?
		default:
			return false;
		}
	}

	Formula* getFormula(bool trueform){
		return new BoolForm(SIGN::POS, trueform, { }, FormulaParseInfo());
	}

	Formula* visit(PredForm* pf){
		if(VocabularyUtils::isPredicate(pf->symbol(), STDPRED::EQ) && isIdentical(pf->sign()==SIGN::POS, pf->subterms()[0], pf->subterms()[1])){
			return getFormula(pf->sign()==SIGN::POS);
		}
		return pf;
	}

	Formula* simplify(Formula* f){
		if(isTrue(f) || isFalse(f)){
			return f;
		}
		auto bf = dynamic_cast<BoolForm*>(f);
		if(bf!=NULL){
			auto sometrue = false, somefalse = false;
			std::vector<Formula*> subforms;
			for(auto subform: bf->subformulas()){
				auto sf = simplify(subform);
				if(isTrue(sf)){
					sometrue = true;
					continue;
				}else if(isFalse(sf)){
					somefalse = true;
					continue;
				}
				subforms.push_back(sf);
			}
			if(sometrue && not bf->isConjWithSign()){
				return getFormula(true);
			}else if(somefalse && bf->isConjWithSign()){
				return getFormula(false);
			}else{
				bf->subformulas(subforms);
			}
			return bf;
		}
		auto qf = dynamic_cast<QuantForm*>(f);
		if(qf!=NULL){
			auto subform = simplify(qf->subformula());
			if(isTrue(subform)){
				return subform;
			}else if(isFalse(subform)){
				return subform;
			}else{
				qf->subformula(subform);
				return qf;
			}
		}
		// TODO handle more?
		return f;
	}

	Formula* visit(BoolForm* bf){
		auto f = traverse(bf);
		return simplify(f);
	}

	Formula* visit(QuantForm* qf){
		auto f = traverse(qf);
		return simplify(f);
	}
};
