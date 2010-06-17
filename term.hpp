/************************************
	term.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TERM_H
#define TERM_H

#include "vocabulary.hpp"
#include "visitor.hpp"

/*******************************
	Abstract base class term
*******************************/

class Term {

	protected:

		vector<Variable*>	_fvars;	// Free variables of the term
		ParseInfo*			_pi;	// the place where the term was parsed (0 for non user-defined terms)

	public:

		// Constructors
		Term(ParseInfo* pi) : _pi(pi) { }

		// Virtual constructors
		virtual Term* clone()									const = 0;	// create a copy of the term while keeping the free variables
		virtual Term* clone(const map<Variable*,Variable*>&)	const = 0;	// create a copy of the term and substitute the
																			// free variables according to the given map

		// Destructor
		virtual ~Term() { if(_pi) delete(_pi); }
		virtual void recursiveDelete() = 0;

		// Mutators
		virtual	void	setfvars();		// Compute the free variables of the term
		virtual void	sort(Sort*)	{ }	// Set the sort of the term (only does something for VarTerm)

		// Inspectors
		virtual	Sort*			sort()					const = 0;	// The sort of the term
		virtual	unsigned int	nrSubforms()			const = 0;	// number of direct subformulas
		virtual	unsigned int	nrSubterms()			const = 0;	// number of direct subterms
		virtual	unsigned int	nrSubsets()				const = 0;	// number of direct subsets
				unsigned int	nrFvars()				const { return _fvars.size();	}
		virtual	unsigned int	nrQvars()				const = 0;	// the number of variables quantified by the term
		virtual	Formula*		subform(unsigned int n)	const = 0;	// the n'th direct subformula
		virtual	Term*			subterm(unsigned int n)	const = 0;	// the n'th direct subterm
		virtual	SetExpr*		subset(unsigned int n)	const = 0;	// the n'th direct subset
		virtual	Variable*		fvar(unsigned int n)	const { return _fvars[n];		}
		virtual Variable*		qvar(unsigned int n)	const = 0;	// the n'th quantified variable of the term
				ParseInfo*		pi()					const { return _pi;				}
		virtual bool			contains(Variable*)		const;		// true iff the term contains the variable

		// Visitor
		virtual void	accept(Visitor*)			= 0;
		virtual Term*	accept(MutatingVisitor*)	= 0;

		// Debugging
		virtual	string	to_string()	const = 0;	

};


/****************
	Variables
****************/

class VarTerm : public Term {
	
	private:
		Variable* _var;	// the variable of the term

	public:

		// Constructors
		VarTerm(Variable* v, ParseInfo* pi);

		VarTerm* clone()								const;
		VarTerm* clone(const map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete() { delete(this);	}

		// Mutators
		void	setfvars();
		void	sort(Sort* s)	{ _var->sort(s);	}

		// Inspectors
		Sort*			sort()					const	{ return _var->sort();		}
		Variable*		var()					const	{ return _var;				}
		unsigned int	nrSubforms()			const	{ return 0;					}
		unsigned int	nrSubterms()			const	{ return 0;					}
		unsigned int	nrSubsets()				const	{ return 0;					}
		unsigned int	nrQvars()				const	{ return 0;					}
		Formula*		subform(unsigned int n)	const	{ assert(false); return 0;	}
		Term*			subterm(unsigned int n)	const	{ assert(false); return 0;	}
		SetExpr*		subset(unsigned int n)	const	{ assert(false); return 0;	}
		Variable*		qvar(unsigned int n)	const	{ assert(false); return 0;	}
		bool			contains(Variable* v)	const	{ return _var == v;			}	

		// Visitor
		void	accept(Visitor* v);
		Term*	accept(MutatingVisitor* v);

		// Output
		string to_string()	const { return _var->to_string();	}

};


/******************************
	Functions and constants
******************************/

class FuncTerm : public Term {
	
	private:

		Function*		_func;		// the function
		vector<Term*>	_args;		// the arguments of the function

