/************************************
	ground.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUND_HPP
#define GROUND_HPP

#include <queue>
#include <stack>
#include <cstdlib> // abs
#include "theory.hpp"
#include "checker.hpp"
#include "generator.hpp"
#include "pcsolver/src/external/ExternalInterface.hpp"

class GroundTheory;

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

/*
 * Enumeration for the possible ways to define a tseitin atom in terms of the subformula it replaces.
 *		TS_EQ:		tseitin <=> subformula
 *		TS_RULE:	tseitin <- subformula
 *		TS_IMPL:	tseitin => subformula
 *		TS_RIMPL:	tseitin <= subformula
 */
enum TsType { TS_EQ, TS_RULE, TS_IMPL, TS_RIMPL };

/*
 * A complete definition of a tseitin atom
*/
struct TsBody {
	vector<int> _body;	// the literals in the subformula replaced by the tseitin
	bool		_conj;	// if true, the replaced subformula is the conjunction of the literals in _body,
						// if false, the replaced subformula is the disjunction of the literals in _body
	TsType		_type;	// the type of "tseitin definition"
};

/*
 *	Ground sets
 */ 
struct GroundSet {
	vector<int>		_setlits;		// All literals in the ground set
	vector<double>	_litweights;	// For each literal a corresponding weight
	vector<double>	_trueweights;	// The weights of the true literals in the set
};

class GroundTranslator  {

	private:
		vector<map<vector<domelement>,int> >		_table;			// map atoms to integers
		vector<PFSymbol*>							_symboffsets;	// map integer to symbol
		vector<PFSymbol*>							_backsymbtable;	// map integer to the symbol of its corresponding atom
		vector<vector<domelement> >					_backargstable;	// map integer to the terms of its corresponding atom

		queue<int>									_freenumbers;		// keeps atom numbers that were freed 
																		// and can be used again
		queue<int>									_freesetnumbers;	// keeps set numbers that were freed
																		// and can be used again

		map<int,TsBody>								_tsbodies;		// keeps mapping between Tseitin numbers and bodies

		vector<GroundSet>							_sets;			// keeps mapping between Set numbers and sets

	public:
		GroundTranslator() : _backsymbtable(1), _backargstable(1), _sets(1) { }

		int							translate(unsigned int,const vector<domelement>&);
		int							translate(const vector<int>& cl, bool conj, TsType tp);
		int							translate(PFSymbol*,const vector<TypedElement>&);
		int							translateSet(const vector<int>&,const vector<double>&,const vector<double>&);
		int							nextNumber();
		PFSymbol*					symbol(int n)	const	{ return _backsymbtable[abs(n)];	}
		const vector<domelement>&	args(int n)		const	{ return _backargstable[abs(n)];	}
		unsigned int				addSymbol(PFSymbol* pfs);
		bool						isTseitin(int l) const	{ return symbol(l) == 0;			}
		const TsBody&				tsbody(int l)	const	{ return _tsbodies.find(abs(l))->second;			}
		const GroundSet&			groundset(int nr)	const	{ return _sets[nr];				}

};


/********************************************************
	Basic top-down, non-optimized grounding algorithm
********************************************************/

class NaiveGrounder : public Visitor {

	private:
		AbstractStructure*			_structure;		// The structure to ground
		map<Variable*,TypedElement>	_varmapping;	// The current assignment of domain elements to variables

		Formula*					_returnFormula;	// The return value when grounding a formula
		SetExpr*					_returnSet;		// The return value when grounding a set
		Term*						_returnTerm;	// The return value when grounding a term
		Definition*					_returnDef;		// The return value when grounding a definition
		FixpDef*					_returnFixpDef;	// The return value when grounding a fixpoint definition
		AbstractTheory*				_returnTheory;	// The return value when grounding a theory

	public:
		NaiveGrounder(AbstractStructure* s) : Visitor(), _structure(s) { }

		AbstractTheory* ground(AbstractTheory* t) { t->accept(this); return _returnTheory;	}

		void visit(VarTerm*);
		void visit(DomainTerm*);
		void visit(FuncTerm*);
		void visit(AggTerm*);

