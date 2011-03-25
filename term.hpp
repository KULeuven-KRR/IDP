/************************************
	term.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TERM_HPP
#define TERM_HPP

#include <vector>
#include <string>
#include <map>
#include <cassert>

#include "element.hpp"
#include "parseinfo.hpp"
#include "visitor.hpp"

class Variable;
class FiniteSortTable;
class AbstractStructure;

/*******************************
	Abstract base class term
*******************************/

class Term {
	protected:
		std::vector<Variable*>	_fvars;	// Free variables of the term
		ParseInfo				_pi;	// the place where the term was parsed

	public:
		// Constructors
		Term(const ParseInfo& pi) : _pi(pi) { }

		// Virtual constructors
		virtual Term* clone()										const = 0;	// create a copy of the term while keeping the free variables
		virtual Term* clone(const std::map<Variable*,Variable*>&)	const = 0;	// create a copy of the term and substitute the
																			// free variables according to the given map

		// Destructor
		virtual ~Term() { }
		virtual void recursiveDelete() = 0;		// Delete a term and its subterms

		// Mutators
		virtual	void	setfvars();		// Compute the free variables of the term
		virtual void	sort(Sort*)	{ }	// Set the sort of the term (only does something for VarTerm and DomainTerm)

		// Inspectors
		virtual	Sort*				sort()						const = 0;	// The sort of the term
		virtual	unsigned int		nrSubforms()				const = 0;	// number of direct subformulas
		virtual	unsigned int		nrSubterms()				const = 0;	// number of direct subterms
		virtual	unsigned int		nrSubsets()					const = 0;	// number of direct subsets
				unsigned int		nrFvars()					const { return _fvars.size();	}
		virtual	unsigned int		nrQvars()					const = 0;	// the number of variables quantified by the term
		virtual	Formula*			subform(unsigned int n)		const = 0;	// the n'th direct subformula
		virtual	Term*				subterm(unsigned int n)		const = 0;	// the n'th direct subterm
		virtual	SetExpr*			subset(unsigned int n)		const = 0;	// the n'th direct subset
		virtual	Variable*			fvar(unsigned int n)		const { return _fvars[n];		}
		virtual Variable*			qvar(unsigned int n)		const = 0;	// the n'th quantified variable of the term
				const ParseInfo&	pi()						const { return _pi;				}
		virtual bool				contains(const Variable*)	const;		// true iff the term contains the variable

		// Visitor
		virtual void	accept(Visitor*) const		= 0;
		virtual Term*	accept(MutatingVisitor*)	= 0;

		// Debugging
		virtual	std::string	to_string()	const = 0;	
};


/*******************************
	Terms that are variables
*******************************/

class VarTerm : public Term {
	private:
		Variable* _var;	// the variable of the term

	public:
		// Constructors
		VarTerm(Variable* v, const ParseInfo& pi);

		VarTerm* clone()										const;
		VarTerm* clone(const std::map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete() { delete(this);	}

		// Mutators
		void	setfvars();
		void	sort(Sort* s);

		// Inspectors
		Sort*			sort()						const;
		Variable*		var()						const	{ return _var;				}
		unsigned int	nrSubforms()				const	{ return 0;					}
		unsigned int	nrSubterms()				const	{ return 0;					}
		unsigned int	nrSubsets()					const	{ return 0;					}
		unsigned int	nrQvars()					const	{ return 0;					}
		Formula*		subform(unsigned int)		const	{ assert(false); return 0;	}
		Term*			subterm(unsigned int)		const	{ assert(false); return 0;	}
		SetExpr*		subset(unsigned int)		const	{ assert(false); return 0;	}
		Variable*		qvar(unsigned int)			const	{ assert(false); return 0;	}
		bool			contains(const Variable* v)	const	{ return _var == v;			}	

		// Visitor
		void	accept(Visitor* v) const;
		Term*	accept(MutatingVisitor* v);

		// Output
		std::string to_string()	const { return _var->to_string();	}
};


/***********************************************************************
	Terms formed by applying a function to a tuple of terms
	Constants (0-ary function applied to empty tuple)
***********************************************************************/

class FuncTerm : public Term {
	private:
		Function*			_func;		// the function
		std::vector<Term*>	_args;		// the arguments of the function

