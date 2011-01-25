/************************************
	theory.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef THEORY_HPP
#define THEORY_HPP

#include "structure.hpp"
#include "term.hpp"

class GroundTranslator;
class EcnfTheory;

/***************
	Formulas
***************/

/** Abstract base class **/

class Formula {

	protected:

		bool				_sign;	// true iff the formula does not start with a negation
		vector<Variable*>	_fvars;	// free variables of the formula
		FormParseInfo		_pi;	// the place where the formula was parsed (0 for non user-defined formulas)

	public:

		// Constructor
		Formula(bool sign) : _sign(sign) { }
		Formula(bool sign, const FormParseInfo& pi):   _sign(sign), _pi(pi)  { }

		// Virtual constructors
		virtual	Formula*	clone()									const = 0;	// copy the formula while keeping the free variables
		virtual	Formula*	clone(const map<Variable*,Variable*>&)	const = 0;	// copy the formulas, and replace the free variables
																				// as inidicated by the map

		// Destructor
		virtual void recursiveDelete() = 0;	// delete the formula and all its children (subformulas, subterms, etc)
		virtual ~Formula() { }				// delete the formula, but not its children

		// Mutators
		void	setfvars();		// compute the free variables
		void	swapsign()	{ _sign = !_sign;	}

		// Inspectors
				bool					sign()					const { return _sign;			}
				unsigned int			nrFvars()				const { return _fvars.size();	}
				Variable*				fvar(unsigned int n)	const { return _fvars[n];		}
				const FormParseInfo&	pi()					const { return _pi;				}
		virtual	unsigned int			nrQvars()				const = 0;	// number of variables quantified by the formula
		virtual	unsigned int			nrSubforms()			const = 0;	// number of direct subformulas
		virtual	unsigned int			nrSubterms()			const = 0;  // number of direct subterms
		virtual	Variable*				qvar(unsigned int n)	const = 0;	// the n'th quantified variable
		virtual	Formula*				subform(unsigned int n)	const = 0;	// the n'th direct subformula
		virtual	Term*					subterm(unsigned int n)	const = 0;	// the n'th direct subterm
				bool					contains(Variable*)		const;		// true iff the formula contains the variable
		virtual	bool					trueformula()			const { return false;	}
		virtual	bool					falseformula()			const { return false;	}

		// Visitor
		virtual void		accept(Visitor* v) = 0;
		virtual Formula*	accept(MutatingVisitor* v) = 0;

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
		PredForm(bool sign, PFSymbol* p, const vector<Term*>& a, const FormParseInfo& pi) : 
			Formula(sign,pi), _symb(p), _args(a) { setfvars(); }

		PredForm*	clone()									const;
		PredForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void	symb(PFSymbol* s)				{ _symb = s;	}
		void	arg(unsigned int n, Term* t)	{ _args[n] = t;	}

		// Inspectors
		PFSymbol*		symb()					const { return _symb;				}
		unsigned int	nrQvars()				const { return 0;					}
		unsigned int	nrSubforms()			const { return 0;					}
		unsigned int	nrSubterms()			const { return _args.size();		}
		Variable*		qvar(unsigned int)		const { assert(false); return 0;	}
		Formula*		subform(unsigned int)	const { assert(false); return 0;	}
		Term*			subterm(unsigned int n)	const { return _args[n];			}
		
		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

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
		EqChainForm(bool sign, bool c, Term* t, const FormParseInfo& pi) : 
			Formula(sign,pi), _conj(c), _terms(1,t), _comps(0), _signs(0) { setfvars(); }
		EqChainForm(bool sign, bool c, const vector<Term*>& vt, const vector<char>& vc, const vector<bool>& vs, const FormParseInfo& pi) :
			Formula(sign,pi), _conj(c), _terms(vt), _comps(vc), _signs(vs) { setfvars();	}

		EqChainForm*	clone()									const;
		EqChainForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void add(char c, bool s, Term* t)		{ _comps.push_back(c); _signs.push_back(s); _terms.push_back(t);	}
		void conj(bool b)						{ _conj = b;														}
		void compsign(unsigned int n, bool b)	{ _signs[n] = b;													}
		void term(unsigned int n, Term* t)		{ _terms[n] = t;													}

