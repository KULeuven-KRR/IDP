#ifndef TERM_HPP
#define TERM_HPP

#include "parseinfo.hpp"
#include "common.hpp"

#include "visitors/VisitorFriends.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

class Sort;
class Variable;
class Function;
class DomainElement;
class TheoryVisitor;
class TheoryMutatingVisitor;

/************
 Terms
 ************/

enum TermType {
	TT_VAR, TT_FUNC, TT_AGG, TT_DOM
};

class VarTerm;

/**
 * Abstract class to represent terms
 */
class Term {
ACCEPTDECLAREBOTH(Term)
private:
	std::set<Variable*> _freevars; //!< the set of free variables of the term
	std::vector<Term*> _subterms; //!< the subterms of the term
	std::vector<SetExpr*> _subsets; //!< the subsets of the term

protected:
	TermParseInfo _pi; //!< the place where the term was parsed

private:
	virtual void setFreeVars(); //!< Compute the free variables of the term

public:
	// Constructors
	Term(const TermParseInfo& pi) :
			_pi(pi) {
	}

	virtual Term* clone() const = 0;
	//!< create a copy of the term while keeping the free variables
	virtual Term* cloneKeepVars() const = 0;
	//!< copy the term while keeping all variables
	virtual Term* clone(const std::map<Variable*, Variable*>&) const = 0;
	//!< create a copy of the term and substitute the free variables according to the given map

	// Destructors
	virtual ~Term() {
	} //!< Shallow destructor. Does not delete subterms and subsets of the term.
	void recursiveDelete(); //!< Delete the term, its subterms, and subsets.

	// Mutators
	virtual void sort(Sort*) {
	} //!< Set the sort of the term (only does something for VarTerm and DomainTerm)

	void addSet(SetExpr* s) {
		_subsets.push_back(s);
		setFreeVars();
	}
	void subterm(unsigned int n, Term* t) {
		_subterms[n] = t;
		setFreeVars();
	}
	void subset(unsigned int n, SetExpr* s) {
		_subsets[n] = s;
		setFreeVars();
	}
	void subterms(const std::vector<Term*>& vt) {
		_subterms = vt;
		setFreeVars();
	}

	// Inspectors
	const TermParseInfo& pi() const {
		return _pi;
	}
	virtual Sort* sort() const = 0; //!< Returns the sort of the term
	virtual TermType type() const = 0;
	const std::set<Variable*>& freeVars() const {
		return _freevars;
	}
	const std::vector<Term*>& subterms() const {
		return _subterms;
	}
	const std::vector<SetExpr*>& subsets() const {
		return _subsets;
	}

	bool contains(const Variable*) const; //!< true iff the term contains the variable

	// Output
	virtual std::ostream& put(std::ostream&, bool longnames = false) const = 0;
	std::string toString(bool longnames = false) const;

	friend class VarTerm;
};

std::ostream& operator<<(std::ostream&, const Term&);

/**
 *	\brief Class to represent terms that are variables
 */
class VarTerm: public Term {
ACCEPTBOTH(Term)
private:
	Variable* _var; //!< the variable of the term

	void setFreeVars();

public:
	VarTerm(Variable* v, const TermParseInfo& pi);

	VarTerm* clone() const;
	VarTerm* cloneKeepVars() const;
	VarTerm* clone(const std::map<Variable*, Variable*>&) const;

	~VarTerm() {
	}

	void sort(Sort* s);

	Sort* sort() const;
	TermType type() const {
		return TT_VAR;
	}
	Variable* var() const {
		return _var;
	}

	std::ostream& put(std::ostream&, bool longnames = false) const;
};

/**
 *	\brief Terms formed by applying a function to a tuple of terms.
 *
 *	Constants are represented by 0-ary functions applied to empty tuples.
 *
 */
class FuncTerm: public Term {
ACCEPTBOTH(Term)
private:
	Function* _function; //!< the function

public:
	FuncTerm(Function* function, const std::vector<Term*>& args, const TermParseInfo& pi);