	public:
		// Constructors
		FuncTerm(Function* f, const std::vector<Term*>& a, const ParseInfo& pi);

		FuncTerm* clone()										const;
		FuncTerm* clone(const std::map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void func(Function* f)				{ _func = f;	}
		void arg(unsigned int n, Term* t)	{ _args[n] = t;	}

		// Inspectors
		Sort*			sort()					const	{ return _func->outsort();	}
		Function*		func()					const	{ return _func;				}
		const std::vector<Term*>&	args()		const	{ return _args;				}
		Term*			arg(unsigned int n)		const	{ return _args[n];			}
		Formula*		subform(unsigned int)	const	{ assert(false); return 0;	}
		Term*			subterm(unsigned int n)	const	{ return _args[n];			}
		SetExpr*		subset(unsigned int)	const	{ assert(false); return 0;	}
		Variable*		qvar(unsigned int)		const	{ assert(false); return 0;	}
		unsigned int	nrSubforms()			const	{ return 0;					}
		unsigned int	nrSubterms()			const	{ return _args.size();		}
		unsigned int	nrSubsets()				const	{ return 0;					}
		unsigned int	nrQvars()				const	{ return 0;					}

		// Visitor
		void	accept(Visitor* v) const;
		Term*	accept(MutatingVisitor* v);

		// Debugging
		std::string to_string() const;
};

/**********************
	Domain constants
**********************/

class DomainTerm : public Term {
	private:
		Sort*		_sort;		// The sort of the domain element
		ElementType	_type;		// Whether the term is an int, double, string, or compound
		Element		_value;		// The value of the domain element

	public:
		// Constructors
		DomainTerm(Sort* s, ElementType t, Element v, const ParseInfo& pi) : 
			Term(pi), _sort(s), _type(t), _value(v) { assert(s); setfvars(); }

		DomainTerm* clone()										const;
		DomainTerm* clone(const std::map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void	sort(Sort* s)	{ _sort = s;	}

		// Inspectors
		Sort*			sort()					const { return _sort;				}
		unsigned int	nrSubforms()			const { return 0;					}
		unsigned int	nrSubterms()			const { return 0;					}
		unsigned int	nrSubsets()				const { return 0;					}
		unsigned int	nrQvars()				const { return 0;					}
		Formula*		subform(unsigned int)	const { assert(false); return 0;	}
		Term*			subterm(unsigned int)	const { assert(false); return 0;	}
		SetExpr*		subset(unsigned int)	const { assert(false); return 0;	}
		Variable*		qvar(unsigned int)		const { assert(false); return 0;	}
		Element			value()					const { return _value;				}
		ElementType		type()					const { return _type;				}

		// Visitor
		void	accept(Visitor* v) const;
		Term*	accept(MutatingVisitor* v);

		// Debugging
		std::string	to_string()	const;	
};


/*****************
	Aggregates
*****************/

/** Abstract base class for set expressions **/
class SetExpr {
	protected:
		std::vector<Variable*>	_fvars;	// The free variables of the set expression
		ParseInfo				_pi;	// the place where the set was parsed

	public:
		// Constructors
		SetExpr(const ParseInfo& pi) : _pi(pi) { }

		virtual SetExpr* clone()										const = 0;
		virtual SetExpr* clone(const std::map<Variable*,Variable*>&)	const = 0;

		// Destructor
		virtual void recursiveDelete() = 0;	// Delete the set and its subformulas and subterms
		virtual ~SetExpr() { }

		// Mutators
		void	setfvars();

		// Inspectors
				unsigned int	nrFvars()				const { return _fvars.size();	}
		virtual	unsigned int	nrSubforms()			const = 0;	// Number of direct subformulas of the set
		virtual	unsigned int	nrSubterms()			const = 0;	// Number of direct subterms of the set
		virtual	unsigned int	nrQvars()				const = 0;	// Number of variables quantified by the set
				Variable*		fvar(unsigned int n)	const { return _fvars[n];		}	
		virtual	Formula*		subform(unsigned int n)	const = 0;	// The n'th direct subformula
		virtual	Term*			subterm(unsigned int n)	const = 0;	// The n'th direct subterm
		virtual	Variable*		qvar(unsigned int n)	const = 0;	// The n'th quantified variable
		virtual	Sort*			firstargsort()			const = 0;	// Sort of the first element in any tuple in the set

