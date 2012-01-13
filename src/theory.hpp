/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef THEORY_HPP
#define THEORY_HPP

#include <set>
#include <vector>
#include "common.hpp"
#include "parseinfo.hpp"
#include "visitors/VisitorFriends.hpp"

/*****************************************************************************
 Abstract base class for formulas, definitions and fixpoint definitions
 *****************************************************************************/

class Function;
class PFSymbol;
class Vocabulary;
class AbstractStructure;
class VarTerm;
class FuncTerm;
class DomainTerm;
class AggTerm;
class EnumSetExpr;
class QuantSetExpr;
class TheoryVisitor;
class TheoryMutatingVisitor;

/**
 *	Abstract base class to represent formulas, definitions, and fixpoint definitions.
 */
class TheoryComponent {
	VISITORFRIENDS()
public:
	// Constructor
	TheoryComponent() {
	}

	// Destructor
	virtual ~TheoryComponent() {
	}
	virtual void recursiveDelete() = 0;

	// Cloning
	virtual TheoryComponent* clone() const = 0; //!< Construct a deep copy of the component

	// Visitor
	virtual void accept(TheoryVisitor*) const = 0;
	virtual TheoryComponent* accept(TheoryMutatingVisitor*) = 0;

	virtual std::ostream& put(std::ostream&) const = 0;
};

std::ostream& operator<<(std::ostream&, const TheoryComponent&);

/***************
 Formulas
 ***************/

/**
 * Abstract base class to represent formulas
 */
class Formula: public TheoryComponent {
ACCEPTDECLAREBOTH(Formula)
private:
	SIGN _sign; //!< the sign of the formula: NEG is that it is negated
	std::set<Variable*> _freevars; //!< the free variables of the formula
	std::set<Variable*> _quantvars; //!< the quantified variables of the formula
	std::vector<Term*> _subterms; //!< the direct subterms of the formula
	std::vector<Formula*> _subformulas; //!< the direct subformulas of the formula
	FormulaParseInfo _pi; //!< the place where the formula was parsed

public:
	// Constructor
	Formula(SIGN sign)
			: _sign(sign) {
	}
	Formula(SIGN sign, const FormulaParseInfo& pi)
			: _sign(sign), _pi(pi) {
	}

	// Virtual constructors
	virtual Formula* clone() const = 0;
	//!< copy the formula while keeping the free variables
	virtual Formula* cloneKeepVars() const = 0;
	//!< copy the formula while keeping all variables
	virtual Formula* clone(const std::map<Variable*, Variable*>&) const = 0;
	//!< copy the formulas, and replace the free variables as indicated by the map

	// Destructor
	void recursiveDelete(); //!< delete the formula and all its children (subformulas, subterms, etc)
	virtual ~Formula() {
	}
	//!< delete the formula, but not its children

	// Mutators
	void negate() {
		_sign = !_sign;
		if (_pi.original())
			_pi.original()->negate();
	}
	//!< swap the sign of the formula

	void addSubterm(Term* t) {
		_subterms.push_back(t);
		setFreeVars();
	}
	void addSubformula(Formula* f) {
		_subformulas.push_back(f);
		setFreeVars();
	}
	void addQuantVar(Variable* v) {
		_quantvars.insert(v);
		setFreeVars();
	}
	void subterm(unsigned int n, Term* t) {
		_subterms[n] = t;
		setFreeVars();
	}
	void subformula(unsigned int n, Formula* f) {
		_subformulas[n] = f;
		setFreeVars();
	}
	void subterms(const std::vector<Term*>& vt) {
		_subterms = vt;
		setFreeVars();
	}
	void subformulas(const std::vector<Formula*>& vf) {
		_subformulas = vf;
		setFreeVars();
	}
	void quantVars(const std::set<Variable*>& sv) {
		_quantvars = sv;
		setFreeVars();
	}

