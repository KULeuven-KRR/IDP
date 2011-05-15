/************************************
	term.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TERM_HPP
#define TERM_HPP

/**
 * \file term.hpp
 *
 *		This file contains the classes to represent first-order terms and first-order sets
 */

#include <set>
#include <vector>
#include <map>
#include "parseinfo.hpp"
#include "commontypes.hpp"

class Sort;
class Variable;
class Function;
class DomainElement;
class TheoryVisitor;
class TheoryMutatingVisitor;

/************
	Terms
************/

class VarTerm;

/**
 * Abstract class to represent terms
 */
class Term {
	private:

		std::set<Variable*>		_freevars;		//!< the set of free variables of the term
		std::vector<Term*>		_subterms;		//!< the subterms of the term
		std::vector<SetExpr*>	_subsets;		//!< the subsets of the term

		virtual	void	setfvars();		//!< Compute the free variables of the term

	protected:
		TermParseInfo			_pi;			//!< the place where the term was parsed

	public:

		// Constructors
		Term(const TermParseInfo& pi) : _pi(pi) { }

		virtual Term* clone()										const = 0;	
			//!< create a copy of the term while keeping the free variables
		virtual Term* clone(const std::map<Variable*,Variable*>&)	const = 0;	
			//!< create a copy of the term and substitute the free variables according to the given map

		// Destructors
		virtual ~Term() { }			//!< Shallow destructor. Does not delete subterms and subsets of the term.
		void recursiveDelete();		//!< Delete the term, its subterms, and subsets.

		// Mutators
		virtual void	sort(Sort*) { }	//!< Set the sort of the term (only does something for VarTerm and DomainTerm)

		void addset(SetExpr* s)						{ _subsets.push_back(s); setfvars();	}
		void subterm(unsigned int n, Term* t)		{ _subterms[n] = t; setfvars();			}
		void subset(unsigned int n, SetExpr* s)		{ _subsets[n] = s; setfvars();			}
		void subterms(const std::vector<Term*>& vt) { _subterms = vt; setfvars();			}

		// Inspectors
				const TermParseInfo&			pi()			const { return _pi;				}
		virtual	Sort*							sort()			const = 0;	//!< Returns the sort of the term
				const std::set<Variable*>&		freevars()		const { return _freevars;		}
				const std::vector<Term*>&		subterms()		const { return _subterms;		}
				const std::vector<SetExpr*>&	subsets()		const { return _subsets;		}

		bool	contains(const Variable*)	const;		//!< true iff the term contains the variable

		// Visitor
		virtual void	accept(TheoryVisitor*)			const = 0;
		virtual Term*	accept(TheoryMutatingVisitor*)  = 0;

		// Output
		virtual std::ostream&	put(std::ostream&)	const = 0;
				std::string		to_string()			const;	

	friend class VarTerm;
};

std::ostream& operator<<(std::ostream&,const Term&);

/**
 *	\brief Class to represent terms that are variables
 */
class VarTerm : public Term {
	private:
		Variable*	_var;	//!< the variable of the term

		void	setfvars();

	public:

		VarTerm(Variable* v, const TermParseInfo& pi);

		VarTerm* clone()										const;
		VarTerm* clone(const std::map<Variable*,Variable*>&)	const;

		~VarTerm() { }

		void	sort(Sort* s);

		Sort*		sort()	const;
		Variable*	var()	const	{ return _var;	}

		void	accept(TheoryVisitor*)	const;
		Term*	accept(TheoryMutatingVisitor*);

		std::ostream&	put(std::ostream&)	const;

};


/**
 *	\brief Terms formed by applying a function to a tuple of terms.
 *
 *	Constants are represented by 0-ary functions applied to empty tuples.
 *
 */
class FuncTerm : public Term {
	private:

		Function*		_function;		//!< the function

	public:

		FuncTerm(Function* function, const std::vector<Term*>& args, const TermParseInfo& pi);

		FuncTerm* clone()										const;
		FuncTerm* clone(const std::map<Variable*,Variable*>&)	const;

		~FuncTerm() { }

		void function(Function* f)	{ _function = f;	}

		Sort*						sort()			const;
		Function*					function()		const	{ return _function;		}
		const std::vector<Term*>&	args()			const	{ return subterms();	}

		void	accept(TheoryVisitor*)	const;
		Term*	accept(TheoryMutatingVisitor*);

		std::ostream&	put(std::ostream&)	const;

};

/**
 *
 * \brief Class to represent terms that are domain elements
 *
 */
class DomainTerm : public Term {
	private:
		Sort*					_sort;		//!< the sort of the domain element
		const DomainElement*	_value;		//!< the actual domain element
		

	public:

		DomainTerm(Sort* sort, const DomainElement* value, const TermParseInfo& pi);

		DomainTerm* clone()										const;
		DomainTerm* clone(const std::map<Variable*,Variable*>&)	const;

		~DomainTerm() { }

		void	sort(Sort* s)	{ _sort = s;	}

		Sort*					sort()		const { return _sort;	}
		const DomainElement*	value()		const { return _value;	}

