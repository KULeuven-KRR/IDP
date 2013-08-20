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

#pragma once

#include <vector>
#include <cstdlib>
#include <memory>

#include "IncludeComponents.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "creation/cppinterface.hpp"

// TODO add markers for structure information
// TODO print out original formula

class AddMarkers: public TheoryMutatingVisitor {
	VISITORFRIENDS()

	std::vector<Variable*> vars;
	std::vector<Predicate*> newpreds;
	std::map<Predicate*, std::pair<std::vector<Variable*>, Formula*>> marker2formula;
	std::map<Predicate*, std::pair<std::vector<Variable*>, Rule*>> marker2rule;
	std::map<Rule*, DefId> rule2defid;

public:
	Theory* execute(Theory* t) {
		auto newt = t->accept(this);
		for(auto p: newpreds){
			newt->vocabulary()->add(p);
		}
		return newt;
	}

	~AddMarkers(){
		for(auto m2form: marker2formula){
		//	m2form.second.second->recursiveDelete();
		}
		for(auto m2rule: marker2rule){
		//	m2rule.second.second->recursiveDelete();
		}
		//deleteList(newpreds);
	}

	const std::vector<Predicate*>& getMarkers() const {
		return newpreds;
	}
	std::vector<TheoryComponent*> getComponentsFromMarkers(const std::vector<PredForm*>& pfs) const {
		std::vector<Formula*> forminstances;
		std::map<DefId, std::vector<Rule*>> ruleinstances;

		std::vector<TheoryComponent*> core;
		for(auto pf: pfs){
			auto pred = dynamic_cast<Predicate*>(pf->symbol());
			if(contains(marker2formula, pred)){
				auto varAndForm = marker2formula.at(pred);
				std::map<Variable*, const DomainElement*> var2elems;
				for(uint i=0; i<pf->args().size(); ++i){
					var2elems[varAndForm.first[i]]=dynamic_cast<DomainTerm*>(pf->args()[i])->value();
				}
				core.push_back(FormulaUtils::substituteVarWithDom(varAndForm.second->cloneKeepVars(), var2elems));
			}else if(contains(marker2rule, pred)){
				auto varAndForm = marker2rule.at(pred);
				std::map<Variable*, const DomainElement*> var2elems;
				for(uint i=0; i<pf->args().size(); ++i){
					var2elems[varAndForm.first[i]]=dynamic_cast<DomainTerm*>(pf->args()[i])->value();
				}
				auto head = FormulaUtils::substituteVarWithDom(varAndForm.second->head()->cloneKeepVars(), var2elems);
				auto body = FormulaUtils::substituteVarWithDom(varAndForm.second->body()->cloneKeepVars(), var2elems);
				ruleinstances[rule2defid.at(varAndForm.second)].push_back(new Rule({}, dynamic_cast<PredForm*>(head), body, {}));
			}else{
				core.push_back(pf);
			}
		}
		for(auto id2rules: ruleinstances){
			auto def = new Definition();
			def->add(id2rules.second);
			core.push_back(def->clone());
		}
		return core;
	}
protected:
	Formula* addMarker(Formula* f){
		std::vector<Sort*> sorts;
		for(auto var:vars){
			sorts.push_back(var->sort());
		}
		auto p = new Predicate(sorts);
		newpreds.push_back(p);
		auto atom = &Gen::atom(p, vars);
		marker2formula[p]={vars, f};
		return &Gen::disj({f, atom});
	}
	Formula* visit(PredForm* pf){
		return addMarker(pf);
	}
	Formula* visit(AggForm* f){
		return addMarker(f);
	}
	Formula* visit(EqChainForm* f){
		return addMarker(f);
	}
	Formula* visit(EquivForm* f){
		return addMarker(f);
	}