	// Inspectors
	SIGN sign() const {
		return _sign;
	}
	const FormulaParseInfo& pi() const {
		return _pi;
	}
	bool contains(const Variable*) const; //!< true iff the formula contains the variable
	bool contains(const PFSymbol*) const; //!< true iff the formula contains the symbol
	virtual bool trueFormula() const {
		return false;
	}
	//!< true iff the formula is the empty conjunction
	virtual bool falseFormula() const {
		return false;
	}
	//!< true iff the formula is the empty disjunction

	const std::set<Variable*>& freeVars() const {
		return _freevars;
	}
	const std::set<Variable*>& quantVars() const {
		return _quantvars;
	}
	const std::vector<Term*>& subterms() const {
		return _subterms;
	}
	const std::vector<Formula*>& subformulas() const {
		return _subformulas;
	}

	// Output
	virtual std::ostream& put(std::ostream&) const = 0;

private:
	void setFreeVars(); //!< compute the free variables of the formula
};

std::ostream& operator<<(std::ostream&, const Formula&);

/** 
 * Class to represent atomic formulas, that is, a predicate or function symbol applied to a tuple of terms.
 * If F is a function symbol, the atomic formula F(t_1,...,t_n) represents the formula F(t_1,...,t_n-1) = t_n.
 */
class PredForm: public Formula {
ACCEPTBOTH(Formula)
private:
	PFSymbol* _symbol; //!< the predicate or function symbol

public:
	// Constructors
	PredForm(SIGN sign, PFSymbol* s, const std::vector<Term*>& a, const FormulaParseInfo& pi)
			: Formula(sign, pi), _symbol(s) {
		subterms(a);
	}

	PredForm* clone() const;
	PredForm* cloneKeepVars() const;
	PredForm* clone(const std::map<Variable*, Variable*>&) const;

	~PredForm() {
	}

	// Mutators
	void symbol(PFSymbol* s) {
		_symbol = s;
	}
	void arg(unsigned int n, Term* t) {
		subterm(n, t);
	}

	// Inspectors
	PFSymbol* symbol() const {
		return _symbol;
	}
	const std::vector<Term*>& args() const {
		return subterms();
	}

	// Output
	std::ostream& put(std::ostream&) const;
};

/** 
 * Class to represent chains of equalities and inequalities 
 */
class EqChainForm: public Formula {
ACCEPTBOTH(Formula)
private:
	bool _conj; //!< Indicates whether the chain is a conjunction or disjunction of (in)equalties
	std::vector<CompType> _comps; //!< The consecutive comparisons in the chain

public:
	// Constructors
	EqChainForm(SIGN sign, bool c, Term* t, const FormulaParseInfo& pi)
			: Formula(sign, pi), _conj(c), _comps(0) {
		subterms(std::vector<Term*>(1, t));
	}
	EqChainForm(SIGN s, bool c, const std::vector<Term*>& vt, const std::vector<CompType>& vc, const FormulaParseInfo& pi)
			: Formula(s, pi), _conj(c), _comps(vc) {
		subterms(vt);
	}

	EqChainForm* clone() const;
	EqChainForm* cloneKeepVars() const;
	EqChainForm* clone(const std::map<Variable*, Variable*>&) const;

	// Destructor
	~EqChainForm() {
	}

	// Mutators
	void add(CompType ct, Term* t) {
		_comps.push_back(ct);
		addSubterm(t);
	}
	void conj(bool b) {
		_conj = b;
	}
	void term(unsigned int n, Term* t) {
		subterm(n, t);
	}
	void comp(unsigned int n, CompType ct) {
		_comps[n] = ct;
	}

	// Inspectors
	bool conj() const {
		return _conj;
	}
	bool isConjWithSign() const {
		return (conj() && isPos(sign())) || (not conj() && isNeg(sign()));
	}
	const std::vector<CompType>& comps() const {
		return _comps;
	}

	// Output
	std::ostream& put(std::ostream&) const;
};

/** 
 * Equivalences 
 */
class EquivForm: public Formula {
ACCEPTBOTH(Formula)
public:
	// Constructors
	EquivForm(SIGN sign, Formula* lf, Formula* rf, const FormulaParseInfo& pi)
			: Formula(sign, pi) {
		addSubformula(lf);
		addSubformula(rf);
	}

