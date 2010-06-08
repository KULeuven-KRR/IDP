/************************************
	theory.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef THEORY_H
#define THEORY_H

#include "structure.h"
#include "term.h"

/***************
	Formulas
***************/

/** Abstract base class **/

class Formula {

	protected:

		bool				_sign;	// true iff the formula does not start with a negation
		vector<Variable*>	_fvars;	// free variables of the formula
		ParseInfo*			_pi;	// the place where the formula was parsed (0 for non user-defined formulas)

	public:

		// Constructor
		Formula(bool sign, ParseInfo* pi):   _sign(sign), _pi(pi)  { }

		// Virtual constructors
		virtual	Formula*	clone()									const = 0;	// copy the formula while keeping the free variables
		virtual	Formula*	clone(const map<Variable*,Variable*>&)	const = 0;	// copy the formulas, and replace the free variables
																				// as inidicated by the map

		// Destructor
		virtual ~Formula() { }

		// Mutators
		void	setfvars();		// compute the free variables
		void	swapsign()	{ _sign = !_sign;	}

		// Inspectors
				bool			sign()					const { return _sign;			}
				unsigned int	nrFvars()				const { return _fvars.size();	}
				Variable*		fvar(unsigned int n)	const { return _fvars[n];		}
				ParseInfo*		pi()					const { return _pi;				}
		virtual	unsigned int	nrQvars()				const = 0;	// number of variables quantified by the formula
		virtual	unsigned int	nrSubforms()			const = 0;	// number of direct subformulas
		virtual	unsigned int	nrSubterms()			const = 0;  // number of direct subterms
		virtual	Variable*		qvar(unsigned int n)	const = 0;	// the n'th quantified variable
		virtual	Formula*		subform(unsigned int n)	const = 0;	// the n'th direct subformula
		virtual	Term*			subterm(unsigned int n)	const = 0;	// the n'th direct subterm
				bool			contains(Variable*)		const;		// true iff the formula contains the variable

		// Visitor
		virtual void accept(Visitor* v) = 0;

		// Debugging
		virtual string to_string()	const = 0;
	
};

/** Atoms **/

class PredForm : public Formula {
	
	private:
		PFSymbol*		_symb;		// the predicate or function
		vector<Term*>	_args;		// the arguments

	public:

		// Constructors
		PredForm(bool sign, PFSymbol* p, const vector<Term*>& a, ParseInfo* pi) : 
			Formula(sign,pi), _symb(p), _args(a) { setfvars(); }

		PredForm*	clone()									const;
		PredForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		~PredForm();

		// Mutators
		void	symb(PFSymbol* s)	{ _symb = s;	}

		// Inspectors
		PFSymbol*		symb()					const { return _symb;				}
		unsigned int	nrQvars()				const { return 0;					}
		unsigned int	nrSubforms()			const { return 0;					}
		unsigned int	nrSubterms()			const { return _args.size();		}
		Variable*		qvar(unsigned int n)	const { assert(false); return 0;	}
		Formula*		subform(unsigned int n)	const { assert(false); return 0;	}
		Term*			subterm(unsigned int n)	const { return _args[n];			}
		
		// Visitor
		void accept(Visitor* v);

		// Debugging
		string to_string() const;

};


/** Chains of equalities and inequalities **/

class EqChainForm : public Formula {

	private:
		bool			_conj;		// Indicates whether the chain is a conjunction or disjunction of (in)equalties
		vector<Term*>	_terms;		// The consecutive terms in the chain
		vector<char>	_comps;		// The consecutive comparisons ('=', '>' or '<') in the chain
		vector<bool>	_signs;		// The signs of the consecutive comparisons

	public:

		// Constructors
		EqChainForm(bool sign, bool c, Term* t, ParseInfo* pi) : 
			Formula(sign,pi), _conj(c), _terms(1,t), _comps(0), _signs(0) { setfvars(); }
		EqChainForm(bool sign, bool c, const vector<Term*>& vt, const vector<char>& vc, const vector<bool>& vs, ParseInfo* pi) :
			Formula(sign,pi), _conj(c), _terms(vt), _comps(vc), _signs(vs) { setfvars();	}

		EqChainForm*	clone()									const;
		EqChainForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		~EqChainForm();

		// Mutators
		void add(char c, bool s, Term* t)		{ _comps.push_back(c); _signs.push_back(s); _terms.push_back(t);	}
		void conj(bool b)						{ _conj = b;														}
		void compsign(unsigned int n, bool b)	{ _signs[n] = b;													}