		void visit(EnumSetExpr*);
		void visit(QuantSetExpr*);

		void visit(PredForm*);
		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(QuantForm*);
		void visit(BoolForm*);

		void visit(Rule*);
		void visit(FixpDef*);
		void visit(Definition*);
		void visit(Theory*);

};


/************************************
	Optimized grounding algorithm
************************************/

class TermGrounder;
class FormulaGrounder;
class SetGrounder;
class RuleGrounder;
class DefinitionGrounder;
struct GroundDefinition;

/** Grounding context **/
enum CompContext { CC_SENTENCE, CC_HEAD, CC_BODY, CC_FORMULA };
enum PosContext { PC_POSITIVE, PC_NEGATIVE, PC_BOTH };

struct GroundingContext {
	bool		_truegen;	// Indicates whether the variables are instantiated in order to obtain
							// a ground formula that is possibly true.
	PosContext	_positive;	// Indicates whether the visited part of the theory occurs in the scope
							// of an even number of negations.
	CompContext	_component;	// Indicates the context of the visited formula
	TsType		_tseitin;	// Indicates the type of tseitin definition that needs to be used.
	bool		_recursive; // Indicates whether the visited rule is recursive.
};


/*** Top level grounders ***/

class TopLevelGrounder {
	protected:
		GroundTheory*	_grounding;
	public:
		TopLevelGrounder(GroundTheory* gt) : _grounding(gt) { }

		virtual bool			run()		const = 0;
				GroundTheory*	grounding()	const { return _grounding;	}
};

class EcnfGrounder : public TopLevelGrounder {
	private:
		EcnfTheory*		_original;
	public:
		EcnfGrounder(GroundTheory* gt, EcnfTheory* orig) : TopLevelGrounder(gt), _original(orig) { }
		bool run() const;
};

class TheoryGrounder : public TopLevelGrounder {
	private:
		vector<TopLevelGrounder*>	_grounders;
	public:
		TheoryGrounder(GroundTheory* gt, const vector<TopLevelGrounder*>& fgs) :
			TopLevelGrounder(gt), _grounders(fgs) { }
		bool run() const;
};

class SentenceGrounder : public TopLevelGrounder {
	private:
		bool				_conj;	
		FormulaGrounder*	_subgrounder;
	public:
		SentenceGrounder(GroundTheory* gt, FormulaGrounder* sub, bool conj) : 
			TopLevelGrounder(gt), _subgrounder(sub), _conj(conj) { }
		bool run() const;
};

class UnivSentGrounder : public TopLevelGrounder {
	private:
		TopLevelGrounder*	_subgrounder;
		InstGenerator*		_generator;	
	public:
		UnivSentGrounder(GroundTheory* gt, TopLevelGrounder* sub, InstGenerator* gen) : 
			TopLevelGrounder(gt), _subgrounder(sub), _generator(gen) { }
		bool run() const;
};

/*** Term grounders ***/

class TermGrounder {
	public:
		TermGrounder() { }
		virtual domelement run() const = 0;
};

class DomTermGrounder : public TermGrounder {
	private:
		domelement	_value;
	public:
		DomTermGrounder(domelement val) : _value(val) { }
		domelement run() const { return _value;	}
};

class VarTermGrounder : public TermGrounder {
	private:
		domelement*	_value;
	public:
		VarTermGrounder(domelement* a) : _value(a) { }
		domelement run() const { return *_value;	}
};

class FuncTermGrounder : public TermGrounder {
	private:
		FuncTable*					_function;
		vector<TermGrounder*>		_subtermgrounders;
		mutable vector<domelement>	_args;
	public:
		FuncTermGrounder(const vector<TermGrounder*>& sub, FuncTable* f) :
			_function(f), _args(sub.size()), _subtermgrounders(sub) { }
		domelement run() const;

		// TODO? Optimisation:
		//			Keep all values of the args + result of the previous call to calc().
		//			If the values of the args did not change, return the result immediately instead of doing the
		//			table lookup
};

class AggTermGrounder : public TermGrounder {
	private:
		AggType				_type;
		SetGrounder*		_setgrounder;
		GroundTranslator*	_translator;
	public:
		AggTermGrounder(GroundTranslator* gt, AggType tp, SetGrounder* gr) : _type(tp), _setgrounder(gr), _translator(gt) { }
		domelement run() const;
};


