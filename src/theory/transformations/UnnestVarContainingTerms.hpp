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

#ifndef UNNNESTTERMSCONTAININGVARS_HPP_
#define UNNNESTTERMSCONTAININGVARS_HPP_

#include "IncludeComponents.hpp"
#include "UnnestTerms.hpp"

#include <set>

class Vocabulary;
class Variable;
class Structure;
class PFSymbol;
class Formula;
class PredForm;
class Term;

/**
 *	Non-recursively moves terms out of heads of rules that contain variables outside a given atom.
 *
 *	@returns The rewritten formula. If no rewriting was needed, it is the same pointer as pf.
 *	If rewriting was needed, pf can be deleted, but not recursively.
 */
class UnnestHeadTermsNotVarsOrDomElems: public UnnestTerms {
	VISITORFRIENDS()
private:
	bool inhead;
public:
	template<typename T>
	T execute(T t, const Structure* str) {
		_structure = str;
		_vocabulary = (str != NULL) ? str->vocabulary() : NULL;
		setAllowedToUnnest(false);
		auto result = t->accept(this);
		return result;
	}

protected:
	virtual Rule* visit(Rule* rule){
		auto saveallowed = isAllowedToUnnest();
		setAllowedToUnnest(true);
		inhead = true;
		visitRuleHead(rule);
		inhead = false;
		setAllowedToUnnest(saveallowed);
		return rule;
	}

	bool wouldMove(Term* t){
		return inhead && t->type()!=TermType::DOM && t->type()!=TermType::VAR;
	}
};

#endif /* UNNNESTTERMSCONTAININGVARS_HPP_ */
