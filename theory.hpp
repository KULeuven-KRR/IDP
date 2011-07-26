/************************************
	theory.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef THEORY_HPP
#define THEORY_HPP

#include <set>
#include <vector>
#include "commontypes.hpp"
#include "parseinfo.hpp"

/*****************************************************************************
	Abstract base class for formulas, definitions and fixpoint definitions
*****************************************************************************/

class Function;
class PFSymbol;
class Vocabulary;
class AbstractStructure;
class VarTerm;
class FuncTerm;
class DomainTerm;
class AggTerm;
class EnumSetExpr;
class QuantSetExpr;
class TheoryVisitor;
class TheoryMutatingVisitor;

/**
 *	Abstract base class to represent formulas, definitions, and fixpoint definitions.
 */
class TheoryComponent {
	public:
		// Constructor
		TheoryComponent() { }

		// Destructor
		virtual ~TheoryComponent() { }
		virtual void recursiveDelete() = 0;

		// Cloning
		virtual TheoryComponent*	clone()	const = 0;	//!< Construct a deep copy of the component

		// Visitor
		virtual void				accept(TheoryVisitor*)			const = 0;
		virtual TheoryComponent*	accept(TheoryMutatingVisitor*) 	= 0;

		// Output
		virtual std::ostream& put(std::ostream&, unsigned int spaces = 0)	const = 0;
				std::string toString(unsigned int spaces = 0)				const;
};

std::ostream& operator<<(std::ostream&, const TheoryComponent&);

/***************
	Formulas
***************/

/**
 * Abstract base class to represent formulas
 */
class Formula : public TheoryComponent {
	private:
		bool					_sign;			//!< true iff the formula does not start with a negation
		std::set<Variable*>		_freevars;		//!< the free variables of the formula
		std::set<Variable*>		_quantvars;		//!< the quantified variables of the formula
		std::vector<Term*>		_subterms;		//!< the direct subterms of the formula
		std::vector<Formula*>	_subformulas;	//!< the direct subformulas of the formula
		FormulaParseInfo		_pi;			//!< the place where the formula was parsed 

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
		virtual ~Formula() { }
			//!< delete the formula, but not its children

		// Mutators
				void	negate()	{ _sign = !_sign; if(_pi.original()) _pi.original()->negate();		}	
					//!< swap the sign of the formula

				void	addSubterm(Term* t)								{ _subterms.push_back(t); setFreeVars();		}
				void	addSubformula(Formula* f)						{ _subformulas.push_back(f); setFreeVars();	}
				void	addQuantVar(Variable* v)						{ _quantvars.insert(v); setFreeVars();			}
				void	subterm(unsigned int n, Term* t)				{ _subterms[n] = t; setFreeVars();				}
				void	subformula(unsigned int n, Formula* f)			{ _subformulas[n] = f; setFreeVars();			}
				void	subterms(const std::vector<Term*>& vt)			{ _subterms = vt; setFreeVars();				}
				void	subformulas(const std::vector<Formula*>& vf)	{ _subformulas = vf; setFreeVars();			}
				void	quantvars(const std::set<Variable*>& sv)		{ _quantvars = sv; setFreeVars();				}

		// Inspectors
				bool					sign()						const { return _sign;			}
				const FormulaParseInfo&	pi()						const { return _pi;				}
				bool					contains(const Variable*)	const;	//!< true iff the formula contains the variable
				bool					contains(const PFSymbol*)	const;	//!< true iff the formula contains the symbol
		virtual	bool					trueformula()				const { return false;	}	
			//!< true iff the formula is the empty conjunction
		virtual	bool					falseformula()				const { return false;	}
			//!< true iff the formula is the empty disjunction
			
		const std::set<Variable*>&		freevars()		const { return _freevars;		}
		const std::set<Variable*>&		quantvars()		const { return _quantvars;		}
		const std::vector<Term*>&		subterms()		const { return _subterms;		}
		const std::vector<Formula*>&	subformulas()	const { return _subformulas;	}