/*** Formula grounders ***/

class FormulaGrounder {
	protected:
		GroundTranslator*	_translator;
		GroundingContext	_context;
	public:
		FormulaGrounder(GroundTranslator* gt, const GroundingContext& ct): _translator(gt), _context(ct) { }
		virtual int		run()				const = 0;
		virtual void	run(vector<int>&)	const = 0;
};

class AtomGrounder : public FormulaGrounder {
	private:
		vector<TermGrounder*>		_subtermgrounders;
		InstanceChecker*			_pchecker;
		InstanceChecker*			_cchecker;
		unsigned int				_symbol;
		mutable vector<domelement>	_args;
		vector<SortTable*>			_tables;
		bool						_sign;
		int							_certainvalue;
	public:
		AtomGrounder(GroundTranslator* gt, bool sign, PFSymbol* s,
					const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
					const vector<SortTable*>& vst, const GroundingContext&);
		int		run() const;
		void	run(vector<int>&) const;
};

class AggGrounder : public FormulaGrounder {
	private:
		AggType			_type;
		SetGrounder*	_setgrounder;
	public:
		AggGrounder(GroundTranslator* tr, GroundingContext gc, AggType tp, SetGrounder* sg) :
			FormulaGrounder(tr,gc), _type(tp), _setgrounder(sg) { }
		int		run()				const;
		void	run(vector<int>&)	const;
};

class ClauseGrounder : public FormulaGrounder {
	protected:
		bool				_sign;
		bool				_conj;
	public:
		ClauseGrounder(GroundTranslator* gt, bool sign, bool conj, const GroundingContext& ct) : 
			FormulaGrounder(gt,ct), _sign(sign), _conj(conj) { }
		int		finish(vector<int>&) const;
		bool	check1(int l) const;
		bool	check2(int l) const;
		int		result1() const;
		int		result2() const;
};

class BoolGrounder : public ClauseGrounder {
	private:
		vector<FormulaGrounder*>	_subgrounders;
	public:
		BoolGrounder(GroundTranslator* gt, const vector<FormulaGrounder*> sub, bool sign, bool conj, const GroundingContext& ct):
			ClauseGrounder(gt,sign,conj,ct), _subgrounders(sub) { }
		int	run() const;
		void	run(vector<int>&) const;
};


class QuantGrounder : public ClauseGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;	
	public:
		QuantGrounder(GroundTranslator* gt, FormulaGrounder* sub, bool sign, bool conj, InstGenerator* gen, const GroundingContext& ct):
			ClauseGrounder(gt,sign,conj,ct), _subgrounder(sub), _generator(gen) { }
		int	run() const;
		void	run(vector<int>&) const;
};

class EquivGrounder : public FormulaGrounder {
	private:
		FormulaGrounder*	_leftgrounder;
		FormulaGrounder*	_rightgrounder;
		bool				_sign;
	public:
		EquivGrounder(GroundTranslator* gt, FormulaGrounder* lg, FormulaGrounder* rg, bool sign, const GroundingContext& ct):
			FormulaGrounder(gt,ct), _leftgrounder(lg), _rightgrounder(rg), _sign(sign) { }
		int run() const;
		void	run(vector<int>&) const;
};


/*** Set grounders ***/

class SetGrounder {
	protected:
		GroundTranslator*	_translator;
	public:
		SetGrounder(GroundTranslator* gt) : _translator(gt) { }
		virtual int run() const = 0;
};

class QuantSetGrounder : public SetGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;	
		domelement*			_weight;
	public:
		QuantSetGrounder(GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, domelement* w) :
			SetGrounder(gt), _subgrounder(gr), _generator(ig), _weight(w) { }
		int run() const;
};

class EnumSetGrounder : public SetGrounder {
	private:
		vector<FormulaGrounder*>	_subgrounders;
		vector<TermGrounder*>		_subtermgrounders;
	public:
		EnumSetGrounder(GroundTranslator* gt, const vector<FormulaGrounder*>& subgr, const vector<TermGrounder*>& subtgr) :
			SetGrounder(gt), _subgrounders(subgr), _subtermgrounders(subtgr) { }
		int run() const;
};


