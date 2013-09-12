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

#include "IncludeComponents.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

class Structure;
class Vocabulary;
class Formula;
class Variable;
class Sort;
class Term;

/**
 * Moves nested terms out
 */
class UnnestTerms: public TheoryMutatingVisitor {
	VISITORFRIENDS()
protected:
	const Structure* _structure; 	// Used to find bounds on introduced variables
	Vocabulary* _vocabulary; 				// Used to do type derivation during rewrites

private:
	bool _onlyrulehead;
	bool _allowedToUnnest; 				// Indicates whether in the current context, it is allowed to unnest terms

	// Temporary information
	Sort* _chosenVarSort;				// Sort for the newly introduced variable
	std::vector<Formula*> _equalities; 	// generated equalities
	varset _variables; 					// newly introduced variables

protected:
	virtual bool wouldMove(Term* t);
	bool shouldMove(Term* t){
		return isAllowedToUnnest() && wouldMove(t);
	}

	bool isAllowedToUnnest() const {
		return _allowedToUnnest;
	}
	void setAllowedToUnnest(bool allowed) {
		_allowedToUnnest = allowed;
	}

public:
	UnnestTerms();

	template<typename T>
	T execute(T t, const Structure* str = NULL, Vocabulary* voc = NULL, bool onlyrulehead = false) {
		_structure = str;
		_vocabulary = voc;
		_allowedToUnnest = false;
		_onlyrulehead = onlyrulehead;
		return t->accept(this);
	}

protected:
	template<typename T>
	Formula* doRewrite(T origformula) {
		auto rewrittenformula = rewrite(origformula);
		if (rewrittenformula == origformula) {
			return origformula;
		} else {
			return rewrittenformula->accept(this);
		}
	}

	Term* doMove(Term* term) {
		if (shouldMove(term)) {
			return move(term);
		}
		return term;
	}

	Theory* visit(Theory*);
	virtual Rule* visit(Rule*);
	virtual Formula* traverse(Formula*);
	virtual Formula* visit(EquivForm*);
	virtual Formula* visit(AggForm*);
	virtual Formula* visit(EqChainForm*);
	virtual Formula* visit(PredForm*);
	virtual Term* traverse(Term*);
	VarTerm* visit(VarTerm*);
	virtual Term* visit(DomainTerm*);
	virtual Term* visit(AggTerm*);
	virtual Term* visit(FuncTerm*);
	virtual EnumSetExpr* visit(EnumSetExpr*);
	virtual QuantSetExpr* visit(QuantSetExpr*);

	void visitRuleHead(Rule* rule); // Split to allow reuse
	Formula* unnest(PredForm* predform);

private:
	Formula* rewrite(Formula*);
	Term* move(Term*, Sort* newsort = NULL);

	void visitTermRecursive(Term* term);

	Sort* deriveSort(Term*);
};