		// Visitor
		virtual void		accept(TheoryVisitor* v)			const = 0;
		virtual Formula*	accept(TheoryMutatingVisitor* v)	= 0;

		// Output
		virtual std::ostream& put(std::ostream&, unsigned int spaces = 0)	const = 0;

	private:
		void	setFreeVars();		//!< compute the free variables of the formula
};

std::ostream& operator<<(std::ostream&, const Formula&);

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
			Formula(sign,pi), _symbol(s) { subterms(a); }

		PredForm*	clone()										const;
		PredForm*	clone(const std::map<Variable*,Variable*>&)	const;

		~PredForm() { }

		// Mutators
		void	symbol(PFSymbol* s)				{ _symbol = s;		}
		void	arg(unsigned int n, Term* t)	{ subterm(n,t);		}

		// Inspectors
		PFSymbol*					symbol()	const { return _symbol;		}
		const std::vector<Term*>&	args()		const { return subterms();	}
		
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
			Formula(sign,pi), _conj(c), _comps(0) { subterms(std::vector<Term*>(1,t)); }
		EqChainForm(bool s,bool c,const std::vector<Term*>& vt,const std::vector<CompType>& vc,const FormulaParseInfo& pi) :
			Formula(s,pi), _conj(c), _comps(vc) { subterms(vt); }

		EqChainForm*	clone()										const;
		EqChainForm*	clone(const std::map<Variable*,Variable*>&)	const;

	    // Destructor
		~EqChainForm() { }

		// Mutators
		void add(CompType ct, Term* t)			{ _comps.push_back(ct); addSubterm(t);	}
		void conj(bool b)						{ _conj = b;							}
		void term(unsigned int n, Term* t)		{ subterm(n,t);							}
		void comp(unsigned int n, CompType ct)	{ _comps[n] = ct;						}

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
			Formula(sign,pi) { addSubformula(lf); addSubformula(rf); }

		EquivForm*	clone()										const;
		EquivForm*	clone(const std::map<Variable*,Variable*>&)	const;

	    // Destructor
		~EquivForm() { }	

		// Mutators
		void left(Formula* f)	{ subformula(0,f); 	}
		void right(Formula* f)	{ subformula(1,f);	}

		// Inspectors
		Formula*		left()		const { return subformulas().front();		}
		Formula*		right()		const { return subformulas().back();		}

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
			Formula(sign,pi), _conj(c) { subformulas(sb); }
		BoolForm(bool sign, bool c, Formula* left, Formula* right, const FormulaParseInfo& pi) :
			Formula(sign,pi), _conj(c) { addSubformula(left); addSubformula(right);	}

		BoolForm*	clone()										const;
		BoolForm*	clone(const std::map<Variable*,Variable*>&)	const;

	    // Destructor
		~BoolForm() { }

		// Mutators
		void	conj(bool b)	{ _conj = b;	}

		// Inspectors
		bool	conj()			const	{ return _conj;											}
		bool	trueformula()	const	{ return (subformulas().empty() && _conj == sign());	}
		bool	falseformula()	const	{ return (subformulas().empty() && _conj != sign());	}

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
		QuantForm(bool sign, bool u, const std::set<Variable*>& v, Formula* sf, const FormulaParseInfo& pi) : 
			Formula(sign,pi), _univ(u) { subformulas(std::vector<Formula*>(1,sf)); quantvars(v); }

		QuantForm*	clone()										const;
		QuantForm*	clone(const std::map<Variable*,Variable*>&)	const;

		// Destructor
		~QuantForm() { }

		// Mutators
		void	add(Variable* v)	{ addQuantVar(v);	}
		void	univ(bool b)		{ _univ = b;		}
		void	subf(Formula* f)	{ subformula(0,f);	}

		// Inspectors
		Formula*	subf()	const { return subformulas()[0];	}
		bool		univ()	const { return _univ;				}

		// Visitor
		void		accept(TheoryVisitor* v) const;
		Formula*	accept(TheoryMutatingVisitor* v);

		// Output
		std::ostream& put(std::ostream&,unsigned int spaces = 0)	const;
};