	FuncTerm* clone() const;
	FuncTerm* cloneKeepVars() const;
	FuncTerm* clone(const std::map<Variable*, Variable*>&) const;

	~FuncTerm() {
	}

	void function(Function* f) {
		_function = f;
	}

	Sort* sort() const;
	TermType type() const {
		return TT_FUNC;
	}
	Function* function() const {
		return _function;
	}
	const std::vector<Term*>& args() const {
		return subterms();
	}

	std::ostream& put(std::ostream&, bool longnames = false) const;
};

/**
 *
 * \brief Class to represent terms that are domain elements
 *
 */
class DomainTerm: public Term {
ACCEPTBOTH(Term)
private:
	Sort* _sort; //!< the sort of the domain element
	const DomainElement* _value; //!< the actual domain element

public:
	DomainTerm(Sort* sort, const DomainElement* value, const TermParseInfo& pi);

	DomainTerm* clone() const;
	DomainTerm* cloneKeepVars() const;
	DomainTerm* clone(const std::map<Variable*, Variable*>&) const;

	~DomainTerm() {
	}

	void sort(Sort* s);

	Sort* sort() const {
		return _sort;
	}
	TermType type() const {
		return TT_DOM;
	}
	const DomainElement* value() const {
		return _value;
	}

	std::ostream& put(std::ostream&, bool longnames = false) const;
};

/**
 *
 *	\brief Class to represent aggregate terms
 *
 */
class AggTerm: public Term {
ACCEPTBOTH(Term)
private:
	AggFunction _function; //!< The aggregate function

public:
	AggTerm(SetExpr* set, AggFunction function, const TermParseInfo& pi);

	AggTerm* clone() const;
	AggTerm* cloneKeepVars() const;
	AggTerm* clone(const std::map<Variable*, Variable*>&) const;

	~AggTerm() {
	}

	Sort* sort() const;
	TermType type() const {
		return TT_AGG;
	}
	SetExpr* set() const {
		return subsets()[0];
	}
	AggFunction function() const {
		return _function;
	}

	std::ostream& put(std::ostream&, bool longnames = false) const;
};

namespace TermUtils {
std::vector<Term*> makeNewVarTerms(const std::vector<Variable*>&); //!< Make a vector of fresh variable terms

/**
 * Returns false if the value of the term is defined
 * for all possible instantiations of its free variables
 */
bool isPartial(Term*);
}

/**************
 Queries
 **************/

/**
 * Class to represent a first-order query
 */
class Query {
private:
	std::vector<Variable*> _variables; //!< The free variables of the query. The order of the variables is the
									   //!< order in which they were parsed.
	Formula* _query; //!< The actual query.
	ParseInfo _pi; //!< The place where the query was parsed.
public:
	// Constructors
	Query(const std::vector<Variable*>& vars, Formula* q, const ParseInfo& pi) :
			_variables(vars), _query(q), _pi(pi) {
	}

	// Inspectors
	Formula* query() const {
		return _query;
	}
	const std::vector<Variable*>& variables() const {
		return _variables;
	}
	const ParseInfo& pi() const {
		return _pi;
	}
};

/**********************
 Set expressions
 **********************/

/** 
 *	\brief Abstract base class for first-order set expressions
 */
class SetExpr {
ACCEPTDECLAREBOTH(SetExpr)
protected:
	std::set<Variable*> _freevars; //!< The free variables of the set expression
	std::set<Variable*> _quantvars; //!< The quantified variables of the set expression
	std::vector<Formula*> _subformulas; //!< The direct subformulas of the set expression
	std::vector<Term*> _subterms; //!< The direct subterms of the set expression
	SetParseInfo _pi; //!< the place where the set was parsed

	void setFreeVars(); //!< Compute the free variables of the set

public:
	// Constructors
	SetExpr(const SetParseInfo& pi) :
			_pi(pi) {
	}