	EquivForm* clone() const;
	EquivForm* cloneKeepVars() const;
	EquivForm* clone(const std::map<Variable*, Variable*>&) const;

	// Destructor
	~EquivForm() {
	}

	// Mutators
	void left(Formula* f) {
		subformula(0, f);
	}
	void right(Formula* f) {
		subformula(1, f);
	}

	// Inspectors
	Formula* left() const {
		return subformulas().front();
	}
	Formula* right() const {
		return subformulas().back();
	}

	// Output
	std::ostream& put(std::ostream&) const;
};

/** 
 * Conjunctions and disjunctions 
 */
class BoolForm: public Formula {
ACCEPTBOTH(Formula)
private:
	bool _conj; //!< true (false) if the formula is a conjunction (disjunction)

public:
	// Constructors
	BoolForm(SIGN sign, bool c, const std::vector<Formula*>& sb, const FormulaParseInfo& pi)
			: Formula(sign, pi), _conj(c) {
		subformulas(sb);
	}
	BoolForm(SIGN sign, bool c, Formula* left, Formula* right, const FormulaParseInfo& pi)
			: Formula(sign, pi), _conj(c) {
		addSubformula(left);
		addSubformula(right);
	}

	BoolForm* clone() const;
	BoolForm* cloneKeepVars() const;
	BoolForm* clone(const std::map<Variable*, Variable*>&) const;

	// Destructor
	~BoolForm() {
	}

	// Mutators
	void conj(bool b) {
		_conj = b;
	}

	// Inspectors
	bool conj() const {
		return _conj;
	}
	bool trueFormula() const {
		return subformulas().empty() && isConjWithSign();
	}
	bool falseFormula() const {
		return subformulas().empty() && not isConjWithSign();
	}

	bool isConjWithSign() const {
		return (conj() && isPos(sign())) || (not conj() && isNeg(sign()));
	}

	// Debugging
	std::ostream& put(std::ostream&) const;
};

/** 
 *	Universally and existentially quantified formulas 
 */
class QuantForm: public Formula {
ACCEPTBOTH(Formula)
private:
	QUANT _quantifier;

public:
	// Constructors
	QuantForm(SIGN sign, QUANT quant, const std::set<Variable*>& v, Formula* sf, const FormulaParseInfo& pi)
			: Formula(sign, pi), _quantifier(quant) {
		subformulas(std::vector<Formula*>(1, sf));
		quantVars(v);
	}

	QuantForm* clone() const;
	QuantForm* cloneKeepVars() const;
	QuantForm* clone(const std::map<Variable*, Variable*>&) const;

	// Destructor
	~QuantForm() {
	}

	// Mutators
	void add(Variable* v) {
		addQuantVar(v);
	}
	void quant(QUANT b) {
		_quantifier = b;
	}
	void subformula(Formula* f) {
		Formula::subformula(0, f);
	}

	// Inspectors
	Formula* subformula() const {
		return subformulas()[0];
	}
	QUANT quant() const {
		return _quantifier;
	}

	bool isUniv() const {
		return _quantifier == QUANT::UNIV;
	}
	bool isUnivWithSign() const {
		return (_quantifier == QUANT::UNIV && isPos(sign())) || (_quantifier == QUANT::EXIST && isNeg(sign()));
	}

	// Output
	std::ostream& put(std::ostream&) const;
};

/** 
 * Aggregate atoms
 *
 * Formulas of the form 't op a', where 't' is a term, 'op' a comparison operator and 'a' an aggregate term
 */

class AggForm: public Formula {
ACCEPTBOTH(Formula)
private:
	CompType _comp; //!< the comparison operator
	AggTerm* _aggterm; //!< the aggregate term

public:
	// Constructors
	AggForm(SIGN sign, Term* l, CompType c, AggTerm* r, const FormulaParseInfo& pi);

	AggForm* clone() const;
	AggForm* cloneKeepVars() const;
	AggForm* clone(const std::map<Variable*, Variable*>&) const;

	// Destructor
	~AggForm() {
	}

	// Mutators
	void left(Term* t) {
		subterm(0, t);
	}