/** 
 * Aggregate atoms
 *
 * Formulas of the form 't op a', where 't' is a term, 'op' a comparison operator and 'a' an aggregate term
 */

class AggForm : public Formula {
	private:
		CompType	_comp;		//!< the comparison operator
		AggTerm*	_aggterm;	//!< the aggregate term

	public:
		// Constructors
		AggForm(bool sign, Term* l, CompType c, AggTerm* r, const FormulaParseInfo& pi); 

		AggForm*	clone()										const;
		AggForm*	clone(const std::map<Variable*,Variable*>&)	const;

		// Destructor
		~AggForm() { }

		// Mutators
		void left(Term* t) { subterm(0,t);	}

		// Inspectors
		Term*		left()		const { return subterms()[0];	}
		AggTerm*	right()		const { return _aggterm;		}
		CompType	comp()		const { return _comp;			}

		// Visitor
		void		accept(TheoryVisitor* v) const;
		Formula*	accept(TheoryMutatingVisitor* v);

		// Output
		std::ostream& put(std::ostream&,unsigned int spaces = 0)	const;
};

namespace FormulaUtils {

	/** \brief Recursively rewrite all EqChainForms in the given formula to BoolForms **/
	Formula* removeEqChains(Formula*,Vocabulary* v = 0);	

	/** **/
	Formula* graphFunctions(Formula* f);	

	/** \brief Non-recursively move terms that are three-valued in a given structure outside of the given atom **/
	Formula* moveThreeValuedTerms(Formula*,AbstractStructure*,bool positive,bool cpsupport=false,
								const std::set<const PFSymbol*> cpsymbols=std::set<const PFSymbol*>());

	/** \brief Returns true iff the aggregate formula is monotone **/
	bool isMonotone(const AggForm* af);

	/** \brief Returns true iff the aggregate formula is anti-monotone **/
	bool isAntimonotone(const AggForm* af);

	/** \brief Create the formula 'true' **/
	BoolForm*	trueform();
	/** \brief Create the formula 'false' **/
	BoolForm*	falseform();
}


/******************
	Definitions
******************/

/**
 * Class to represent a single rule of a definition or fixpoint definition
 */
class Rule {
	private:
		PredForm*				_head;		//!< the head of the rule
		Formula*				_body;		//!< the body of the rule
		std::set<Variable*>		_quantvars;	//!< the universally quantified variables of the rule
		ParseInfo				_pi;		//!< the place where the rule was parsed

	public:
		// Constructors
		Rule(const std::set<Variable*>& vv, PredForm* h, Formula* b, const ParseInfo& pi) : 
			_head(h), _body(b), _quantvars(vv), _pi(pi) { }

		Rule*	clone()	const;	//!< Make a deep copy of the rule

		// Destructor
		~Rule() { }				//!< Delete the rule, but not its components
		void recursiveDelete();	//!< Delete the rule and its components

		// Mutators
		void	head(PredForm* h)	{ _head = h;	}	//!< Replace the head of the rule
		void	body(Formula* f)	{ _body = f;	}	//!< Replace the body of the rule
		void	addvar(Variable* v)	{ _quantvars.insert(v);	}

		// Inspectors
		PredForm*					head()			const { return _head;		}
		Formula*					body()			const { return _body;		}
		const ParseInfo&			pi()			const { return _pi;			}
		const std::set<Variable*>&	quantvars()		const { return _quantvars;	}

		// Visitor
		void	accept(TheoryVisitor* v) const;
		Rule*	accept(TheoryMutatingVisitor* v);

		// Output
		std::ostream&	put(std::ostream&, unsigned int spaces = 0) const;
		std::string		toString(unsigned int spaces = 0)			const;
};

std::ostream& operator<<(std::ostream&,const Rule&);

/**
 * \brief Absract class to represent definitions
 */
