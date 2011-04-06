/************************************
	theory.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef THEORY_HPP
#define THEORY_HPP

#include <set>
#include <vector>
#include "parseinfo.hpp"

/*****************************************************************************
	Abstract base class for formulas, definitions and fixpoint definitions
*****************************************************************************/

class TheoryVisitor;
class TheoryMutatingVisitor;

/**
 *	Abstract base class to represent formulas, definitions, and fixpoint definitions.
 */
class TheoryComponent {

	public:
		// Constructor
		TheoryComponent() { }

		// Cloning
		virtual TheoryComponent*	clone()	const = 0;	//!< Construct a deep copy of the component

		// Visitor
		virtual void				accept(TheoryVisitor*)			const = 0;
		virtual TheoryComponent*	accept(TheoryMutatingVisitor*) 	= 0;

		// Output
		virtual std::ostream& put(std::ostream&, unsigned int spaces = 0)	const = 0;
				std::string to_string(unsigned int spaces = 0)				const;
};

/***************
	Formulas
***************/

/**
 * Abstract base class to represent formulas
 */
class Formula : public TheoryComponent {

	protected:
		bool					_sign;			//!< true iff the formula does not start with a negation
		std::set<Variable*>		_freevars;		//!< the free variables of the formula
		std::set<Variable*>		_quantvars;		//!< the quantified variables of the formula
		std::vector<Term*>		_subterms;		//!< the direct subterms of the formula
		std::vector<Formula*>	_subformulas;	//!< the direct subformulas of the formula
		FormulaParseInfo		_pi;			//!< the place where the formula was parsed 

		void	setfvars();		//!< compute the free variables of the formula

	public:

		// Constructor
		Formula(bool sign) : _sign(sign) { }
		Formula(bool sign, const FormulaParseInfo& pi): _sign(sign), _pi(pi)  { }

		// Virtual constructors
		virtual	Formula*	clone()										const = 0;	
			//!< copy the formula while keeping the free variables
		virtual	Formula*	clone(const std::map<Variable*,Variable*>&)	const = 0;	
			//!< copy the formulas, and replace the free variables as inidicated by the map

		// Destructor
				void recursiveDelete();	//!< delete the formula and all its children (subformulas, subterms, etc)
		virtual ~Formula() { }			//!< delete the formula, but not its children

		// Mutators
		void	swapsign()	{ _sign = !_sign;	}	//!< swap the sign of the formula

		// Inspectors
				bool					sign()						const { return _sign;			}
				const FormulaParseInfo&	pi()						const { return _pi;				}
				bool					contains(const Variable*)	const;	//!< true iff the formula contains the variable
				bool					contains(const PFSymbol*)	const;	//!< true iff the formula contains the symbol
		virtual	bool					trueformula()				const { return false;	}	
			//!< true iff the formula is the empty conjunction
		virtual	bool					falseformula()				const { return false;	}
			//!< true iff the formula is the empty disjunction

		// Visitor
		virtual void		accept(TheoryVisitor* v)			const = 0;
		virtual Formula*	accept(TheoryMutatingVisitor* v)	= 0;

		// Output
		virtual std::ostream& to_string(std::ostream&, unsigned int spaces = 0)	const = 0;
};

/** 
 * Class to represent atomic formulas, that is, a predicate or function symbol applied to a tuple of terms.
 * If F is a function symbol, the atomic formula F(t_1,...,t_n) represents the formula F(t_1,...,t_n-1) = t_n.
 */
class PredForm : public Formula {
	private:
		PFSymbol*			_symbol;		//!< the predicate or function symbol

	public:
		// Constructors
		PredForm(bool sign, PFSymbol* s, const std::vector<Term*>& a, const FormulaParseInfo& pi) : 
			Formula(sign,pi), _symbol(s) { _subterms = a; setfvars(); }

		PredForm*	clone()										const;
		PredForm*	clone(const std::map<Variable*,Variable*>&)	const;

		~PredForm() { }

		// Mutators
		void	symbol(PFSymbol* s)				{ _symbol = s;					}
		void	arg(unsigned int n, Term* t)	{ _subterms[n] = t;	setfvars(); }

		// Inspectors
		PFSymbol*					symbol()	const { return _symbol;		}
		const std::vector<Term*>&	args()		const { return _subterms;	}
		
		// Visitor
		void		accept(TheoryVisitor* v) const;
		Formula*	accept(TheoryMutatingVisitor* v);