		// Visitor
		virtual void		accept(Visitor* v) const = 0;
		virtual SetExpr*	accept(MutatingVisitor* v) = 0;

		// Debugging
		virtual std::string	to_string()	const = 0;
};

/** Set expression of the form [ (phi1,w1); ... ; (phin,wn) ] **/
class EnumSetExpr : public SetExpr {
	private:
		std::vector<Formula*>	_subf;		// the subformulas
		std::vector<Term*>		_weights;	// the associated weights

	public:
		// Constructors
		EnumSetExpr(const std::vector<Formula*>& s, const std::vector<Term*>& w, const ParseInfo& pi) : 
			SetExpr(pi), _subf(s), _weights(w) { setfvars(); }

		EnumSetExpr* clone()								const;
		EnumSetExpr* clone(const std::map<Variable*,Variable*>&)	const;

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
		std::string	to_string()	const;	
};

/** Set expression of the form { x1 ... xn : phi } **/
class QuantSetExpr : public SetExpr {
	private:
		Formula*				_subf;	// the direct subformula
		std::vector<Variable*>	_vars;	// the quantified variables

	public:
		// Constructors
		QuantSetExpr(const std::vector<Variable*>& v, Formula* s, const ParseInfo& pi) : 
			SetExpr(pi), _subf(s), _vars(v) { setfvars(); }

		QuantSetExpr* clone()									const;
		QuantSetExpr* clone(const std::map<Variable*,Variable*>&)	const;

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
		const std::vector<Variable*>&	qvars()	const	{ return _vars;				}

		// Visitor
		void		accept(Visitor* v) const;
		SetExpr*	accept(MutatingVisitor* v);

		// Debugging
		std::string	to_string()	const;	
};

namespace SetUtils {
	bool isTwoValued(SetExpr*,AbstractStructure*);
}

/** Aggregate types **/
namespace AggUtils {
	double compute(AggType,const std::vector<double>&);	// apply the aggregate on the given set of doubles 
}

/** Aggregate term **/
class AggTerm : public Term {
	private:
		SetExpr*	_set;
		AggType		_type;

	public:
		// Constructors
		AggTerm(SetExpr* s, AggType t, const ParseInfo& pi) : 
			Term(pi), _set(s), _type(t) { setfvars(); }

		AggTerm* clone()										const;
		AggTerm* clone(const std::map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete() { _set->recursiveDelete(); delete(this);	}

		// Mutators
		void	set(SetExpr* s) { _set = s;	}

		// Inspectors
		Sort*			sort()					const;
		unsigned int	nrSubforms()			const	{ return _set->nrSubforms();	}
		unsigned int	nrSubterms()			const	{ return _set->nrSubterms();	}
		unsigned int	nrQvars()				const	{ return _set->nrQvars();		}
		unsigned int	nrSubsets()				const	{ return 1;						}
		Formula*		subform(unsigned int n)	const	{ return _set->subform(n);		}
		Term*			subterm(unsigned int n)	const	{ return _set->subterm(n);		}
		SetExpr*		subset(unsigned int)	const	{ return _set;					}
		Variable*		qvar(unsigned int n)	const	{ return _set->qvar(n);			}
		SetExpr*		set()					const	{ return _set;					}
		AggType			type()					const	{ return _type;					}

		// Visitor
		void	accept(Visitor* v) const;
		Term*	accept(MutatingVisitor* v);

		// Debugging
		std::string	to_string()	const;	
};

/***********************
	Evaluating terms
***********************/

class TermEvaluator : public Visitor {
	private:
		FiniteSortTable*					_returnvalue;
		AbstractStructure*					_structure;
		std::map<Variable*,TypedElement>	_varmapping;

	public:
		TermEvaluator(AbstractStructure* s,const std::map<Variable*,TypedElement> m);
		TermEvaluator(Term* t,AbstractStructure* s,const std::map<Variable*,TypedElement> m);

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
	FiniteSortTable*	evaluate(Term*,AbstractStructure*,const std::map<Variable*,TypedElement>&);	

	/**
	 * DESCRIPTION
	 * 	Make a vector of fresh variable terms.
	 */ 
	std::vector<Term*> 		makeNewVarTerms(const std::vector<Variable*>&);
}

#endif 
