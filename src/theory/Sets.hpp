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

#include "parseinfo.hpp"
#include "common.hpp"
#include "vocabulary/VarCompare.hpp"
#include "visitors/VisitorFriends.hpp"

class QuantSetExpr;
struct tablesize;
class Sort;
class Structure;

/**
 * Base class for first-order set expressions
 */
class SetExpr {
	ACCEPTDECLAREBOTH(SetExpr)
private:
	varset _quantvars; //!< The quantified variables of the set expression
	varset _freevars; //!< the EXACT set of variables occurring unquantified in the term

	SetParseInfo _pi; //!< the place where the set was parsed
	bool _allwaysDeleteRecursively; //!<Standard: false. If true, always deletes recursively (for use in ParseInfo)

	std::vector<QuantSetExpr*> _subsets;
	Formula* _subformula;
	Term* _subterm;

protected:
	void setFreeVars(); //!< Compute the free variables of the set

	void addSubSet(QuantSetExpr* set);
	void setTerm(Term* term) {
		_subterm = term;
		setFreeVars();
	}
	void setSubFormula(Formula* formula) {
		Assert(formula!=NULL);
		_subformula = formula;
		setFreeVars();
	}
	Formula* getSubFormula() const {
		return _subformula;
	}
	Term* getSubTerm() const {
		return _subterm;
	}
	void setSubSet(uint index, QuantSetExpr* set) {
		_subsets[index] = set;
		setFreeVars();
	}
	void setSubSets(std::vector<QuantSetExpr*> sets);
	void setSubTerm(Term* term) {
		_subterm = term;
		setFreeVars();
	}
	const std::vector<QuantSetExpr*>& getSubSets() const {
		return _subsets;
	}
	const varset& getSubQuantVars() const {
		return _quantvars;
	}
	void addSubQuantVar(Variable* var) {
		_quantvars.insert(var);
		setFreeVars();
	}
	void setQuantVars(const varset& vars) {
		_quantvars = vars;
		setFreeVars();
	}

public:
	SetExpr(const SetParseInfo& pi)
			: 	_pi(pi),
				_allwaysDeleteRecursively(false),
				_subformula(NULL),
				_subterm(NULL) {
	}

	virtual SetExpr* clone() const = 0;
	//!< create a copy of the set while keeping the free variables
	virtual SetExpr* cloneKeepVars() const = 0;
	//!< copy the set while keeping all variables
	virtual SetExpr* clone(const std::map<Variable*, Variable*>&) const = 0;
	//!< create a copy of the set and substitute the free variables according to the given map

	virtual ~SetExpr(); //!< Delete the set, but not its subformulas and subterms UNLESS _allwaysDeleteRecursively is true
	void recursiveDelete(); //!< Delete the set and its subformulas and subterms
	void recursiveDeleteKeepVars(); //!< Delete the set and its subformulas and subterms but don't delete variables

	void allwaysDeleteRecursively(bool aRD) {
		_allwaysDeleteRecursively = aRD;
	}

	const varset& freeVars() const {
		return _freevars;
	}

	Sort* sort() const;

	virtual tablesize maxSize(const Structure* str = NULL) const = 0;

	bool contains(const Variable*) const;

	const SetParseInfo& pi() const {
		return _pi;
	}

	virtual std::ostream& put(std::ostream&) const = 0;
private:
	void deleteChildren(bool deleteVars); //Deletes all children (and depending on the boolean also the vars)

};

std::ostream& operator<<(std::ostream&, const SetExpr&);

/**
 * Set representing the union of sets
 */
class EnumSetExpr: public SetExpr {
ACCEPTBOTH(EnumSetExpr)
public:
	EnumSetExpr(const SetParseInfo& pi)
			: SetExpr(pi) {
	}
	EnumSetExpr(const std::vector<QuantSetExpr*>& s, const SetParseInfo& pi);

	void addSet(QuantSetExpr* set) {
		addSubSet(set);
	}

	EnumSetExpr* clone() const;
	EnumSetExpr* cloneKeepVars() const;
	EnumSetExpr* clone(const std::map<Variable*, Variable*>&) const;

	const std::vector<QuantSetExpr*>& getSets() const {
		return getSubSets();
	}
	void setSet(uint index, QuantSetExpr* set) {
		setSubSet(index, set);
	}

	tablesize maxSize(const Structure* str = NULL) const;

	std::ostream& put(std::ostream&) const;
};

/**
 * Set expression of the form { x1 ... xn : phi : t }
 **/
class QuantSetExpr: public SetExpr {
ACCEPTBOTH(QuantSetExpr)
public:
	QuantSetExpr(const varset& v, Formula* s, Term* t, const SetParseInfo& pi);

	Formula* getCondition() const {
		return getSubFormula();
	}
	void setCondition(Formula* condition) {
		setSubFormula(condition);
	}
	Term* getTerm() const {
		return getSubTerm();
	}
	void setTerm(Term* term) {
		setSubTerm(term);
	}

	QuantSetExpr* clone() const;
	QuantSetExpr* cloneKeepVars() const;
	QuantSetExpr* clone(const std::map<Variable*, Variable*>&) const;

	const varset& quantVars() const {
		return getSubQuantVars();
	}

	void quantVars(const varset& vs) {
		return setQuantVars(vs);
	}
	void addQuantVar(Variable* v) {
		addSubQuantVar(v);
	}

	tablesize maxSize(const Structure* str = NULL) const;

	std::ostream& put(std::ostream&) const;
};
