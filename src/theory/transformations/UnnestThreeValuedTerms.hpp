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

#include <set>

class Vocabulary;
class Variable;
class AbstractStructure;
class PFSymbol;
class Formula;
class PredForm;
class Term;

/**
 *	Non-recursively moves terms that are three-valued according to a given structure
 *	outside a given atom. The applied rewriting depends on the given context:
 *		- positive context:
 *			P(t) becomes	! x : t = x => P(x).
 *		- negative context:
 *			P(t) becomes	? x : t = x & P(x).
 *	The fact that the rewriting is non-recursive means that in the above example, term t
 *	can still contain terms that are three-valued according to the structure.
 *
 *	\param pf			the given atom
 *	\param str			the given structure
 *	\param Context	true iff we are in a positive context
 *	\param usingcp
 *
 *	\return The rewritten formula. If no rewriting was needed, it is the same pointer as pf.
 *	If rewriting was needed, pf can be deleted, but not recursively.
 *
 */
class UnnestThreeValuedTerms: public UnnestTerms {
	VISITORFRIENDS()
private:
	bool _cpsupport;
	std::set<const PFSymbol*> _cpsymbols;

public:
	template<typename T>
	T execute(T t, AbstractStructure* str, Context context, bool cpsupport, const std::set<const PFSymbol*>& cpsymbols) {
		_structure = str;
		_vocabulary = (str != NULL) ? str->vocabulary() : NULL;
		setContext(context);
		setAllowedToUnnest(false);
		_cpsupport = cpsupport;
		_cpsymbols = cpsymbols;
		return t->accept(this);
		//return UnnestTerms::execute(t, context, str, str->vocabulary());
	}

protected:
	bool shouldMove(Term* t);

	Formula* traverse(PredForm* f);
	Rule* traverse(Rule* r); //TODO should this have non-standard behavior?

private:
	bool isCPSymbol(const PFSymbol* symbol) const;
};

#endif /* REMOVETHREEVALUEDTERMS_HPP_ */