		// Output
		std::ostream& put(std::ostream&,unsigned int spaces = 0) const;
};

/** 
 * Class to represent chains of equalities and inequalities 
 */
class EqChainForm : public Formula {
	private:
		bool					_conj;	//!< Indicates whether the chain is a conjunction or disjunction of (in)equalties
		std::vector<CompType>	_comps;	//!< The consecutive comparisons in the chain

	public:
		// Constructors
		EqChainForm(bool sign, bool c, Term* t, const FormulaParseInfo& pi) : 
			Formula(sign,pi), _conj(c), _comps(0) { _subterms = std::vector<Term*>(1,t); setfvars(); }
		EqChainForm(bool s,bool c,const std::vector<Term*>& vt,const std::vector<CompType>& vc,const FormulaParseInfo& pi) :
			Formula(s,pi), _conj(c), _comps(vc) { _subterms = vt; setfvars();	}

		EqChainForm*	clone()										const;
		EqChainForm*	clone(const std::map<Variable*,Variable*>&)	const;

	    // Destructor
		~EqChainForm() { }

		// Mutators
		void add(CompType ct, Term* t)		{ _comps.push_back(ct); _subterms.push_back(t);	setfvars();	}
		void conj(bool b)					{ _conj = b;												}
		void term(unsigned int n, Term* t)	{ _subterms[n] = t; setfvars();								}

		// Inspectors
		bool							conj()	const { return _conj;	}
		const std::vector<CompType>&	comps()	const { return _comps;	}

		// Visitor
		void		accept(TheoryVisitor* v) const;
		Formula*	accept(TheoryMutatingVisitor* v);

		// Output
		std::ostream& put(std::ostream&, unsigned int spaces = 0) const;
};

/** 
 * Equivalences 
 */
class EquivForm : public Formula {

	public:
		// Constructors
		EquivForm(bool sign, Formula* lf, Formula* rf, const FormulaParseInfo& pi) : 
			Formula(sign,pi) { _subformulas.push_back(lf); _subformulas.push_back(rf); setfvars(); }

		EquivForm*	clone()										const;
		EquivForm*	clone(const std::map<Variable*,Variable*>&)	const;

	    // Destructor
		~EquivForm() { }	

		// Mutators
		void left(Formula* f)	{ _subformulas[0] = f; setfvars();	}
		void right(Formula* f)	{ _subformulas[1] = f; setfvars();	}

		// Inspectors
		Formula*		left()					const { return _subformulas.front();	}
		Formula*		right()					const { return _subformulas.back();		}

		// Visitor
		void		accept(TheoryVisitor* v) const;
		Formula*	accept(TheoryMutatingVisitor* v);

		// Output
		std::ostream& put(std::ostream&,unsigned int spaces = 0) const;
};

/** 
 * Conjunctions and disjunctions 
 */
class BoolForm : public Formula {
	private:
		bool	_conj;	//!< true (false) if the formula is a conjunction (disjunction)
									
	public:
		// Constructors
		BoolForm(bool sign, bool c, const std::vector<Formula*>& sb, const FormulaParseInfo& pi) :
			Formula(sign,pi), _conj(c) { _subformulas = sb; setfvars(); }

		BoolForm*	clone()										const;
		BoolForm*	clone(const std::map<Variable*,Variable*>&)	const;

	    // Destructor
		~BoolForm() { }

		// Mutators
		void	conj(bool b)							{ _conj = b;						}
		void	subf(unsigned int n, Formula* f)		{ _subformulas[n] = f; setfvars();	}
		void	subf(const std::vector<Formula*>& s)	{ _subformulas = s;	setfvars();		}

		// Inspectors
		bool			conj()					const	{ return _conj;										}
		bool			trueformula()			const	{ return (_subformulas.empty() && _conj == _sign);	}
		bool			falseformula()			const	{ return (_subformulas.empty() && _conj != _sign);	}

		// Visitor
		void		accept(TheoryVisitor* v) const;
		Formula*	accept(TheoryMutatingVisitor* v);

		// Debugging
		std::ostream& put(std::ostream&, unsigned int spaces = 0)	const;
};

/** 
 *	Universally and existentially quantified formulas 
 */	
class QuantForm : public Formula {
	private:
		bool	_univ;	// true (false) if the quantifier is universal (existential)