		// Inspectors
		bool			conj()						const	{ return _conj;				}
		char			comp(unsigned int n)		const	{ return _comps[n];			}
		bool			compsign(unsigned int n)	const	{ return _signs[n];			}
		unsigned int	nrComps()					const	{ return _comps.size();		}
		unsigned int	nrQvars()					const	{ return 0;					}
		unsigned int	nrSubforms()				const	{ return 0;					}
		unsigned int	nrSubterms()				const	{ return _terms.size();		}
		Variable*		qvar(unsigned int)			const	{ assert(false); return 0;	}
		Formula*		subform(unsigned int)		const	{ assert(false); return 0;	}
		Term*			subterm(unsigned int n)		const	{ return _terms[n];			}

		const vector<char>&	comps()		const	{ return _comps;	}
		const vector<bool>& compsigns()	const	{ return _signs;	}

		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

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
		EquivForm(bool sign, Formula* lf, Formula* rt, const FormParseInfo& pi) : 
			Formula(sign,pi), _left(lf), _right(rt) { setfvars(); }

		EquivForm*	clone()									const;
		EquivForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void left(Formula* f)	{ _left = f;	}
		void right(Formula* f)	{ _right = f;	}

		// Inspectors
		Formula*		left()					const { return _left;					}
		Formula*		right()					const { return _right;					}
		unsigned int	nrQvars()				const { return 0;						}
		unsigned int	nrSubforms()			const { return 2;						}
		unsigned int	nrSubterms()			const { return 0;						}	
		Variable*		qvar(unsigned int)		const { assert(false); return 0;		}
		Formula*		subform(unsigned int n)	const { return (n ? _left : _right);	}
		Term*			subterm(unsigned int)	const { assert(false); return 0;	 	}

		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

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
		BoolForm(bool sign, bool c, const vector<Formula*>& sb, const FormParseInfo& pi) :
			Formula(sign,pi), _subf(sb), _conj(c) { setfvars(); }

		BoolForm*	clone()									const;
		BoolForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void	conj(bool b)						{ _conj = b;	}
		void	subf(unsigned int n, Formula* f)	{ _subf[n] = f;	}
		void	subf(const vector<Formula*>& s)		{ _subf = s;	}

		// Inspectors
		bool			conj()					const	{ return _conj;				}
		Formula*		subf(unsigned int n)	const	{ return _subf[n];			}
		unsigned int	nrQvars()				const	{ return 0;					}
		unsigned int	nrSubforms()			const	{ return _subf.size();		}
		unsigned int	nrSubterms()			const	{ return 0;					}
		Variable*		qvar(unsigned int)		const	{ assert(false); return 0;	}
		Formula*		subform(unsigned int n)	const	{ return _subf[n];			}
		Term*			subterm(unsigned int)	const	{ assert(false); return 0; 	}
		bool			trueformula()			const	{ return (_subf.empty() && _conj == _sign);	}
		bool			falseformula()			const	{ return (_subf.empty() && _conj != _sign);	}


		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

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
		QuantForm(bool sign, bool u, const vector<Variable*>& v, Formula* sf, const FormParseInfo& pi) : 
			Formula(sign,pi), _vars(v), _subf(sf), _univ(u) { setfvars(); }

		QuantForm*	clone()									const;
		QuantForm*	clone(const map<Variable*,Variable*>&)	const;

	   // Destructor
	   void recursiveDelete();

		// Mutators
		void	add(Variable* v)	{ _vars.push_back(v);	}
		void	univ(bool b)		{ _univ = b;			}
		void	subf(Formula* f)	{ _subf = f;			}

		// Inspectors
		Formula*		subf()					const { return _subf;				}
		Variable*		vars(unsigned int n)	const { return _vars[n];			}
		bool			univ()					const { return _univ;				}
		unsigned int	nrQvars()				const { return _vars.size();		}
		unsigned int	nrSubforms()			const { return 1;					}
		unsigned int	nrSubterms()			const { return 0;					}
		Variable*		qvar(unsigned int n)	const { return _vars[n];			}
		Formula*		subform(unsigned int)	const { return	_subf;				}
		Term*			subterm(unsigned int)	const { assert(false); return 0;	}
		const vector<Variable*>&	qvars()		const { return _vars;				}

		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		string to_string()	const;

};