class AbstractDefinition : public TheoryComponent {
	public:
		virtual AbstractDefinition* clone() const = 0;

		// Destructor
		virtual ~AbstractDefinition() { }

		// Visitor
		virtual void				accept(TheoryVisitor* v)			const = 0;
		virtual AbstractDefinition*	accept(TheoryMutatingVisitor* v)	= 0;
};

/**
 * \brief Class to represent inductive definitions
 */
class Definition : public AbstractDefinition {
	private:
		std::vector<Rule*>		_rules;		//!< The rules in the definition
		std::set<PFSymbol*>		_defsyms;	//!< Symbols defined by the definition

	public:
		// Constructors
		Definition() { } 

		Definition*	clone()	const;

		// Destructor
		~Definition() { }
		void recursiveDelete();

		// Mutators
		void					add(Rule*);						//!< add a rule to the definition
		void					rule(unsigned int n, Rule* r);	//!< Replace the n'th rule of the definition

		// Inspectors
		const std::vector<Rule*>&		rules()			const { return _rules;		}
		const std::set<PFSymbol*>&		defsymbols()	const { return _defsyms;	}

		// Visitor
		void		accept(TheoryVisitor* v) const;
		Definition*	accept(TheoryMutatingVisitor* v);

		// output
		std::ostream& put(std::ostream&,unsigned int spaces = 0) const;
};

/***************************
	Fixpoint definitions
***************************/

class AbstractFixpDef : public TheoryComponent {
	public:
		virtual AbstractFixpDef* clone() const = 0;

		// Destructor
		virtual ~AbstractFixpDef() { }

		// Visitor
		virtual void				accept(TheoryVisitor* v)			const = 0;
		virtual AbstractFixpDef*	accept(TheoryMutatingVisitor* v)	= 0;
};

class FixpDef : public AbstractFixpDef {
	private:
		bool					_lfp;		//!< true iff it is a least fixpoint definition
		std::vector<FixpDef*>	_defs;		//!< the direct subdefinitions  of the definition
		std::vector<Rule*>		_rules;		//!< the rules of the definition
		std::set<PFSymbol*>		_defsyms;	//!< the predicates in heads of rules in _rules

	public:
		// Constructors
		FixpDef(bool lfp = false) : _lfp(lfp) { }

		FixpDef*	clone()	const;

		// Destructor 
		~FixpDef() { }
		void recursiveDelete();

		// Mutators
		void	lfp(bool b)						{ _lfp = b;				}
		void	add(Rule* r);												//!< add a rule
		void	add(FixpDef* d)					{ _defs.push_back(d);	}	//!< add a direct subdefinition
		void	rule(unsigned int n, Rule* r);								//!< replace the n'th rule
		void	def(unsigned int n, FixpDef* d)	{ _defs[n] = d;			}	//!< replace the n'th subdefinition

		// Inspectors
		bool							lfp()		const { return _lfp;		}
		const std::vector<FixpDef*>&	defs()		const { return _defs;		}
		const std::vector<Rule*>&		rules()		const { return _rules;		}
		const std::set<PFSymbol*>&		defsyms()	const { return _defsyms;	}

		// Visitor
		void		accept(TheoryVisitor* v) const;
		FixpDef*	accept(TheoryMutatingVisitor* v);

		// output
		std::ostream& put(std::ostream&,unsigned int spaces = 0) const;
};


/***************
	Theories
***************/

/**
 * \brief Abstract base class for theories
 */
class AbstractTheory {
	protected:
		std::string		_name;			//!< the name of the theory
		Vocabulary*		_vocabulary;	//!< the vocabulary of the theory
		ParseInfo		_pi;			//!< the place where the theory was parsed

	public:
		// Constructors 
		AbstractTheory(const std::string& name, const ParseInfo& pi) : 
			_name(name), _vocabulary(0), _pi(pi) { }
		AbstractTheory(const std::string& name, Vocabulary* voc, const ParseInfo& pi) : 
			_name(name), _vocabulary(voc), _pi(pi) { }

