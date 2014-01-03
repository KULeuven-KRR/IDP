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

class RemoveValidQuantifications: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	Structure* _structure;
	bool _assumeTypesNotEmpty;

public:
	template<typename T>
	T execute(T t, bool assumeTypesNotEmpty) {
		_assumeTypesNotEmpty = assumeTypesNotEmpty;
		_structure = NULL;
		t = t->accept(this);
		return t;
	}
	template<typename T>
	T execute(T t, Structure* structure) {
		_assumeTypesNotEmpty = false;
		_structure = structure;
		t = t->accept(this);
		return t;
	}

protected:
	Formula* visit(QuantForm* qf){
		if(not _assumeTypesNotEmpty && _structure==NULL){
			return qf;
		}
		qf->subformula(qf->subformula()->accept(this));
		varset remainingvars;
		for(auto var: qf->quantVars()){
			if(contains(qf->subformula()->freeVars(), var) || var->sort()==NULL
					|| (_structure!=NULL && _structure->inter(var->sort())->empty())){
				remainingvars.insert(var);
			}
		}

		if(remainingvars.empty()){
			return qf->subformula()->cloneKeepVars();
		}
		if(remainingvars.size()<qf->quantVars().size()){
			varset newvars;
			std::map<Variable*, Variable*> var2var;
			for(auto var: remainingvars){
				auto newvar = new Variable(var->sort());
				newvars.insert(newvar);
				var2var[var] = newvar;
			}

			return new QuantForm(qf->sign(), qf->quant(), newvars, qf->subformula()->clone(var2var), FormulaParseInfo());
		}
		return qf;
	}

	Rule* visit(Rule* rule){
		if(not _assumeTypesNotEmpty && _structure==NULL){
			return rule;
		}
		rule->body(rule->body()->accept(this));
		varset remainingvars;
		for(auto var: rule->quantVars()){
			if(contains(rule->head()->freeVars(), var) || contains(rule->body()->freeVars(), var)
					|| var->sort()==NULL || (_structure!=NULL && _structure->inter(var->sort())->empty())){
				remainingvars.insert(var);
			}
		}
		rule->setQuantVars(remainingvars);
		return rule;
	}
};
