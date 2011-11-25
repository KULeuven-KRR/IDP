#ifndef CPPINTERFACE_HPP_
#define CPPINTERFACE_HPP_

#include "common.hpp"
#include "vocabulary.hpp"
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
	return new Sort(name, sorttable);
}

VarTerm* varterm(Sort* s) {
	auto variable = new Variable(s);
	return new VarTerm(variable, TermParseInfo());
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
	PredWrapper(Predicate* p) :
			_p(p) {

	}
	Formula& operator()(const std::vector<Variable*>& vars) {
		return atom(_p, vars);
	}
	Predicate* p() const {
		return _p;
	}
};

PredWrapper pred(const std::string& name, const std::vector<Sort*>& sorts) {
	return PredWrapper(new Predicate(name, sorts, false));
}

#endif /* CPPINTERFACE_HPP_ */
