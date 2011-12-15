/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CPPINTERFACE_HPP_
#define CPPINTERFACE_HPP_

#include "common.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "structure.hpp"

/**
 * Preliminary version of an interface to easily create vocs, theories and structures from c++
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

Sort* sort(const std::string& name, int min, int max) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(min, max));
	auto sort = new Sort(name, sorttable);
	sort->addParent(VocabularyUtils::intsort());
	return sort;
}

const DomainElement* domainelement(int v) {
	return DomainElementFactory::createGlobal()->create(v);
}

DomainTerm* domainterm(Sort* s, int v) {
	auto domainelement = DomainElementFactory::createGlobal()->create(v);
	return new DomainTerm(s,domainelement,TermParseInfo());
}

Variable* var(Sort* s) {
	return new Variable(s);
}

VarTerm* varterm(Sort* s) {
	auto variable = new Variable(s);
	return new VarTerm(variable, TermParseInfo());
}

Term& functerm(Function* f, const std::vector<Variable*>& vars) {
	std::vector<Term*> terms;
	for (auto i = vars.cbegin(); i < vars.cend(); ++i) {
		terms.push_back(new VarTerm(*i, TermParseInfo()));
	}
	return *new FuncTerm(f, terms, TermParseInfo());
}

Term& functerm(Function* f, const std::vector<Term*>& terms) {
	return *new FuncTerm(f, terms, TermParseInfo());
}

Formula& operator==(Term& left, Term& right) {
	auto sort = SortUtils::resolve(left.sort(),right.sort());
	Assert(sort != NULL);
	auto eq = VocabularyUtils::equal(sort);
	return *new PredForm(SIGN::POS, eq, { &left, &right }, FormulaParseInfo());
}

Formula& operator&(Formula& left, Formula& right) {
	return *new BoolForm(SIGN::POS, true, &left, &right, FormulaParseInfo());
}

Formula& operator|(Formula& left, Formula& right) {
	return *new BoolForm(SIGN::POS, false, &left, &right, FormulaParseInfo());
}

Formula& operator not(Formula& f) {
	f.negate();
	return f;
}

Formula& all(Variable* var, Formula& formula) {
	return *new QuantForm(SIGN::POS, QUANT::UNIV, { var }, &formula, FormulaParseInfo());
}

Formula& atom(Predicate* p, const std::vector<Variable*>& vars) {
	std::vector<Term*> terms;
	for (auto i = vars.cbegin(); i < vars.cend(); ++i) {
		terms.push_back(new VarTerm(*i, TermParseInfo()));
	}
	return *new PredForm(SIGN::POS, p, terms, FormulaParseInfo());
}

void add(Vocabulary* v, const std::vector<PFSymbol*> symbols) {
	for (auto i = symbols.cbegin(); i < symbols.cend(); ++i) {
		v->add(*i);
	}
}

class PredWrapper {
private:
	Predicate* _p;
public:
	PredWrapper(Predicate* p) : _p(p) { }
	Formula& operator()(const std::vector<Variable*>& vars) {
		return atom(_p, vars);
	}
	Predicate* p() const {
		return _p;
	}
};

PredWrapper pred(const std::string& name, const std::vector<Sort*>& sorts) {
	return PredWrapper(new Predicate(name, sorts));
}

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

FuncWrapper func(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort) {
	return FuncWrapper(new Function(name, insorts, outsort)); 
}

#endif /* CPPINTERFACE_HPP_ */