	public:
		// Constructors
		QuantForm(bool sign, bool u, const std::vector<Variable*>& v, Formula* sf, const FormulaParseInfo& pi) : 
			Formula(sign,pi), _vars(v), _subf(sf), _univ(u) { setfvars(); }

		QuantForm*	clone()										const;
		QuantForm*	clone(const std::map<Variable*,Variable*>&)	const;

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
		const std::vector<Variable*>&	qvars()	const { return _vars;				}

		// Visitor
		void		accept(Visitor* v) const;
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		std::string to_string(unsigned int spaces = 0)	const;
};

/** Aggregate atoms **/

class AggForm : public Formula {
	private:
		char		_comp;	// '=', '<', or '>'
		Term*		_left;
		AggTerm*	_right;

	public:
		// Constructors
		AggForm(bool sign, char c, Term* l, AggTerm* r, const FormulaParseInfo& pi) : 
			Formula(sign,pi), _comp(c), _left(l), _right(r) { setfvars(); }

		AggForm*	clone()										const;
		AggForm*	clone(const std::map<Variable*,Variable*>&)	const;

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
		char			comp()					const { return _comp;					}

		// Visitor
		void		accept(Visitor* v) const;
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		std::string to_string(unsigned int spaces = 0)	const;
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

		BracketForm*	clone()										const;
		BracketForm*	clone(const std::map<Variable*,Variable*>&)	const;

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
		void		accept(Visitor* v) const;
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		std::string to_string(unsigned int spaces = 0) const;
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
	TruthValue evaluate(Formula*,AbstractStructure*,const std::map<Variable*,TypedElement>&);	
	
	Formula* remove_eqchains(Formula*,Vocabulary* v = 0);	// Rewrite chains of equalities to a 
															// conjunction or disjunction of atoms.
	Formula* graph_functions(Formula* f);	// Rewrite a function F(x) = y in an equality as a predicate F(x,y)

	Formula* moveThreeValTerms(Formula*,AbstractStructure*,bool positive,bool usingcp=false);	// non-recursively moves terms 
																								// that are three-valued in the given structure
																								// outside of the given atom

	bool monotone(const AggForm* af);
	bool antimonotone(const AggForm* af);
}


/******************
	Definitions
******************/

class Rule {
	private:
		PredForm*				_head;
		Formula*				_body;
		std::vector<Variable*>	_vars;	// The universally quantified variables
		ParseInfo				_pi;

	public:
		// Constructors
		Rule(const std::vector<Variable*>& vv, PredForm* h, Formula* b, const ParseInfo& pi) : 
			_head(h), _body(b), _vars(vv), _pi(pi) { }

		Rule*	clone()	const;

		// Destructor
		~Rule() { }
		void recursiveDelete();

		// Mutators
		void	body(Formula* f)	{ _body = f;	}

		// Inspectors
		PredForm*						head()					const { return _head;			}
		Formula*						body()					const { return _body;			}
		const ParseInfo&				pi()					const { return _pi;				}
		unsigned int					nrQvars()				const { return _vars.size();	}
		Variable*						qvar(unsigned int n)	const { return _vars[n];		}
		const std::vector<Variable*>&	qvars()					const { return _vars;			}

		// Visitor
		void	accept(Visitor* v) const;
		Rule*	accept(MutatingVisitor* v);

		// Debug
		std::string to_string() const;
};

class AbstractDefinition : public TheoryComponent {
	public:
		virtual AbstractDefinition* clone() const = 0;

		// Destructor
		virtual ~AbstractDefinition() { }

		// Visitor
		virtual void				accept(Visitor* v) const	= 0;
		virtual AbstractDefinition*	accept(MutatingVisitor* v)	= 0;
};

class Definition : public AbstractDefinition {
	private:
		std::vector<Rule*>		_rules;		// The rules in the definition
		std::vector<PFSymbol*>	_defsyms;	// Symbols defined by the definition

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
		void		accept(Visitor* v) const;
		Definition*	accept(MutatingVisitor* v);

		// Debug
		std::string to_string(unsigned int spaces = 0) const;
};

class FixpDef : public AbstractDefinition {
	private:
		bool					_lfp;		// True iff it is a least fixpoint definition
		std::vector<FixpDef*>	_defs;		// The direct subdefinitions  of the definition
		std::vector<Rule*>		_rules;		// The rules of the definition
		std::vector<PFSymbol*>	_defsyms;	// The predicates in heads of rules in _rules

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
		void		accept(Visitor* v) const;
		FixpDef*	accept(MutatingVisitor* v);

