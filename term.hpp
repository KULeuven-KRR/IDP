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

/************
	Terms
************/

/**
 * Abstract class to represent terms
 */
class Term {

	protected:

		std::set<Variable*>		_freevars;		//!< the set of free variables of the term
		std::vector<Term*>		_subterms;		//!< the subterms of the term
		std::vector<SetExpr*>	_subsets;		//!< the subsets of the term
		TermParseInfo			_pi;			//!< the place where the term was parsed

		virtual	void	setfvars();		//!< Compute the free variables of the term

	public:

		Term(const TermParseInfo& pi) : _pi(pi) { }

		virtual Term* clone()										const = 0;	
			//!< create a copy of the term while keeping the free variables
		virtual Term* clone(const std::map<Variable*,Variable*>&)	const = 0;	
			//!< create a copy of the term and substitute the free variables according to the given map

		virtual ~Term() { }	
			//!< Shallow destructor. Does not delete subterms and subsets of the term.
		void recursiveDelete();		
			//!< Delete the term, its subterms, and subsets.

		virtual void	sort(Sort*) { }	//!< Set the sort of the term (only does something for VarTerm and DomainTerm)

				const ParseInfo&	pi()						const { return _pi;				}
		virtual	Sort*							sort()			const = 0;	//!< Returns the sort of the term
				const std::set<Variable*>&		freevars()		const { return _freevars;		}
				const std::vector<Term*>&		subterms()		const { return _subterms;		}
				const std::vector<SetExpr*>&	subsets()		const { return _subsets;		}

		bool	contains(const Variable*)	const;		//!< true iff the term contains the variable

		virtual std::ostream&	put(std::ostream&)	const = 0;
				std::string		to_string()			const;	

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
		Function*					function()		const	{ return _function;			}
		const std::vector<Term*>&	args()			const	{ return _subterms;			}

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
		SetExpr*	set()		const	{ return *(_subsets.begin());	}
		AggFunction	function()	const	{ return _function;				}

		std::ostream&	put(std::ostream&)	const;

};


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

		void	setfvars();

	public:

		SetExpr(const SetParseInfo& pi) : _pi(pi) { }

		virtual SetExpr* clone()										const = 0;
		virtual SetExpr* clone(const std::map<Variable*,Variable*>&)	const = 0;

		virtual ~SetExpr() { }		// Delete the set, but not 
		void	recursiveDelete();	// Delete the set and its subformulas and subterms

		std::ostream	put(std::ostream&)	const;
		std::string		to_string()			const;

};

std::ostream& operator<<(std::ostream&,const SetExpr&);

/** 
 *	\brief Set expression of the form [ (phi_1,w_1); ... ; (phi_n,w_n) ] 
 */
class EnumSetExpr : public SetExpr {

	public:

		// Constructors
		EnumSetExpr(const vector<Formula*>& s, const vector<Term*>& w, const ParseInfo& pi) : 
			SetExpr(pi), _subf(s), _weights(w) { setfvars(); }

		EnumSetExpr* clone()								const;
		EnumSetExpr* clone(const map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void	subf(unsigned int n, Formula* f)	{ _subf[n] = f;		}
		void	weight(unsigned int n, Term* t)		{ _weights[n] = t;	}

		// Inspectors
		unsigned int	nrSubforms()			const	{ return _subf.size();		}	
		unsigned int	nrSubterms()			const	{ return _weights.size();	}
		unsigned int	nrQvars()				const	{ return 0;					}
		Formula*		subform(unsigned int n)	const	{ return _subf[n];			}
		Term*			subterm(unsigned int n)	const	{ return _weights[n];		}
		Variable*		qvar(unsigned int)		const	{ assert(false); return 0;	}
		Sort*			firstargsort()			const;

		// Visitor
		void		accept(Visitor* v) const;
		SetExpr*	accept(MutatingVisitor* v);

		// Debugging
		string	to_string()	const;	

};

/** 
 * \brief Set expression of the form { x1 ... xn : t : phi }
 **/
class QuantSetExpr : public SetExpr {

	public:

		// Constructors
		QuantSetExpr(const vector<Variable*>& v, Formula* s, const ParseInfo& pi) : 
			SetExpr(pi), _subf(s), _vars(v) { setfvars(); }

		QuantSetExpr* clone()									const;
		QuantSetExpr* clone(const map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void	subf(Formula* f)	{ _subf = f;	}

		// Inspectors
		unsigned int	nrSubforms()			const	{ return 1;					}	
		unsigned int	nrSubterms()			const	{ return 0;					}
		unsigned int	nrQvars()				const	{ return _vars.size();		}
		Formula*		subform(unsigned int)	const	{ return _subf;				}
		Term*			subterm(unsigned int)	const	{ assert(false); return 0;	}
		Variable*		qvar(unsigned int n)	const	{ return _vars[n];			}
		Formula*		subf()					const	{ return _subf;				}
		Sort*			firstargsort()			const;
		const vector<Variable*>&	qvars()		const	{ return _vars;				}

		// Visitor
		void		accept(Visitor* v) const;
		SetExpr*	accept(MutatingVisitor* v);

		// Debugging
		string	to_string()	const;	

};

class AbstractStructure;
namespace SetUtils {
	bool isTwoValued(SetExpr*,AbstractStructure*);
}

/** Aggregate types **/
enum AggType { AGGCARD, AGGSUM, AGGPROD, AGGMIN, AGGMAX };

namespace AggUtils {
	double compute(AggType,const vector<double>&);	// apply the aggregate on the given set of doubles 
}

/***********************
	Evaluating terms
***********************/

class TermEvaluator : public Visitor {

	private:
		FiniteSortTable*			_returnvalue;
		AbstractStructure*			_structure;
		map<Variable*,TypedElement>	_varmapping;

	public:
		TermEvaluator(AbstractStructure* s,const map<Variable*,TypedElement> m);
		TermEvaluator(Term* t,AbstractStructure* s,const map<Variable*,TypedElement> m);

		FiniteSortTable* returnvalue()	{ return _returnvalue;	}

		void visit(const VarTerm* vt);
		void visit(const FuncTerm* ft);
		void visit(const DomainTerm* dt);
		void visit(const AggTerm* at);
		
};

namespace TermUtils {
	/**
	 * DESCRIPTION
	 *	evaluate the given term in the given structure under the given variable mapping
	 *		in case of a three-valued function, this may result in multiple values of the term
	 *		in case of a partial function, the term may have no value
	 * NOTE 
	 *	This method works for general terms and structures. Therefore, it is rather slow.
	 *	Faster methods exist if the structure is two-valued and the term contains no partial functions
	 * PRECONDITION
	 *	- all bounded variables in the term range over a finite domain in the given structure
	 *	- all free variables of the term are interpreted by the given map
	 */
	FiniteSortTable*	evaluate(Term*,AbstractStructure*,const map<Variable*,TypedElement>&);	

	/**
	 * DESCRIPTION
	 * 	Make a vector of fresh variable terms.
	 */ 
	vector<Term*> 		makeNewVarTerms(const vector<Variable*>&);
}



#endif 