	public:

		// Constructors
		FuncTerm(Function* f, const vector<Term*>& a, ParseInfo* pi);

		FuncTerm* clone()									const;
		FuncTerm* clone(const map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void func(Function* f)				{ _func = f;	}
		void arg(unsigned int n, Term* t)	{ _args[n] = t;	}

		// Inspectors
		Sort*			sort()					const	{ return _func->outsort();	}
		Function*		func()					const	{ return _func;				}
		Term*			arg(unsigned int n)		const	{ return _args[n];			}
		Formula*		subform(unsigned int n)	const	{ assert(false); return 0;	}
		Term*			subterm(unsigned int n)	const	{ return _args[n];			}
		SetExpr*		subset(unsigned int n)	const	{ assert(false); return 0;	}
		Variable*		qvar(unsigned int n)	const	{ assert(false); return 0;	}
		unsigned int	nrSubforms()			const	{ return 0;					}
		unsigned int	nrSubterms()			const	{ return _args.size();		}
		unsigned int	nrSubsets()				const	{ return 0;					}
		unsigned int	nrQvars()				const	{ return 0;					}

		// Visitor
		void	accept(Visitor* v);
		Term*	accept(MutatingVisitor* v);

		// Debugging
		string to_string() const;

};

/** Domain elements **/

class DomainTerm : public Term {

	private:
		Sort*		_sort;		// The sort of the domain element
		ElementType	_type;		// Whether the term is an int, double or string
		Element		_value;		// The value of the domain element
		

	public:

		// Constructors
		DomainTerm(Sort* s, ElementType t, Element v, ParseInfo* pi) : 
			Term(pi), _sort(s), _type(t), _value(v) { assert(s); setfvars(); }

		DomainTerm* clone()								const;
		DomainTerm* clone(const map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete();

		// Inspectors
		Sort*			sort()					const { return _sort;				}
		unsigned int	nrSubforms()			const { return 0;					}
		unsigned int	nrSubterms()			const { return 0;					}
		unsigned int	nrSubsets()				const { return 0;					}
		unsigned int	nrQvars()				const { return 0;					}
		Formula*		subform(unsigned int n)	const { assert(false); return 0;	}
		Term*			subterm(unsigned int n)	const { assert(false); return 0;	}
		SetExpr*		subset(unsigned int n)	const { assert(false); return 0;	}
		Variable*		qvar(unsigned int n)	const { assert(false); return 0;	}
		Element			value()					const { return _value;				}
		ElementType		type()					const { return _type;				}

		// Visitor
		void	accept(Visitor* v);
		Term*	accept(MutatingVisitor* v);

		// Debugging
		string	to_string()	const;	

};


/*****************
	Aggregates
*****************/

/** Abstract base class for set expressions **/
class SetExpr {

	protected:
		
		vector<Variable*>	_fvars;	// The free variables of the set expression
		ParseInfo*			_pi;	// the place where the set was parsed (0 for non user-defined sets)

	public:

		// Constructors
		SetExpr(ParseInfo* pi) : _pi(pi) { }

		virtual SetExpr* clone()								const = 0;
		virtual SetExpr* clone(const map<Variable*,Variable*>&)	const = 0;

		// Destructor
		virtual void recursiveDelete() = 0;
		virtual ~SetExpr() { if(_pi) delete(_pi); }

		// Mutators
		void	setfvars();

		// Inspectors
				unsigned int	nrFvars()				const { return _fvars.size();	}
		virtual	unsigned int	nrSubforms()			const = 0;	
		virtual	unsigned int	nrSubterms()			const = 0;	
		virtual	unsigned int	nrQvars()				const = 0;	
				Variable*		fvar(unsigned int n)	const { return _fvars[n];		}	
		virtual	Formula*		subform(unsigned int n)	const = 0;	
		virtual	Term*			subterm(unsigned int n)	const = 0;	
		virtual	Variable*		qvar(unsigned int n)	const = 0;	
		virtual	Sort*			firstargsort()			const = 0;