		virtual AbstractTheory* clone() const = 0;	//!< Make a deep copy of the theory

		// Destructor
		virtual void recursiveDelete() = 0;	//!< Delete the theory and its components
		virtual ~AbstractTheory() { }		//!< Delete the theory, but not its components

		// Mutators
				void	vocabulary(Vocabulary* v)	{ _vocabulary = v;	}	//!< Change the vocabulary of the theory
				void	name(const std::string& n)	{ _name = n;		}	//!< Change the name of the theory
		virtual	void	add(Formula* f)				= 0;	//!< Add a formula to the theory
		virtual void	add(Definition* d)			= 0;	//!< Add a definition to the theory
		virtual void	add(FixpDef* fd)			= 0;	//!< Add a fixpoint definition to the theory

		// Inspectors
		const std::string&	name()			const { return _name;			}
		Vocabulary*			vocabulary()	const { return _vocabulary;		}
		const ParseInfo&	pi()			const { return _pi;				}

		// Visitor
		virtual void			accept(TheoryVisitor*) const	= 0;
		virtual AbstractTheory*	accept(TheoryMutatingVisitor*)	= 0;

		// Output
		virtual std::ostream&	put(std::ostream&,unsigned int spaces)	const = 0;
				std::string		toString(unsigned int spaces)			const;
};

std::ostream& operator<<(std::ostream&,const AbstractTheory&);

/**
 * \brief Class to represent first-order theories
 */
class Theory : public AbstractTheory {
	private:
		std::vector<Formula*>		_sentences;		//!< the sentences of the theory
		std::vector<Definition*>	_definitions;	//!< the definitions of the theory
		std::vector<FixpDef*>		_fixpdefs;		//!< the fixpoint definitions of the theory

	public:
		// Constructors 
		Theory(const std::string& name, const ParseInfo& pi) : 
			AbstractTheory(name,pi) { }
		Theory(const std::string& name, Vocabulary* voc, const ParseInfo& pi) : 
			AbstractTheory(name,voc,pi) { }

		Theory*	clone()	const;

		// Destructor
		void recursiveDelete();
		~Theory() { }

		// Mutators
		void	add(Formula* f)								{ _sentences.push_back(f);		}
		void	add(Definition* d)							{ _definitions.push_back(d);	}
		void	add(FixpDef* fd)							{ _fixpdefs.push_back(fd);		}
		void	addTheory(AbstractTheory*);
		void	sentence(unsigned int n, Formula* f)		{ _sentences[n] = f;			}
		void	definition(unsigned int n, Definition* d)	{ _definitions[n] = d;			}
		void	fixpdef(unsigned int n, FixpDef* d)			{ _fixpdefs[n] = d;				}

		std::vector<Formula*>&		sentences()		{ return _sentences;	}
		std::vector<Definition*>&	definitions()	{ return _definitions;	}
		std::vector<FixpDef*>&		fixpdefs()		{ return _fixpdefs;		}

		// Inspectors
		const std::vector<Formula*>&		sentences()		const { return _sentences;	}
		const std::vector<Definition*>&		definitions()	const { return _definitions;	}
		const std::vector<FixpDef*>&		fixpdefs()		const { return _fixpdefs;		}
		std::set<TheoryComponent*>			components()	const;

		// Visitor
		void	accept(TheoryVisitor*) const;
		Theory*	accept(TheoryMutatingVisitor*);

		// Output
		std::ostream& put(std::ostream&,unsigned int spaces = 0) const;
};

namespace TheoryUtils {

	/** \brief Push negations inside **/
	void pushNegations(AbstractTheory*);	

	/** \brief Rewrite A <=> B to (A => B) & (B => A) **/
	void removeEquiv(AbstractTheory*);		

	/** \brief Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc. **/
	void flatten(AbstractTheory*);			

	/** \brief Rewrite chains of equalities to a conjunction or disjunction of atoms. **/
	void removeEqchains(AbstractTheory*);	

