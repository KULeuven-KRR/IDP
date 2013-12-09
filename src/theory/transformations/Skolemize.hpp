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

#ifndef SKOLEMIZE_HPP_
#define SKOLEMIZE_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"
#include "utils/ListUtils.hpp"

/**
 * Replaced an atom with function occurrences with a new propositional symbol and an equivalence.
 * Replace multiple occurrences with the same propositional symbol!
 */
class Skolemize: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::vector<Variable*> quantified;
	std::map<Variable*, Term*> replace;
	Vocabulary* vocabulary;

public:
	// FIXME only correct when calling with a monotone FO formula which is not quantified externally!
	// FIXME changes vocabulary! => is this safe?
	// NOTE: changes vocabulary and theory
	template<typename T>
	T execute(T t, Vocabulary* v) {
		vocabulary = v;
		auto result = t->accept(this);
		return result;
	}
protected:
	Theory* visit(Theory* t){
		auto components = t->sentences();
		t->sentences().clear();
		for(auto c:components){
			quantified.clear();
			replace.clear();
			t->add(c->accept(this));
		}
		return t;
	}

	Rule* visit(Rule* r){
		return r;
	}
	FixpDef* visit(FixpDef* d){
		return d;
	}

	EquivForm* visit(EquivForm* eq){
		return eq;
	}

	Term* visit(VarTerm* vt){
		auto it = replace.find(vt->var());
		if(it!=replace.cend()){
			return it->second->clone();
		}
		return vt;
	}

	Formula* visit(QuantForm* qf){
		Assert(qf->sign()==SIGN::POS); // FIXME require pushed negations!
		if(qf->isUniv()){
			insertAtEnd(quantified, qf->quantVars());
			return traverse(qf);
		}else{
			for(auto i=qf->quantVars().cbegin(); i!=qf->quantVars().cend(); ++i) {
				std::vector<Term*> varterms;
				std::vector<Sort*> sorts;
				for(auto j=quantified.cbegin(); j<quantified.cend(); ++j) {
					varterms.push_back(new VarTerm(*j, TermParseInfo())); // TODO correct parse info?
					sorts.push_back((*j)->sort());
				}
				auto func = new Function(sorts, (*i)->sort(), ParseInfo()); // TODO correct parse info?
				vocabulary->add(func);
				replace.insert({*i, new FuncTerm(func, varterms, TermParseInfo())}); // TODO correct parse info?
			}
			return qf->subformula()->accept(this);
		}
	}
};

#endif /* SKOLEMIZE_HPP_ */