	// Inspectors
	Term* left() const {
		return subterms()[0];
	}
	AggTerm* right() const {
		return _aggterm;
	}
	CompType comp() const {
		return _comp;
	}

	// Output
	std::ostream& put(std::ostream&) const;
};

/******************
 Definitions
 ******************/

/**
 * Class to represent a single rule of a definition or fixpoint definition
 */
class Rule {
ACCEPTBOTH(Rule)
private:
	PredForm* _head; //!< the head of the rule
	Formula* _body; //!< the body of the rule
	std::set<Variable*> _quantvars; //!< the universally quantified variables of the rule
	ParseInfo _pi; //!< the place where the rule was parsed

public:
	// Constructors
	Rule(const std::set<Variable*>& vv, PredForm* h, Formula* b, const ParseInfo& pi)
			: _head(h), _body(b), _quantvars(vv), _pi(pi) {
	}

	Rule* clone() const; //!< Make a deep copy of the rule

	// Destructor
	~Rule() {
	} //!< Delete the rule, but not its components
	void recursiveDelete(); //!< Delete the rule and its components

	// Mutators
	void head(PredForm* h) {
		_head = h;
	} //!< Replace the head of the rule
	void body(Formula* f) {
		_body = f;
	} //!< Replace the body of the rule
	void addvar(Variable* v) {
		_quantvars.insert(v);
	}

	// Inspectors
	PredForm* head() const {
		return _head;
	}
	Formula* body() const {
		return _body;
	}
	const ParseInfo& pi() const {
		return _pi;
	}
	const std::set<Variable*>& quantVars() const {
		return _quantvars;
	}

	// Output
	std::ostream& put(std::ostream&) const;
	std::string toString(unsigned int spaces = 0) const;
};

std::ostream& operator<<(std::ostream&, const Rule&);

/**
 * \brief Absract class to represent definitions
 */
class AbstractDefinition: public TheoryComponent {
ACCEPTDECLAREBOTH(AbstractDefinition)
public:
	virtual AbstractDefinition* clone() const = 0;
	virtual ~AbstractDefinition() {
	}
};

/**
 * \brief Class to represent inductive definitions
 */
class Definition: public AbstractDefinition {
ACCEPTBOTH(Definition)
private:
	std::vector<Rule*> _rules; //!< The rules in the definition
	std::set<PFSymbol*> _defsyms; //!< Symbols defined by the definition

public:
	Definition() {
	}

	Definition* clone() const;

	~Definition() {
	}
	void recursiveDelete();

	void add(Rule*); //!< add a rule to the definition
	void rule(unsigned int n, Rule* r); //!< Replace the n'th rule of the definition

	const std::vector<Rule*>& rules() const {
		return _rules;
	}
	const std::set<PFSymbol*>& defsymbols() const {
		return _defsyms;
	}

	unsigned int getNumberOfRules() const {
		return _rules.size();
	}

	std::ostream& put(std::ostream&) const;
};

/***************************
 Fixpoint definitions
 ***************************/

class AbstractFixpDef: public TheoryComponent {
ACCEPTDECLAREBOTH(AbstractFixpDef)
public:
	virtual AbstractFixpDef* clone() const = 0;
	virtual ~AbstractFixpDef() {
	}
};

class FixpDef: public AbstractFixpDef {
ACCEPTBOTH(FixpDef)
private:
	bool _lfp; //!< true iff it is a least fixpoint definition
	std::vector<FixpDef*> _defs; //!< the direct subdefinitions  of the definition
	std::vector<Rule*> _rules; //!< the rules of the definition
	std::set<PFSymbol*> _defsyms; //!< the predicates in heads of rules in _rules

public:
	FixpDef(bool lfp = false)
			: _lfp(lfp) {
	}

	FixpDef* clone() const;

	~FixpDef() {
	}
	void recursiveDelete();

	void lfp(bool b) {
		_lfp = b;
	}
	void add(Rule* r); //!< add a rule
	void add(FixpDef* d) {
		_defs.push_back(d);
	} //!< add a direct subdefinition
	void rule(unsigned int n, Rule* r); //!< replace the n'th rule
	void def(unsigned int n, FixpDef* d) {
		_defs[n] = d;
	} //!< replace the n'th subdefinition