		// Visitor
		virtual void		accept(Visitor* v) = 0;
		virtual SetExpr*	accept(MutatingVisitor* v) = 0;

		// Debugging
		virtual string	to_string()	const = 0;

};

/** Set expression of the form [ (phi1,w1); ... ; (phin,wn) ] **/
class EnumSetExpr : public SetExpr {

	private:
		
		vector<Formula*>	_subf;
		vector<Term*>		_weights;

	public:

		// Constructors
		EnumSetExpr(const vector<Formula*>& s, const vector<Term*>& w, ParseInfo* pi) : 
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
		Variable*		qvar(unsigned int n)	const	{ assert(false); return 0;	}
		Sort*			firstargsort()			const;

		// Visitor
		void		accept(Visitor* v);
		SetExpr*	accept(MutatingVisitor* v);

		// Debugging
		string	to_string()	const;	

};

/** Set expression of the form { x1 ... xn : phi } **/
class QuantSetExpr : public SetExpr {

	private:

		Formula*			_subf;
		vector<Variable*>	_vars;

	public:

		// Constructors
		QuantSetExpr(const vector<Variable*>& v, Formula* s, ParseInfo* pi) : 
			SetExpr(pi), _subf(s), _vars(v) { setfvars(); }

		QuantSetExpr* clone()								const;
		QuantSetExpr* clone(const map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void	subf(Formula* f)	{ _subf = f;	}

		// Inspectors
		unsigned int	nrSubforms()			const	{ return 1;					}	
		unsigned int	nrSubterms()			const	{ return 0;					}
		unsigned int	nrQvars()				const	{ return _vars.size();		}
		Formula*		subform(unsigned int n)	const	{ return _subf;				}
		Term*			subterm(unsigned int n)	const	{ assert(false); return 0;	}
		Variable*		qvar(unsigned int n)	const	{ return _vars[n];			}
		Formula*		subf()					const	{ return _subf;				}
		Sort*			firstargsort()			const;

		// Visitor
		void		accept(Visitor* v);
		SetExpr*	accept(MutatingVisitor* v);

		// Debugging
		string	to_string()	const;	

};

/** Aggregate types **/
enum AggType { AGGCARD, AGGSUM, AGGPROD, AGGMIN, AGGMAX };

/** Aggregate term **/
class AggTerm : public Term {

	private:
		
		SetExpr*	_set;
		AggType		_type;

	public:

		// Constructors
		AggTerm(SetExpr* s, AggType t, ParseInfo* pi) : 
			Term(pi), _set(s), _type(t) { setfvars(); }

		AggTerm* clone()								const;
		AggTerm* clone(const map<Variable*,Variable*>&)	const;

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
		SetExpr*		subset(unsigned int n)	const	{ return _set;					}
		Variable*		qvar(unsigned int n)	const	{ return _set->qvar(n);			}
		SetExpr*		set()					const	{ return _set;					}
		AggType			type()					const	{ return _type;					}

		// Visitor
		void	accept(Visitor* v);
		Term*	accept(MutatingVisitor* v);

		// Debugging
		string	to_string()	const;	

};

namespace TermUtils {
	// evaluate the given term in the given structure under the given variable mapping
	TypedElement		evaluate(Term*,Structure*,const map<Variable*,TypedElement>&);	
}

class TermEvaluator : public Visitor {

	private:
		TypedElement				_returnvalue;
		Structure*					_structure;
		map<Variable*,TypedElement>	_varmapping;

	public:
		TermEvaluator(Structure* s,const map<Variable*,TypedElement> m);
		TermEvaluator(Term* t,Structure* s,const map<Variable*,TypedElement> m);

		TypedElement returnvalue()	{ return _returnvalue;	}

		void visit(VarTerm* vt);
		void visit(FuncTerm* ft);
		void visit(DomainTerm* dt);
		void visit(AggTerm* at);
		
};


#endif 
