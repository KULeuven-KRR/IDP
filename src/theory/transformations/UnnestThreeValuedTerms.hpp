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

#ifndef REMOVETHREEVALUEDTERMS_HPP_
#define REMOVETHREEVALUEDTERMS_HPP_

#include "IncludeComponents.hpp"
#include "UnnestTerms.hpp"
#include "utils/CPUtils.hpp"

class Vocabulary;
class Variable;
class Structure;
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
 *	For CP, when should we NOT unnest:
 *	For any type of relation which can be translated completely into CP!
 *	 => pred(f(t)...)
 *	 		if pred and f are cp-able, only unnest deeper than f
 *	 => agg(S) op t
 *	 		if agg with given functerms are cpable
 *
 */

class UnnestThreeValuedTerms: public UnnestTerms {
	VISITORFRIENDS()
private:
	std::set<PFSymbol*> _definedsymbols;
	bool _cpsupport;

	TruthValue _cpablerelation;
	bool _cpablefunction;

public:
	template<typename T>
	T execute(T t,const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool cpsupport, TruthValue cpablerelation = TruthValue::Unknown) {
		Assert(str!=NULL);
		_definedsymbols = definedsymbols;
		_structure = str;
		_vocabulary = str->vocabulary();
		setAllowedToUnnest(false);
		_cpablerelation = cpablerelation;
		_cpablefunction = false;
		_cpsupport = cpsupport;
		return t->accept(this);
	}

protected:
	bool wouldMove(Term*);
	Formula* visit(PredForm*);
	Formula* visit(AggForm*);
	Term* visit(AggTerm*);
	Term* visit(FuncTerm*);
	Rule* visit(Rule* r);
};

#endif /* REMOVETHREEVALUEDTERMS_HPP_ */