	bool lfp() const {
		return _lfp;
	}
	const std::vector<FixpDef*>& defs() const {
		return _defs;
	}
	const std::vector<Rule*>& rules() const {
		return _rules;
	}
	const std::set<PFSymbol*>& defsyms() const {
		return _defsyms;
	}

	std::ostream& put(std::ostream&) const;
};

/***************
 Theories
 ***************/

/**
 * \brief Abstract base class for theories
 */
class AbstractTheory {
ACCEPTDECLAREBOTH(AbstractTheory)
protected:
	std::string _name; //!< the name of the theory
	Vocabulary* _vocabulary; //!< the vocabulary of the theory
	ParseInfo _pi; //!< the place where the theory was parsed

public:
	AbstractTheory(const std::string& name, const ParseInfo& pi)
			: _name(name), _vocabulary(0), _pi(pi) {
	}
	AbstractTheory(const std::string& name, Vocabulary* voc, const ParseInfo& pi)
			: _name(name), _vocabulary(voc), _pi(pi) {
	}

	virtual AbstractTheory* clone() const = 0; //!< Make a deep copy of the theory

	virtual void recursiveDelete() = 0; //!< Delete the theory and its components
	virtual ~AbstractTheory() {
	} //!< Delete the theory, but not its components

	void vocabulary(Vocabulary* v) {
		_vocabulary = v;
	} //!< Change the vocabulary of the theory
	void name(const std::string& n) {
		_name = n;
	} //!< Change the name of the theory
	virtual void add(Formula* f) = 0; //!< Add a formula to the theory
	virtual void add(Definition* d) = 0; //!< Add a definition to the theory
	virtual void add(FixpDef* fd) = 0; //!< Add a fixpoint definition to the theory

	const std::string& name() const {
		return _name;
	}
	Vocabulary* vocabulary() const {
		return _vocabulary;
	}
	const ParseInfo& pi() const {
		return _pi;
	}

	virtual std::ostream& put(std::ostream&) const = 0;
};

std::ostream& operator<<(std::ostream&, const AbstractTheory&);

/**
 * \brief Class to represent first-order theories
 */
class Theory: public AbstractTheory {
ACCEPTBOTH(AbstractTheory)
private:
	std::vector<Formula*> _sentences; //!< the sentences of the theory
	std::vector<Definition*> _definitions; //!< the definitions of the theory
	std::vector<FixpDef*> _fixpdefs; //!< the fixpoint definitions of the theory

public:
	Theory(const std::string& name, const ParseInfo& pi)
			: AbstractTheory(name, pi) {
	}
	Theory(const std::string& name, Vocabulary* voc, const ParseInfo& pi)
			: AbstractTheory(name, voc, pi) {
	}

	Theory* clone() const;

	void recursiveDelete();
	~Theory() {
	}

	void add(Formula* f) {
		_sentences.push_back(f);
	}
	void add(Definition* d) {
		_definitions.push_back(d);
	}
	void add(FixpDef* fd) {
		_fixpdefs.push_back(fd);
	}
	void addTheory(AbstractTheory*);
	void sentence(unsigned int n, Formula* f) {
		_sentences[n] = f;
	}
	void definition(unsigned int n, Definition* d) {
		_definitions[n] = d;
	}
	void fixpdef(unsigned int n, FixpDef* d) {
		_fixpdefs[n] = d;
	}
	void remove(Definition* d);

	std::vector<Formula*>& sentences() {
		return _sentences;
	}
	std::vector<Definition*>& definitions() {
		return _definitions;
	}
	std::vector<FixpDef*>& fixpdefs() {
		return _fixpdefs;
	}

	const std::vector<Formula*>& sentences() const {
		return _sentences;
	}
	const std::vector<Definition*>& definitions() const {
		return _definitions;
	}
	const std::vector<FixpDef*>& fixpdefs() const {
		return _fixpdefs;
	}
	std::set<TheoryComponent*> components() const;

	virtual std::ostream& put(std::ostream&) const;
};

#endif