/** Aggregate atoms **/

class AggForm : public Formula {

	private:
		char		_comp;	// '=', '<', or '>'
		Term*		_left;
		AggTerm*	_right;

	public:

		// Constructors
		AggForm(bool sign, char c, Term* l, AggTerm* r, const FormParseInfo& pi) : 
			Formula(sign,pi), _comp(c), _left(l), _right(r) { setfvars(); }

		AggForm*	clone()									const;
		AggForm*	clone(const map<Variable*,Variable*>&)	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void left(Term* t) { _left = t;	}

		// Inspectors
		unsigned int	nrQvars()				const { return 0;						}
		unsigned int	nrSubforms()			const { return 0;						}
		unsigned int	nrSubterms()			const { return 2;						}
		Variable*		qvar(unsigned int)		const { assert(false); return 0;		}
		Formula*		subform(unsigned int)	const { assert(false); return 0;		}
		Term*			subterm(unsigned int n)	const { return (n ? _right : _left);	}
		Term*			left()					const { return _left;					}
		AggTerm*		right()					const { return _right;					}

		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		string to_string()	const;

};

/*******************************************
	Formulas for debugging purposes only
		these formulas will only appear 
		in parseinfo objects
*******************************************/


class BracketForm : public Formula {

	private:
		Formula*		_subf;		// the subformula

	public:

		// Constructors
		BracketForm(bool sign, Formula* subf) : 
			Formula(sign), _subf(subf) { setfvars(); }

		BracketForm*	clone()									const;
		BracketForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void	subf(Formula* f) { _subf = f;	}

		// Inspectors
		Formula*		subf()					const { return _subf;				}
		unsigned int	nrQvars()				const { return 0;					}
		unsigned int	nrSubforms()			const { return 1;					}
		unsigned int	nrSubterms()			const { return 0;					}
		Variable*		qvar(unsigned int)		const { assert(false); return 0;	}
		Formula*		subform(unsigned int)	const { return _subf;				}
		Term*			subterm(unsigned int)	const { assert(false); return 0;	}
		
		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		string to_string() const;

};
/*
class ImplicationFormula : public Formula {
	TODO
};

class RestrQuantFormula : public Formula {
	TODO
};

class NegatedFormula : public Formula {
	TODO
};
*/

// Truth values
enum TruthValue { TV_TRUE, TV_FALSE, TV_UNKN };

namespace FormulaUtils {
	
	/*
	 * Evaluate a formula in a structure under the given variable mapping
	 *	Preconditions: 
	 *		- for all subterms, the preconditions of evaluate(Term*,Structure*,const map<Variable*,TypedElement>&) must hold
	 *		- the sort of every quantified variable in the formula should have a finite domain in the given structure
	 *		- every free variable in the formula is interpreted by the given map
	 */
	TruthValue evaluate(Formula*,AbstractStructure*,const map<Variable*,TypedElement>&);	
																				
}


/******************
	Definitions
******************/

class Rule {

	private:
		PredForm*			_head;
		Formula*			_body;
		vector<Variable*>	_vars;	// The universally quantified variables
		ParseInfo			_pi;

	public:

		// Constructors
		Rule(const vector<Variable*>& vv, PredForm* h, Formula* b, const ParseInfo& pi) : 
			_head(h), _body(b), _vars(vv), _pi(pi) { }

		Rule*	clone()									const;

		// Destructor
		~Rule() { }
		void recursiveDelete();

		// Mutators
		void	body(Formula* f)	{ _body = f;	}

		// Inspectors
		PredForm*			head()					const { return _head;			}
		Formula*			body()					const { return _body;			}
		const ParseInfo&	pi()					const { return _pi;				}
		unsigned int		nrQvars()				const { return _vars.size();	}
		Variable*			qvar(unsigned int n)	const { return _vars[n];		}
		const vector<Variable*>&	qvars()			const { return _vars;			}