	/** \brief Rewrite (! x : phi & chi) to ((! x : phi) & (!x : chi)), and similarly for ?. **/
	void moveQuantifiers(AbstractTheory*);	

	/** \brief Rewrite the theory so that there are no nested terms **/
	void removeNesting(AbstractTheory*);

	/** \brief Replace all definitions in the theory by their completion **/
	void completion(AbstractTheory*);

	/** \brief Count the number of subformulas in the theory **/
	int nrSubformulas(AbstractTheory*);

	/** \brief Merge two theories **/
	AbstractTheory* merge(AbstractTheory*,AbstractTheory*);
}

/**************
	Visitor
**************/

class GroundTheory;
class SolverTheory;
class GroundDefinition;
class GroundRuleBody;
class PCGroundRuleBody;
class AggGroundRuleBody;
class GroundSet;
class GroundAggregate;

class CPTerm;
class CPVarTerm;
class CPWSumTerm;
class CPSumTerm;
class CPReification;

// All have a default implementation to allow visitors only implementing some of the traversals, the other
// being no-ops.

/**
 * Visitor class for theories
 */
class TheoryVisitor {
	public:
		// Theories
		virtual void visit(const Theory*);
		virtual void visit(const GroundTheory*);
		virtual void visit(const SolverTheory*);

		// Formulas     
				void traverse(const Formula*);
		virtual void visit(const PredForm*);			
		virtual void visit(const EqChainForm*);
		virtual void visit(const EquivForm*);
		virtual void visit(const BoolForm*);
		virtual void visit(const QuantForm*);
		virtual void visit(const AggForm*);

		virtual void visit(const PCGroundRuleBody*);
		virtual void visit(const AggGroundRuleBody*);
		virtual void visit(const GroundSet*);
		virtual void visit(const GroundAggregate*);

		virtual void visit(const CPReification*);

		// Definitions 
		virtual void visit(const Rule*);
		virtual void visit(const Definition*);
		virtual void visit(const FixpDef*);
		virtual void visit(const GroundDefinition*);

		// Terms
				void traverse(const Term*);
		virtual void visit(const VarTerm*);
		virtual void visit(const FuncTerm*);
		virtual void visit(const DomainTerm*);
		virtual void visit(const AggTerm*);

		virtual void visit(const CPVarTerm*);
		virtual void visit(const CPWSumTerm*);
		virtual void visit(const CPSumTerm*);

		// Set expressions
				void traverse(const SetExpr*);
		virtual void visit(const EnumSetExpr*);
		virtual void visit(const QuantSetExpr*);

};

class TheoryMutatingVisitor {
	public:
		// Theories
		virtual Theory* visit(Theory*);
		virtual GroundTheory* visit(GroundTheory*);
		virtual SolverTheory* visit(SolverTheory*);

		// Formulas     
				Formula* traverse(Formula*);
		virtual Formula* visit(PredForm*);			
		virtual Formula* visit(EqChainForm*);
		virtual Formula* visit(EquivForm*);
		virtual Formula* visit(BoolForm*);
		virtual Formula* visit(QuantForm*);
		virtual Formula* visit(AggForm*);

		virtual GroundRuleBody*	visit(AggGroundRuleBody*);
		virtual GroundRuleBody*	visit(PCGroundRuleBody*);
	

		// Definitions 
		virtual Rule*				visit(Rule*);
		virtual Definition*			visit(Definition*);
		virtual FixpDef*			visit(FixpDef*);
		virtual GroundDefinition*	visit(GroundDefinition*);

		// Terms
				Term* traverse(Term*);
		virtual Term* visit(VarTerm*);
		virtual Term* visit(FuncTerm*);
		virtual Term* visit(DomainTerm*);
		virtual Term* visit(AggTerm*);

		// Set expressions
				SetExpr* traverse(SetExpr*);
		virtual SetExpr* visit(EnumSetExpr*);
		virtual SetExpr* visit(QuantSetExpr*);
};


#endif