		// Inspectors
		bool			conj()						const	{ return _conj;				}
		bool			compsign(unsigned int n)	const	{ return _signs[n];			}
		unsigned int	nrQvars()					const	{ return 0;					}
		unsigned int	nrSubforms()				const	{ return 0;					}
		unsigned int	nrSubterms()				const	{ return _terms.size();		}
		Variable*		qvar(unsigned int n)		const	{ assert(false); return 0;	}
		Formula*		subform(unsigned int n)		const	{ assert(false); return 0;	}
		Term*			subterm(unsigned int n)		const	{ return _terms[n];			}

		// Visitor
		void accept(Visitor* v);

		// Debugging
		string to_string() const;

};

/** Equivalences **/

class EquivForm : public Formula {
	
	protected:
		Formula*	_left;		// left-hand side formula
		Formula*	_right;		// right-hand side formula

	public:
		
		// Constructors
		EquivForm(bool sign, Formula* lf, Formula* rt, ParseInfo* pi) : 
			Formula(sign,pi), _left(lf), _right(rt) { setfvars(); }

		EquivForm*	clone()									const;
		EquivForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		~EquivForm();

		// Inspectors
		Formula*		left()					const { return _left;					}
		Formula*		right()					const { return _right;					}
		unsigned int	nrQvars()				const { return 0;						}
		unsigned int	nrSubforms()			const { return 2;						}
		unsigned int	nrSubterms()			const { return 0;						}	
		Variable*		qvar(unsigned int n)	const { assert(false); return 0;		}
		Formula*		subform(unsigned int n)	const { return (n ? _left : _right);	}
		Term*			subterm(unsigned int n) const { assert(false); return 0;	 	}

		// Visitor
		void accept(Visitor* v);

		// Debuging
		string to_string() const;

};


/** Conjunctions and disjunctions **/

class BoolForm : public Formula {
	
	private:
		vector<Formula*>	_subf;	// the direct subformulas
		bool				_conj;	// true (false) if the formula is the conjunction (disjunction) of the 
									// formulas in _subf
									
	public:

		// Constructors
		BoolForm(bool sign, bool c, const vector<Formula*>& sb, ParseInfo* pi) :
			Formula(sign,pi), _subf(sb), _conj(c) { setfvars(); }

		BoolForm*	clone()									const;
		BoolForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		~BoolForm();

		// Mutators
		void			conj(bool b)	{ _conj = b;	}

		// Inspectors
		bool			conj()					const	{ return _conj;				}
		Formula*		subf(unsigned int n)	const	{ return _subf[n];			}
		unsigned int	nrQvars()				const	{ return 0;					}
		unsigned int	nrSubforms()			const	{ return _subf.size();		}
		unsigned int	nrSubterms()			const	{ return 0;					}
		Variable*		qvar(unsigned int n)	const	{ assert(false); return 0;	}
		Formula*		subform(unsigned int n)	const	{ return _subf[n];			}
		Term*			subterm(unsigned int n)	const	{ assert(false); return 0; 	}

		// Visitor
		void accept(Visitor* v);

		// Debugging
		string to_string()	const;

};


/** Universally and existentially quantified formulas **/

class QuantForm : public Formula {

	private:
		vector<Variable*>	_vars;	// the quantified variables
		Formula*			_subf;	// the direct subformula
		bool				_univ;	// true (false) if the quantifier is universal (existential)

	public:

		// Constructors
		QuantForm(bool sign, bool u, const vector<Variable*>& v, Formula* sf, ParseInfo* pi) : 
			Formula(sign,pi), _vars(v), _subf(sf), _univ(u) { setfvars(); }

		QuantForm*	clone()									const;
		QuantForm*	clone(const map<Variable*,Variable*>&)	const;

	   // Destructor
	   ~QuantForm();

		// Mutators
		void	univ(bool b)	{ _univ = b;	}

		// Inspectors
		Formula*		subf()					const { return _subf;				}
		Variable*		vars(unsigned int n)	const { return _vars[n];			}
		bool			univ()					const { return _univ;				}
		unsigned int	nrQvars()				const { return _vars.size();		}
		unsigned int	nrSubforms()			const { return 1;					}
		unsigned int	nrSubterms()			const { return 0;					}
		Variable*		qvar(unsigned int n)	const { return _vars[n];			}
		Formula*		subform(unsigned int n)	const { return	_subf;				}
		Term*			subterm(unsigned int n)	const { assert(false); return 0;	}

		// Visitor
		void accept(Visitor* v);

		// Debugging
		string to_string()	const;

};


/******************
	Definitions
******************/

class Rule {

	private:
		PredForm*			_head;
		Formula*			_body;
		vector<Variable*>	_vars;	// The universally quantified variables
		ParseInfo*			_pi;

	public:

		// Constructors
		Rule(const vector<Variable*>& vv, PredForm* h, Formula* b, ParseInfo* pi) : 
			_head(h), _body(b), _vars(vv), _pi(pi) { }

		Rule*	clone()									const;