		// Visitor
		void	accept(Visitor* v);
		Rule*	accept(MutatingVisitor* v);

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

		Definition*	clone()	const;

		// Destructor
		~Definition() { }
		void recursiveDelete();

		// Mutators
		void	add(Rule*);						// add a rule
		void	add(PFSymbol* p);				// set 'p' to be a defined symbol
		void	rule(unsigned int n, Rule* r)	{ _rules[n] = r;	}
		void	defsyms();						// (Re)compute the list of defined symbols

		// Inspectors
		unsigned int	nrRules()				const { return _rules.size();		}
		Rule*			rule(unsigned int n)	const { return _rules[n];			}
		unsigned int	nrDefsyms()				const { return _defsyms.size();		}
		PFSymbol*		defsym(unsigned int n)	const { return _defsyms[n];			}

		// Visitor
		void		accept(Visitor* v);
		Definition*	accept(MutatingVisitor* v);

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

		FixpDef*	clone()	const;

		// Destructor 
		~FixpDef() { }
		void recursiveDelete();

		// Mutators
		void	add(Rule* r);					// add a rule
		void	add(PFSymbol* p);				// set 'p' to be a defined symbol
		void	add(FixpDef* d)					{ _defs.push_back(d);	}
		void	rule(unsigned int n, Rule* r)	{ _rules[n] = r;		}
		void	def(unsigned int n, FixpDef* d)	{ _defs[n] = d;			}
		void	defsyms();						// (Re)compute the list of defined symbols

		// Inspectors
		bool			lfp()					const { return _lfp;			}
		unsigned int	nrRules()				const { return _rules.size();	}
		unsigned int	nrDefs()				const { return _defs.size();	}
		Rule*			rule(unsigned int n)	const { return _rules[n];		}
		FixpDef*		def(unsigned int n)		const { return _defs[n];		}

		// Visitor
		void		accept(Visitor* v);
		FixpDef*	accept(MutatingVisitor* v);

		// Debug
		string to_string(unsigned int spaces = 0) const;

};


/***************
	Theories
***************/

class AbstractTheory {

	protected:

		string				_name;
		Vocabulary*			_vocabulary;
		ParseInfo			_pi;

	public:

		// Constructors 
		AbstractTheory(const string& name, const ParseInfo& pi) : _name(name), _vocabulary(0), _pi(pi) { }
		AbstractTheory(const string& name, Vocabulary* voc, const ParseInfo& pi) : _name(name), _vocabulary(voc), _pi(pi) { }

		// Destructor
		virtual void recursiveDelete() = 0;
		virtual ~AbstractTheory() { }

		// Mutators
				void	vocabulary(Vocabulary* v)	{ _vocabulary = v;	}
				void	name(const string& n)		{ _name = n;		}
		virtual	void	add(Formula* f)				= 0;	// Add a formula to the theory
		virtual void	add(Definition* d)			= 0;	// Add a definition to the theory
		virtual void	add(FixpDef* fd)			= 0;	// Add a fixpoint definition to the theory

		// Inspectors
				const string&		name()						const { return _name;				}
				Vocabulary*			vocabulary()				const { return _vocabulary;			}
				const ParseInfo&	pi()						const { return _pi;					}
		virtual	unsigned int		nrSentences()				const = 0;	// the number of sentences in the theory
		virtual	unsigned int		nrDefinitions()				const = 0;	// the number of definitions in the theory
		virtual unsigned int		nrFixpDefs()				const = 0;	// the number of fixpoind definitions in the theory
		virtual Formula*			sentence(unsigned int n)	const = 0;	// the n'th sentence in the theory
		virtual Definition*			definition(unsigned int n)	const = 0;	// the n'th definition in the theory
		virtual FixpDef*			fixpdef(unsigned int n)		const = 0;  // the n'th fixpoint definition in the theory

		// Visitor
		virtual void			accept(Visitor*) = 0;
		virtual AbstractTheory*	accept(MutatingVisitor*) = 0;