		// Debug
		std::string to_string(unsigned int spaces = 0) const;
};


/***************
	Theories
***************/

class AbstractTheory {
	protected:
		std::string		_name;
		Vocabulary*		_vocabulary;
		ParseInfo		_pi;

	public:
		// Constructors 
		AbstractTheory(const std::string& name, const ParseInfo& pi) : _name(name), _vocabulary(0), _pi(pi) { }
		AbstractTheory(const std::string& name, Vocabulary* voc, const ParseInfo& pi) : _name(name), _vocabulary(voc), _pi(pi) { }

		virtual AbstractTheory* clone() const = 0;

		// Destructor
		virtual void recursiveDelete() = 0;
		virtual ~AbstractTheory() { }

		// Mutators
				void	vocabulary(Vocabulary* v)	{ _vocabulary = v;	}
				void	name(const std::string& n)	{ _name = n;		}
		virtual	void	add(Formula* f)				= 0;	// Add a formula to the theory
		virtual void	add(Definition* d)			= 0;	// Add a definition to the theory
		virtual void	add(FixpDef* fd)			= 0;	// Add a fixpoint definition to the theory

		// Inspectors
				const std::string&	name()						const { return _name;				}
				Vocabulary*			vocabulary()				const { return _vocabulary;			}
				const ParseInfo&	pi()						const { return _pi;					}
		virtual	unsigned int		nrSentences()				const = 0;	// the number of sentences in the theory
		virtual	unsigned int		nrDefinitions()				const = 0;	// the number of definitions in the theory
		virtual unsigned int		nrFixpDefs()				const = 0;	// the number of fixpoind definitions in the theory
				unsigned int		nrComponents()				const { return nrSentences() + nrDefinitions() + nrFixpDefs();	 }
		virtual Formula*			sentence(unsigned int n)	const = 0;	// the n'th sentence in the theory
		virtual AbstractDefinition*	definition(unsigned int n)	const = 0;	// the n'th definition in the theory
		virtual AbstractDefinition*	fixpdef(unsigned int n)		const = 0;  // the n'th fixpoint definition in the theory
				TheoryComponent*	component(unsigned int n)	const;

		// Visitor
		virtual void			accept(Visitor*) const		= 0;
		virtual AbstractTheory*	accept(MutatingVisitor*)	= 0;

		// Debugging
		virtual std::string to_string() const = 0;
};

class Theory : public AbstractTheory {
	private:
		std::vector<Formula*>		_sentences;
		std::vector<Definition*>	_definitions;
		std::vector<FixpDef*>		_fixpdefs;

	public:
		// Constructors 
		Theory(const std::string& name, const ParseInfo& pi) : AbstractTheory(name,pi) { }
		Theory(const std::string& name, Vocabulary* voc, const ParseInfo& pi) : AbstractTheory(name,voc,pi) { }

		Theory*	clone()	const;

		// Destructor
		void recursiveDelete();

		// Mutators
		void	add(Formula* f)								{ _sentences.push_back(f);		}
		void	add(Definition* d)							{ _definitions.push_back(d);	}
		void	add(FixpDef* fd)							{ _fixpdefs.push_back(fd);		}
		void	add(Theory* t);
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
		void	accept(Visitor*) const;
		Theory*	accept(MutatingVisitor*);

		// Debugging
		std::string to_string() const;
};

namespace TheoryUtils {
	/** Rewriting theories **/
	void push_negations(AbstractTheory*);	// Push negations inside
	void remove_equiv(AbstractTheory*);		// Rewrite A <=> B to (A => B) & (B => A)
	void flatten(AbstractTheory*);			// Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc.
	void remove_eqchains(AbstractTheory*);	// Rewrite chains of equalities to a conjunction or disjunction of atoms.
	void move_quantifiers(AbstractTheory*);	// Rewrite (! x : phi & chi) to ((! x : phi) & (!x : chi)), and similarly for ?.
	void move_functions(AbstractTheory*);
	// TODO  Merge definitions

	/** Tseitin transformation **/
	// Apply the Tseitin transformation, using (where possible) implications to define new predicates.
	void tseitin(AbstractTheory*);	
	
	/** Reduce theories **/
	void reduce(AbstractTheory*, AbstractStructure*);		// Replace ground atoms by their truth value in s

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
	GroundTheory* convert_to_ecnf(AbstractTheory*);
}

#endif
