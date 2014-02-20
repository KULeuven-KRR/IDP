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

#ifndef DERIVESORTS_HPP_
#define DERIVESORTS_HPP_

#include <set>
#include <map>
#include <cstddef>

#include "visitors/TheoryMutatingVisitor.hpp"
#include "vocabulary/VarCompare.hpp"

class Sort;
class Variable;
class Vocabulary;

/**
 * First visit:
 * 	find all untyped vars and domainelements, and all variables occurring in overloaded positions except builtins
 * 	derive all untypes vars from the positions they occur in
 * Afterwards:
 * 	fixpoint over equality, taking the resolvent of both sides and assigning it to both
 *
 * 	TODO domelem = domelem ?
 */

class DeriveSorts: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	varset _underivableVariables;
	varset _untypedvariables;
	std::set<PredForm*> _overloadedatoms;
	std::set< std::pair<FuncTerm*, Sort*>> _overloadedterms; //The overloaded terms, together with their expected output argument (derived during top-down run).
	std::set<DomainTerm*> _domelements; // The untyped domain elements
	bool _changed, _firstvisit, _underivable;
	Sort* _assertsort;
	const Vocabulary* _vocab;
	bool _useBuiltIns;
	std::map<Variable*,std::set<Sort*>> derivations;

public:
	template<typename T>
	//Note: always run this twice: first with useBuiltins false, then with useBuiltIns true.
	//This allows for better type-derivation.
	void execute(T f, Vocabulary* v, bool useBuiltIns) {
		_useBuiltIns = useBuiltIns;
		_assertsort = NULL;
		_vocab = v;
		deriveSorts(f);
	}

protected:
	Formula* visit(QuantForm*);
	Formula* visit(PredForm*);
	Formula* visit(EqChainForm*);
	Rule* visit(Rule*);
	Term* visit(VarTerm*);
	Term* visit(DomainTerm*);
	Term* visit(AggTerm* t);
	Term* visit(FuncTerm*);
	QuantSetExpr* visit(QuantSetExpr*);

private:
	template<typename T>
	void deriveSorts(T f) {
		_underivable = false;
		_changed = true;
		_firstvisit = true;
		run(f); // First visit: collect untyped symbols, set types of variables that occur in typed positions.

		_firstvisit = false;
		while (_changed) {
			_changed = false;
			run(f);
		}
		check();
	}

	template<typename T>
	void run(T f) {
		_underivableVariables.clear();
		f->accept(this);
		derivefuncs();
		derivepreds();
	}

	void derivefuncs(); // disambiguate the overloaded functions
	void derivepreds(); // disambiguate the overloaded predicates

	void checkVars(const varset& quantvars);

	void check();
};

class Rule;
template<> void DeriveSorts::execute(Rule* r, Vocabulary* v, bool useBuiltins);

#endif /* DERIVESORTS_HPP_ */