		// Debugging
		virtual string to_string() const = 0;


};

class Theory : public AbstractTheory {
	
	private:

		vector<Formula*>	_sentences;
		vector<Definition*>	_definitions;
		vector<FixpDef*>	_fixpdefs;

	public:

		// Constructors 
		Theory(const string& name, const ParseInfo& pi) : AbstractTheory(name,pi) { }
		Theory(const string& name, Vocabulary* voc, const ParseInfo& pi) : AbstractTheory(name,voc,pi) { }

		Theory*	clone()	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void	add(Formula* f)								{ _sentences.push_back(f);		}
		void	add(Definition* d)							{ _definitions.push_back(d);	}
		void	add(FixpDef* fd)							{ _fixpdefs.push_back(fd);		}
		void	add(AbstractTheory* t);
		void	sentence(unsigned int n, Formula* f)		{ _sentences[n] = f;			}
		void	definition(unsigned int n, Definition* d)	{ _definitions[n] = d;			}
		void	fixpdef(unsigned int n, FixpDef* d)			{ _fixpdefs[n] = d;				}
		void	pop_sentence()								{ _sentences.pop_back();		}
		void	clear_sentences()							{ _sentences.clear();			}
		void	clear_definitions()							{ _definitions.clear();			}
		void	clear_fixpdefs()							{ _fixpdefs.clear();			}

		// Inspectors
		unsigned int	nrSentences()				const { return _sentences.size();	}
		unsigned int	nrDefinitions()				const { return _definitions.size();	}
		unsigned int	nrFixpDefs()				const { return _fixpdefs.size();	}
		Formula*		sentence(unsigned int n)	const { return _sentences[n];		}
		Definition*		definition(unsigned int n)	const { return _definitions[n];		}
		FixpDef*		fixpdef(unsigned int n)		const { return _fixpdefs[n];		}

		// Visitor
		void	accept(Visitor*);
		Theory*	accept(MutatingVisitor*);

		// Debugging
		string to_string() const;

};

namespace TheoryUtils {

	/** Rewriting theories **/
	void push_negations(AbstractTheory*);	// Push negations inside
	void remove_equiv(AbstractTheory*);		// Rewrite A <=> B to (A => B) & (B => A)
	void flatten(AbstractTheory*);			// Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc.
	void remove_eqchains(AbstractTheory*);	// Rewrite chains of equalities to a conjunction or disjunction of atoms.
	void move_quantifiers(AbstractTheory* t);	// Rewrite (! x : phi & chi) to ((! x : phi) & (!x : chi)), and similarly for ?.
	void move_functions(AbstractTheory* t);
	// TODO  Merge definitions

	/** Tseitin transformation **/
	// Apply the Tseitin transformation, using (where possible) implications to define new predicates.
	void tseitin(AbstractTheory*);	
	
	/** Reduce theories **/
	void reduce(AbstractTheory* t, AbstractStructure* s);		// Replace ground atoms by their truth value in s

	/** Completion **/
	// TODO  Compute completion of definitions
	
	/** ECNF **/
	// Convert the theory to ecnf using the given translator
	//		Preconditions:
	//			1) The input theory is ground
	//				(no quantifiers, no set expressions { x | phi }, no rules with free variables)
	//			2) The only built-in predicates appear in formulas of the form
	//				~(d < agg) or ~(d > agg), where d is a DomainTerm and agg an AggTerm
	//			   These are represented by a PredForm, not by an EqChainForm.
	//			3) Every set in an AggTerm is an EnumSetExpr
	//			4) The only equivalences are sentences of the form (atom <=> aggatom), where 
	//			   atom is a PredForm that does not contain an aggregate 
	//			   and aggatom is a formula of the form mentioned in (2).
	//			5) Aggregate atoms only occur in sentences or rules of the form
	//				(atom <=> aggatom)		NOTE: (aggatom <=> atom) is not allowed
	//				(~atom | aggatom)		NOTE: (aggatom | ~atom) is not allowed
	//				atom <- aggatom
	EcnfTheory*	convert_to_ecnf(AbstractTheory*);
	
}

#endif