	virtual SetExpr* clone() const = 0;
	//!< create a copy of the set while keeping the free variables
	virtual SetExpr* cloneKeepVars() const = 0;
	//!< copy the set while keeping all variables
	virtual SetExpr* clone(const std::map<Variable*, Variable*>&) const = 0;
	//!< create a copy of the set and substitute the free variables according to the given map
	virtual SetExpr* positiveSubset() const = 0;
	//!< generate the subset of positive terms ({x:p(x):t(x)} becomes {x:p(x)&t(x)>0: t(x)})
	virtual SetExpr* negativeSubset() const = 0;
	//!< generate the subset of negated negative terms ({x:p(x):t(x)} becomes {x:p(x)&t(x)<0: -t(x)})
	virtual SetExpr* zeroSubset() const = 0;
	//!< generate the subset of zero terms ({x:p(x):t(x)} becomes {x:p(x)&t(x)=0: 0})

	// Destructors
	virtual ~SetExpr() {
	} //!< Delete the set, but not
	void recursiveDelete(); //!< Delete the set and its subformulas and subterms

	// Mutators
	void subterm(unsigned int n, Term* t) {
		_subterms[n] = t;
		setFreeVars();
	}
	void subformula(unsigned int n, Formula* f) {
		_subformulas[n] = f;
		setFreeVars();
	}
	void addTerm(Term* t) {
		_subterms.push_back(t);
		setFreeVars();
	}
	void addFormula(Formula* f) {
		_subformulas.push_back(f);
		setFreeVars();
	}
	void addQuantVar(Variable* v) {
		_quantvars.insert(v);
		setFreeVars();
	}

	// Inspectors
	virtual Sort* sort() const = 0; //!< Returns the sort of the set
	const std::set<Variable*>& freeVars() const {
		return _freevars;
	}
	const std::set<Variable*>& quantVars() const {
		return _quantvars;
	}
	bool contains(const Variable*) const;
	const std::vector<Formula*>& subformulas() const {
		return _subformulas;
	}
	const std::vector<Term*>& subterms() const {
		return _subterms;
	}
	const SetParseInfo& pi() const {
		return _pi;
	}

	// Output
	virtual std::ostream& put(std::ostream&, bool longnames = false) const = 0;
	std::string toString(bool longnames = false) const;
};

std::ostream& operator<<(std::ostream&, const SetExpr&);

/** 
 *	\brief Set expression of the form [ (phi_1,w_1); ... ; (phi_n,w_n) ] 
 */
class EnumSetExpr: public SetExpr {
ACCEPTBOTH(SetExpr)
public:
	// Constructors
	EnumSetExpr(const SetParseInfo& pi) :
			SetExpr(pi) {
	}
	EnumSetExpr(const std::vector<Formula*>& s, const std::vector<Term*>& w, const SetParseInfo& pi);

	EnumSetExpr* clone() const;
	EnumSetExpr* cloneKeepVars() const;
	EnumSetExpr* clone(const std::map<Variable*, Variable*>&) const;
	EnumSetExpr* positiveSubset() const;
	EnumSetExpr* negativeSubset() const;
	EnumSetExpr* zeroSubset() const;

	~EnumSetExpr() {
	}

	Sort* sort() const;

	std::ostream& put(std::ostream&, bool longnames = false) const;
};

/** 
 * \brief Set expression of the form { x1 ... xn : phi : t }
 **/
class QuantSetExpr: public SetExpr {
ACCEPTBOTH(SetExpr)
public:
	QuantSetExpr(const std::set<Variable*>& v, Formula* s, Term* t, const SetParseInfo& pi);

	QuantSetExpr* clone() const;
	QuantSetExpr* cloneKeepVars() const;
	QuantSetExpr* clone(const std::map<Variable*, Variable*>&) const;
	QuantSetExpr* positiveSubset() const;
	QuantSetExpr* negativeSubset() const;
	QuantSetExpr* zeroSubset() const;

	Sort* sort() const;

	std::ostream& put(std::ostream&, bool longnames = false) const;
};

#endif 
