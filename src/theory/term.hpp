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

#ifndef TERM_HPP
#define TERM_HPP

#include "parseinfo.hpp"
#include "common.hpp"
#include "vocabulary/VarCompare.hpp"
#include "visitors/VisitorFriends.hpp"

class Sort;
class Function;
class DomainElement;
class DefaultTraversingTheoryVisitor;
class TheoryMutatingVisitor;
class Vocabulary;
class Structure;

enum class TermType {
	VAR,
	FUNC,
	AGG,
	DOM
};

class Term {
ACCEPTDECLAREBOTH(Term)
private:
	TermType _type;
	varset _freevars; //!< the EXACT set of variables occurring unquantified in the term
	std::vector<Term*> _subterms; //!< the subterms of the term
	std::vector<EnumSetExpr*> _subsets; //!< the subsets of the term
	bool _allwaysDeleteRecursively; //!<Standard: false. If true, always deletes recursively (for use in ParseInfo)

	// Terms declared in blocks have additional properties:
	std::string _name;
	Vocabulary* _voc;

private:
	virtual void setFreeVars(); //!< Compute the free variables of the term

	void deleteChildren(bool deleteVars); //Deletes all children of this formula (and depending on the boolean also the vars)

protected:
	TermParseInfo _pi; //!< the place where the term was parsed
	void setFreeVars(varset freevars) {
		_freevars = freevars;
	}

	Term(TermType type, const TermParseInfo& pi)
			: 	_type(type),
				_allwaysDeleteRecursively(false),
				_name(""),
				_voc(NULL),
				_pi(pi){
	}

public:
	virtual Term* clone() const = 0;
	//!< create a copy of the term while keeping the free variables
	virtual Term* cloneKeepVars() const = 0;
	//!< copy the term while keeping all variables
	virtual Term* clone(const std::map<Variable*, Variable*>&) const = 0;
	//!< create a copy of the term and substitute the free variables according to the given map

	virtual ~Term(); //!< Shallow destructor. Does not delete subterms and subsets of the term UNLESS _allwaysDeleteRecursively
	void recursiveDelete(); //!< Delete the term, its subterms, and subsets.
	void recursiveDeleteKeepVars(); //!< Delete the term, its subterms, and subsets. But don't delete variables

	// Mutators
	virtual void sort(Sort*) {
	} //!< Set the sort of the term (only does something for VarTerm and DomainTerm)

	void addSet(EnumSetExpr* s);
	void subterm(size_t n, Term* t) {
		_subterms[n] = t;
		setFreeVars();
	}
	void subset(size_t n, EnumSetExpr* s) {
		_subsets[n] = s;
		setFreeVars();
	}
	void subterms(const std::vector<Term*>& vt) {
		_subterms = vt;
		setFreeVars();
	}
	void allwaysDeleteRecursively(bool aRD) {
		_allwaysDeleteRecursively = aRD;
	}
	void name(std::string name) {
		_name = name;
	}
	void vocabulary(Vocabulary* v) {
		_voc = v;
	}

	const TermParseInfo& pi() const {
		return _pi;
	}
	virtual Sort* sort() const = 0; //!< Returns the sort of the term
	virtual TermType type() const {
		return _type;
	}
	bool contains(const Variable*) const; //!< true iff the term contains the variable
	std::string name() const {
		return _name;
	}
	Vocabulary* vocabulary() const{
		return _voc;
	}
	const varset& freeVars() const {
		return _freevars;
	}
	const std::vector<Term*>& subterms() const {
		return _subterms;
	}
	const std::vector<EnumSetExpr*>& subsets() const {
		return _subsets;
	}

	virtual std::ostream& put(std::ostream&) const = 0;
};

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

	void sort(Sort* s);

	Sort* sort() const;
	Variable* var() const {
		return _var;
	}

	std::ostream& put(std::ostream&) const;
};

/**
 *	Constants are represented by 0-ary functions applied to empty tuples.
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

	void function(Function* f) {
		Assert(f!=NULL);
		_function = f;
	}
	Sort* sort() const;
	Function* function() const {
		return _function;
	}
	const std::vector<Term*>& args() const {
		return subterms();
	}

	std::ostream& put(std::ostream&) const;
};

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

	void sort(Sort* s);

	Sort* sort() const {
		return _sort;
	}
	const DomainElement* value() const {
		return _value;
	}

	std::ostream& put(std::ostream&) const;
};

class AggTerm: public Term {
ACCEPTBOTH(Term)
private:
	AggFunction _function; //!< The aggregate function

public:
	AggTerm(EnumSetExpr* set, AggFunction function, const TermParseInfo& pi);

	AggTerm* clone() const;
	AggTerm* cloneKeepVars() const;
	AggTerm* clone(const std::map<Variable*, Variable*>&) const;

	Sort* sort() const;
	EnumSetExpr* set() const {
		return subsets()[0];
	}
	AggFunction function() const {
		return _function;
	}

	std::ostream& put(std::ostream&) const;
};

namespace TermUtils {
/** Make a vector of fresh variable terms */
std::vector<Term*> makeNewVarTerms(const std::vector<Variable*>&);
std::vector<Term*> makeNewVarTerms(const std::vector<Sort*>&);

/** Attempts to derive a sort which is only a subset of the current sort. If not possible, the original sort is returned. */
Sort* deriveSmallerSort(const Term*, const Structure*);
}

#endif /* TERM_HPP_ */
