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

/**
 * Preliminary version of an interface to easily create vocs, theories and structures from c++
 *
 * TODO currently loses all available parseinfo!
 *
 * It allows for example the following declaration:
 auto s = sort("x", -2, 2);
 auto p = pred("p", {s});
 auto q = pred("q", {s});
 auto r = pred("r", {s});

 auto xt = varterm(s);
 auto x = xt->var();

 auto vocabulary = new Vocabulary("V");
 add(vocabulary, {p.p(), q.p(), r.p()});

 auto theory = new Theory("T", vocabulary, ParseInfo());
 Formula& formula = all(x, (not p({x})) | q({x})) & all(x, (not q({x})) | r({x}));
 */

#include <string>
#include <set>
#include <vector>
#include "vocabulary/VarCompare.hpp"
#include "IncludeComponents.hpp"

class DomainTerm;
class Variable;
class VarTerm;
class EnumSetExpr;
class Term;
class AggTerm;
class PredTable;

namespace Gen {

Sort* sort(const std::string& name, int min, int max);

const DomainElement* domainelement(int value);
DomainTerm* domainterm(Sort*, int value);

Variable* var(Sort*);
VarTerm* varterm(Sort*);

EnumSetExpr* qset(const varset&, Formula&, Term*);
AggTerm* sum(EnumSetExpr*);

Term& functerm(Function*, const std::vector<Variable*>&);
Term& functerm(Function*, const std::vector<Term*>&);

Formula& operator==(Term& left, Term& right);
Formula& operator<(Term& left, Term& right);

Formula& operator&(Formula& left, Formula& right);
Formula& operator|(Formula& left, Formula& right);
Formula& operator not(Formula&);

Formula& disj(const std::vector<Formula*>& formulas);
Formula& conj(const std::vector<Formula*>& formulas);

Formula& forall(Variable*, Formula&);
Formula& exists(Variable*, Formula&);
Formula& forall(const varset&, Formula&);
Formula& exists(const varset& vars, Formula& formula);
PredForm& atom(PFSymbol*, const std::vector<Variable*>&);
PredForm& atom(PFSymbol*, const ElementTuple&);

PredTable* predtable(const SortedElementTable& table, const Universe& universe);

void add(Vocabulary*, const std::vector<Sort*>);
void add(Vocabulary*, const std::vector<PFSymbol*>);

class PredWrapper {
private:
	Predicate* _p;
public:
	PredWrapper(Predicate* p) : _p(p) { }
	PredForm& operator()(const std::vector<Variable*>& vars) {
		return atom(_p, vars);
	}
	Predicate* p() const {
		return _p;
	}
};

class FuncWrapper {
private:
	Function* _f;
public:
	FuncWrapper(Function* f) : _f(f) { }
	Term& operator()(const std::vector<Variable*>& vars) {
		return functerm(_f, vars);
	}
	Term& operator()(const std::vector<Term*>& terms) {
		return functerm(_f, terms);
	}
	Function* f() const {
		return _f;
	}
};

PredWrapper pred(const std::string& name, const std::vector<Sort*>& sorts);
FuncWrapper func(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort);

} /* namespace Tests */