/*** Definition grounders ***/

/** Grounder for a head of a rule **/
class HeadGrounder {
	private:
		GroundTheory*				_grounding;
		vector<TermGrounder*>		_subtermgrounders;
		InstanceChecker*			_truechecker;
		InstanceChecker*			_falsechecker;
		unsigned int				_symbol;
		mutable vector<domelement>	_args;
		vector<SortTable*>			_tables;
	public:
		HeadGrounder(GroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s, 
			const vector<TermGrounder*>&, const vector<SortTable*>&);
		int	run()	const;

};

/** Grounder for a single rule **/
class RuleGrounder {
	private:
		GroundDefinition*	_definition;
		HeadGrounder*		_headgrounder;
		FormulaGrounder*	_bodygrounder;
		InstGenerator*		_headgenerator;	
		InstGenerator*		_bodygenerator;	
		GroundingContext	_context;
		bool				_conj;
//		bool				_recursive;
	public:
		RuleGrounder(GroundDefinition* def, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, bool conj, GroundingContext& ct) :
			_definition(def), _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig), _bodygenerator(big), _conj(conj), _context(ct) { }
		bool run() const;
};

/** Grounder for a definition **/
class DefinitionGrounder : public TopLevelGrounder {
	private:
		GroundDefinition*		_definition;	// The ground definition that will be produced by running the grounder.
		vector<RuleGrounder*>	_subgrounders;	// Grounders for the rules of the definition.
	public:
		DefinitionGrounder(GroundTheory* gt, GroundDefinition* def, vector<RuleGrounder*> subgr) :
			TopLevelGrounder(gt), _definition(def), _subgrounders(subgr) { }
		bool run() const;
};


/***********************
	Grounder Factory
***********************/

/*
 * Class to produce grounders 
 */
class GrounderFactory : public Visitor {

	private:
		// Data
		AbstractStructure*	_structure;		// The structure that will be used to reduce the grounding
		GroundTheory*		_grounding;		// The ground theory that will be produced

		// Context
		GroundingContext		_context;
		stack<GroundingContext>	_contextstack;

		void	InitContext();		// Initialize the context 
		void	SaveContext();		// Push the current context onto the stack
		void	RestoreContext();	// Set _context to the top of the stack and pop the stack
		void	DeeperContext(bool);

		// Variable mapping
		map<Variable*,domelement*>	_varmapping;	// Maps variables to their counterpart during grounding.
													// That is, the corresponding domelement* acts as a variable+value.

		// Current ground definition
		GroundDefinition*		_definition;	// The ground definition that will be produced by the 
												// currently constructed definition grounder.

		// Is last visited formula a conjunction?
		bool	_conjunction;

		// Return values
		FormulaGrounder*		_formgrounder;
		TermGrounder*			_termgrounder;
		SetGrounder*			_setgrounder;
		TopLevelGrounder*		_toplevelgrounder;
		HeadGrounder*			_headgrounder;
		RuleGrounder*			_rulegrounder;
		DefinitionGrounder*		_defgrounder;

		// Descend in the parse tree while taking care of the context
		void	descend(Formula* f); 
		void	descend(Term* t);
		
	public:
		// Constructor
		GrounderFactory(AbstractStructure* structure): _structure(structure) { }

		// Factory method
		TopLevelGrounder* create(AbstractTheory* theory);
		TopLevelGrounder* create(AbstractTheory* theory, MinisatID::WrappedPCSolver* solver);

		// Visitors
		void visit(EcnfTheory*);
		void visit(Theory*);

		void visit(PredForm*);
		void visit(BoolForm*);
		void visit(QuantForm*);
		void visit(EquivForm*);
		void visit(EqChainForm*);

		void visit(VarTerm*);
		void visit(DomainTerm*);
		void visit(FuncTerm*);
		void visit(AggTerm*);

		void visit(EnumSetExpr*);
		void visit(QuantSetExpr*);

		void visit(Definition*);
		void visit(Rule*);

};

#endif