	Formula* visit(BoolForm* bf){
		if(bf->isConjWithSign()){
			return traverse(bf);
		}else{
			return addMarker(bf);
		}
	}
	Formula* visit(QuantForm* q){
		if(q->isUnivWithSign()){
			auto tempvars = vars;
			vars.insert(vars.end(), q->quantVars().cbegin(), q->quantVars().cend());
			q->subformula(q->subformula()->accept(this));
			vars = tempvars;
			return q;
		}else{
			return addMarker(q);
		}
	}
	Definition* visit(Definition* d){
		auto rules = d->rules();
		for(auto r: d->rules()){
			auto h = r->head();
			auto b = r->body();

			// H <- B ===> H <- ~M1 & (M2 | B)

			std::vector<Sort*> sorts;
			std::vector<Variable*> vars;
			for(auto var:r->quantVars()){
				sorts.push_back(var->sort());
				vars.push_back(var);
			}
			auto conjp = new Predicate(sorts);
			auto disjp = new Predicate(sorts);
			newpreds.push_back(disjp);
			newpreds.push_back(conjp);
			auto conjmarker = &Gen::operator !(Gen::atom(conjp, vars));
			auto disjmarker = &Gen::atom(disjp, vars);
			auto rc1 = new Rule({}, r->head()->cloneKeepVars(), r->body()->cloneKeepVars(),{}), rc2 = new Rule({}, r->head()->cloneKeepVars(), r->body()->cloneKeepVars(),{});
			rule2defid[rc1] = d->getID();
			rule2defid[rc2] = d->getID();
			marker2rule[conjp]={vars, rc1};
			marker2rule[disjp]={vars, rc2};
			r->body(&Gen::conj({conjmarker, &Gen::disj({disjmarker, r->body()})}));
		}
		d->rules(rules);
		return d;
	}

	virtual AbstractGroundTheory* visit(AbstractGroundTheory*){
		throw IdpException("Invalid code path");
	}

	virtual GroundDefinition* visit(GroundDefinition*){
		throw IdpException("Invalid code path");
	}
	virtual GroundRule* visit(AggGroundRule*){
		throw IdpException("Invalid code path");
	}
	virtual GroundRule* visit(PCGroundRule*){
		throw IdpException("Invalid code path");
	}

	virtual Rule* visit(Rule*){
		throw IdpException("Invalid code path");
	}
	virtual FixpDef* visit(FixpDef*){
		throw IdpException("Invalid code path");
	}

	virtual Term* visit(VarTerm*){
		throw IdpException("Invalid code path");
	}
	virtual Term* visit(FuncTerm*){
		throw IdpException("Invalid code path");
	}
	virtual Term* visit(DomainTerm*){
		throw IdpException("Invalid code path");
	}
	virtual Term* visit(AggTerm*){
		throw IdpException("Invalid code path");
	}

	virtual EnumSetExpr* visit(EnumSetExpr*){
		throw IdpException("Invalid code path");
	}
	virtual QuantSetExpr* visit(QuantSetExpr*){
		throw IdpException("Invalid code path");
	}
};

class UnsatCoreExtraction {
public:
	static std::vector<TheoryComponent*> extractCore(AbstractTheory* atheory, Structure* structure){
		auto theory = dynamic_cast<Theory*>(atheory);
		if(theory==NULL){
			throw notyetimplemented("Can only handle theories");
		}
		auto voc = new Vocabulary("test");
		voc->add(theory->vocabulary());

		auto newtheory = theory->clone();
		newtheory->vocabulary(voc);

		auto s = structure->clone();
		s->changeVocabulary(newtheory->vocabulary()); // TODO theory voc?

		std::map<Function*, Formula*>func2form;
		FormulaUtils::addFuncConstraints(newtheory, newtheory->vocabulary(), func2form, getOption(CPSUPPORT));
		for(auto f2f: func2form){
			newtheory->add(f2f.second);
		}

		for(auto def: newtheory->definitions()){
			for(auto p : def->defsymbols()){
				varset vars;
				std::vector<Term*> varlist;
				for(uint i=0; i<p->sorts().size(); ++i){
					auto var = new Variable(p->sort(i));
					vars.insert(var);
					varlist.push_back(new VarTerm(var, {}));
				}
				def->add(new Rule(vars, new PredForm(SIGN::POS, p, varlist, {}), FormulaUtils::falseFormula(), {}));
			}
		}

		AddMarkers am;
		newtheory = am.execute(newtheory);

		auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL, am.getMarkers()); // TODO no function constraints
		if(not mxresult.unsat){
			throw IdpException("The given theory has models that extend the structure, so there are no unsat cores.");
		}

		auto coreresult = am.getComponentsFromMarkers(mxresult.unsat_in_function_of_ct_lits);
		newtheory->recursiveDelete();
		delete(s);
		delete(voc);
		return coreresult;

	}
};
