/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef REMOVETHREEVALUEDTERMS_HPP_
#define REMOVETHREEVALUEDTERMS_HPP_

#include "IncludeComponents.hpp"
#include "UnnestTerms.hpp"
#include "utils/CPUtils.hpp"

class Vocabulary;
class Variable;
class AbstractStructure;
class PFSymbol;
class Formula;
class PredForm;
class Term;

/**
 *	Moves terms that are three-valued according to a given structure
 *	outside a given atom. The applied rewriting depends on the given context:
 *		- positive context:
 *			P(t) becomes	! x : t = x => P(x).
 *		- negative context:
 *			P(t) becomes	? x : t = x & P(x).
 *	The fact that the rewriting is non-recursive means that in the above example, term t
 *	can still contain terms that are three-valued according to the structure.
 *
 *	\param t			given object
 *	\param str			given structure
 *	\param context		given context
 *
 *	\return The rewritten formula. If no rewriting was needed, it is the same pointer as t.
 *	If rewriting was needed, t can be deleted, but not recursively (TODO).
 *
 */
class UnnestThreeValuedTerms: public UnnestTerms {
	VISITORFRIENDS()
private:
	bool _cpsupport;
	bool _allowedToKeepNested;

public:
	template<typename T>
	T execute(T t, AbstractStructure* str, Context context, bool cpsupport, bool nestingIsAllowed = false) {
		_structure = str;
		_vocabulary = (str != NULL) ? str->vocabulary() : NULL;
		setContext(context);
		setAllowedToUnnest(false);
		setNestingIsAllowed(nestingIsAllowed);
		_cpsupport = cpsupport;
		return t->accept(this);
	}

protected:
	bool shouldMove(Term*);
	Formula* visit(PredForm*);
	Rule* visit(Rule*);
	Term* visit(AggTerm*);

private:
	bool nestingIsAllowed() const {
		return _allowedToKeepNested;
	}
	void setNestingIsAllowed(bool allowed) {
		_allowedToKeepNested = allowed;
	}
};

#endif /* REMOVETHREEVALUEDTERMS_HPP_ */
