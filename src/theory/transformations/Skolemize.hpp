/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef SKOLEMIZE_HPP_
#define SKOLEMIZE_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"
#include "structure/StructureComponents.hpp"

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
	// FIXME changes vocabulary! => is this safe?
	// FIXME changes vocabulary! => should also adapt structures over the vocabulary
	// NOTE: changes vocabulary and theory
	template<typename T>
	T execute(T t) {
		vocabulary = t->vocabulary();
		auto result = t->accept(this);
		return result;
	}
protected:
	Theory* visit(Theory* t){
		// TODO dangerous code: adding more constructs to the theory will skip them here!
		for(auto i=t->definitions().begin(); i<t->definitions().end(); ++i){
			quantified.clear();
			replace.clear();
			*i = (*i)->accept(this);
		}
		for(auto i=t->sentences().begin(); i<t->sentences().end(); ++i){
			quantified.clear();
			replace.clear();
			*i = (*i)->accept(this);
		}
		for(auto i=t->fixpdefs().begin(); i<t->fixpdefs().end(); ++i){
			quantified.clear();
			replace.clear();
			*i = (*i)->accept(this);
		}
		return t;
	}

	Term* visit(VarTerm* vt){
		auto it = replace.find(vt->var());
		if(it!=replace.cend()){
			return it->second;
		}
		return vt;
	}

	Formula* visit(QuantForm* qf){
		Assert(qf->sign()==SIGN::POS); // FIXME require pushed negations!
		if(qf->isUniv()){
			quantified.insert(quantified.end(), qf->quantVars().cbegin(), qf->quantVars().cend());
			return qf;
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