		void	accept(TheoryVisitor*)	const;
		Term*	accept(TheoryMutatingVisitor*);

		std::ostream&	put(std::ostream&)	const;	

};

/**
 *
 *	\brief Class to represent aggregate terms
 *
 */
class AggTerm : public Term {

	private:
		AggFunction		_function;	//!< The aggregate function

	public:

		AggTerm(SetExpr* set, AggFunction function, const TermParseInfo& pi);

		AggTerm* clone()										const;
		AggTerm* clone(const std::map<Variable*,Variable*>&)	const;

		~AggTerm() { }

		Sort*		sort()		const;
		SetExpr*	set()		const	{ return subsets()[0];	}
		AggFunction	function()	const	{ return _function;		}

		void	accept(TheoryVisitor*)	const;
		Term*	accept(TheoryMutatingVisitor*);

		std::ostream&	put(std::ostream&)	const;

};

namespace TermUtils {
	std::vector<Term*> 	makeNewVarTerms(const std::vector<Variable*>&);	//!< Make a vector of fresh variable terms
}


/**********************
	Set expressions
**********************/

/** 
 *
 *	\brief Abstract base class for first-order set expressions 
 *
 */
class SetExpr {
	protected:
		
		std::set<Variable*>		_freevars;		//!< The free variables of the set expression
		std::set<Variable*>		_quantvars;		//!< The quantified variables of the set expression
		std::vector<Formula*>	_subformulas;	//!< The direct subformulas of the set expression
		std::vector<Term*>		_subterms;		//!< The direct subterms of the set expression
		SetParseInfo			_pi;			//!< the place where the set was parsed

		void	setfvars();	//!< Compute the free variables of the set

	public:

		// Constructors
		SetExpr(const SetParseInfo& pi) : _pi(pi) { }

		virtual SetExpr* clone()										const = 0;
			//!< create a copy of the set while keeping the free variables
		virtual SetExpr* clone(const std::map<Variable*,Variable*>&)	const = 0;
			//!< create a copy of the set and substitute the free variables according to the given map

		// Destructors
		virtual ~SetExpr() { }		//!< Delete the set, but not 
		void	recursiveDelete();	//!< Delete the set and its subformulas and subterms

		// Mutators
		void subterm(unsigned int n, Term* t)		{ _subterms[n] = t; setfvars();				}
		void subformula(unsigned int n, Formula* f)	{ _subformulas[n] = f; setfvars();			}
		void addterm(Term* t)						{ _subterms.push_back(t); setfvars();		}
		void addformula(Formula* f)					{ _subformulas.push_back(f); setfvars();	}
		
		// Inspectors
		virtual Sort*							sort()						const = 0;	//!< Returns the sort of the set
				const std::set<Variable*>&		freevars()					const { return _freevars;	}
				const std::set<Variable*>&		quantvars()					const { return _quantvars;	}
				bool							contains(const Variable*)	const;
				const std::vector<Formula*>&	subformulas()				const { return _subformulas;	}
				const std::vector<Term*>&		subterms()					const { return _subterms;		}
				const SetParseInfo&				pi()						const { return _pi;				}

		// Visitor
		virtual void		accept(TheoryVisitor*)			const = 0;
		virtual SetExpr*	accept(TheoryMutatingVisitor*)	= 0;

		// Output
		virtual std::ostream&	put(std::ostream&)	const = 0;
				std::string		to_string()			const;

};

std::ostream& operator<<(std::ostream&,const SetExpr&);

/** 
 *	\brief Set expression of the form [ (phi_1,w_1); ... ; (phi_n,w_n) ] 
 */
class EnumSetExpr : public SetExpr {

	public:
		// Constructors
		EnumSetExpr(const SetParseInfo& pi) : SetExpr(pi) { }
		EnumSetExpr(const std::vector<Formula*>& s, const std::vector<Term*>& w, const SetParseInfo& pi);
			

		EnumSetExpr* clone()										const;
		EnumSetExpr* clone(const std::map<Variable*,Variable*>&)	const;

		~EnumSetExpr() { }

		Sort*	sort()	const;

		void		accept(TheoryVisitor*)	const;
		SetExpr*	accept(TheoryMutatingVisitor*);

		std::ostream& put(std::ostream&) const;
};

/** 
 * \brief Set expression of the form { x1 ... xn : t : phi }
 **/
class QuantSetExpr : public SetExpr {

	public:
		QuantSetExpr(const std::set<Variable*>& v, Term* t, Formula* s, const SetParseInfo& pi);

		QuantSetExpr* clone()										const;
		QuantSetExpr* clone(const std::map<Variable*,Variable*>&)	const;

		~QuantSetExpr() { }

		Sort*	sort()	const;

		void		accept(TheoryVisitor*)	const;
		SetExpr*	accept(TheoryMutatingVisitor*);

		std::ostream&	put(std::ostream&)	const;	
};

class AbstractStructure;

namespace SetUtils {
	bool approxTwoValued(SetExpr*,AbstractStructure*);	
		//!< Returns false if the set expression is not two-valued in the given structure. May return true
		//!< if the set expression is two-valued in the structure.
											
}

#endif 
