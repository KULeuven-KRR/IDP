/************************************
	DeriveSorts.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef DERIVESORTS_HPP_
#define DERIVESORTS_HPP_

#include <set>
#include <map>

#include "visitors/TheoryMutatingVisitor.hpp"

class Sort;
class Variable;
class Vocabulary;

class DeriveSorts: public TheoryMutatingVisitor {
private:
	std::map<Variable*, std::set<Sort*> > _untyped; // The untyped variables, with their possible types
	std::map<FuncTerm*, Sort*> _overloadedterms; // The terms with an overloaded function
	std::set<PredForm*> _overloadedatoms; // The atoms with an overloaded predicate
	std::set<DomainTerm*> _domelements; // The untyped domain elements
	bool _changed;
	bool _firstvisit;
	Sort* _assertsort;
	Vocabulary* _vocab;

public:
	DeriveSorts(Formula* f, Vocabulary* v) :
			_vocab(v) {
		deriveSorts(f);
	}
	DeriveSorts(Rule* r, Vocabulary* v) :
			_vocab(v) {
		deriveSorts(r);
	}
	DeriveSorts(Term* t, Vocabulary* v) :
			_vocab(v) {
		deriveSorts(t);
	}

	Formula* visit(QuantForm*);
	Formula* visit(PredForm*);
	Formula* visit(EqChainForm*);
	Rule* visit(Rule*);
	Term* visit(VarTerm*);
	Term* visit(DomainTerm*);
	Term* visit(FuncTerm*);
	SetExpr* visit(QuantSetExpr*);

private:
	void deriveSorts(Formula*);
	void deriveSorts(Rule*);
	void deriveSorts(Term*);

	void derivesorts(); // derive the sorts of the variables, based on the sorts in _untyped
	void derivefuncs(); // disambiguate the overloaded functions
	void derivepreds(); // disambiguate the overloaded predicates

	void check();
};

#endif /* DERIVESORTS_HPP_ */