		// Destructor
		~Rule();

		// Inspectors
		PredForm*		head()					const { return _head;			}
		Formula*		body()					const { return _body;			}
		ParseInfo*		pi()					const { return _pi;				}
		unsigned int	nrQvars()				const { return _vars.size();	}
		Variable*		qvar(unsigned int n)	const { return _vars[n];		}

		// Visitor
		void accept(Visitor* v);

		// Debug
		string to_string() const;

};

class Definition {

	private:
		vector<Rule*>		_rules;		// The rules in the definition
		vector<PFSymbol*>	_defsyms;	// Symbols defined by the definition

	public:

		// Constructors
		Definition() : _rules(0), _defsyms(0) { } 

		Definition*	clone()									const;

		// Destructor
		~Definition();

		// Mutators
		void	add(Rule*);				// add a rule
		void	add(PFSymbol* p);		// set 'p' to be a defined symbol

		// Inspectors
		unsigned int	nrrules()				const { return _rules.size();		}
		Rule*			rule(unsigned int n)	const { return _rules[n];			}
		unsigned int	nrdefsyms()				const { return _defsyms.size();		}
		PFSymbol*		defsym(unsigned int n)	const { return _defsyms[n];			}

		// Visitor
		void accept(Visitor* v);

		// Debug
		string to_string(unsigned int spaces = 0) const;

};

class FixpDef {
	
	private:
		bool				_lfp;		// True iff it is a least fixpoint definition
		vector<FixpDef*>	_defs;		// The direct subdefinitions  of the definition
		vector<Rule*>		_rules;		// The rules of the definition
		vector<PFSymbol*>	_defsyms;	// The predicates in heads of rules in _rules

	public:

		// Constructors
		FixpDef(bool lfp) : _lfp(lfp), _defs(0), _rules(0) { }

		FixpDef*	clone()									const;

		// Destructor 
		~FixpDef();

		// Mutators
		void	add(Rule* r);			// add a rule
		void	add(PFSymbol* p);		// set 'p' to be a defined symbol
		void	add(FixpDef* d)			{ _defs.push_back(d);	}

		// Inspectors
		bool			lfp()					const { return _lfp;			}
		unsigned int	nrrules()				const { return _rules.size();	}
		unsigned int	nrdefs()				const { return _defs.size();	}
		Rule*			rule(unsigned int n)	const { return _rules[n];		}
		FixpDef*		def(unsigned int n)		const { return _defs[n];		}

		// Visitor
		void accept(Visitor* v);

		// Debug
		string to_string(unsigned int spaces = 0) const;

};


/***************
	Theories
***************/

class Theory {
	
	private:

		string				_name;
		Vocabulary*			_vocabulary;
		Structure*			_structure;
		ParseInfo*			_pi;

		vector<Formula*>	_sentences;
		vector<Definition*>	_definitions;
		vector<FixpDef*>	_fixpdefs;

	public:

		// Constructors 
		Theory(const string& name, ParseInfo* pi) : _name(name), _vocabulary(0), _structure(0), _pi(pi) { }
		Theory(const string& name, Vocabulary* voc, Structure* str, ParseInfo* pi) : _name(name), _vocabulary(voc), _structure(str), _pi(pi) { }

		Theory*	clone()									const;

		// Destructor
		~Theory();

		// Mutators
		void	vocabulary(Vocabulary* v)	{ _vocabulary = v;				}
		void	structure(Structure* s)		{ _structure = s;				}
		void	add(Formula* f)				{ _sentences.push_back(f);		}
		void	add(Definition* d)			{ _definitions.push_back(d);	}
		void	add(FixpDef* fd)			{ _fixpdefs.push_back(fd);		}

		// Inspectors
		const string&	name()						const { return _name;				}
		Vocabulary*		vocabulary()				const { return _vocabulary;			}
		Structure*		structure()					const { return _structure;			}
		ParseInfo*		pi()						const { return _pi;					}
		unsigned int	nrSentences()				const { return _sentences.size();	}
		unsigned int	nrDefinitions()				const { return _definitions.size();	}
		unsigned int	nrFixpDefs()				const { return _fixpdefs.size();	}
		Formula*		sentence(unsigned int n)	const { return _sentences[n];		}
		Definition*		definition(unsigned int n)	const { return _definitions[n];		}
		FixpDef*		fixpdef(unsigned int n)		const { return _fixpdefs[n];		}

		// Debugging
		string to_string() const;

};

namespace TheoryUtils {
	// Push negations inside
	void push_negations(Theory*);
	// Flatten formulas
	// TODO
	// Rewrite A <=> B to (A => B) & (B => A)
	// TODO
	// Rewrite chains of equalities to a chain of conjunctions
	// TODO
	// Merge definitions
	// TODO
}

#endif
