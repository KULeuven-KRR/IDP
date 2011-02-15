/************************************
	ground.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUND_HPP
#define GROUND_HPP

#include <queue>
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

//TODO: Probably need some forward declarations here...

class Grounder {
	protected:
		GroundTheory*	_grounding;
		static int		_true;
		static int		_false;

	public:
		Grounder(GroundTheory* gt) : _grounding(gt) { }
		GroundTheory* grounding() const { return _grounding;	}
};


/*** Theory grounders ***/

class AbstractTheoryGrounder : public Grounder {
	public:
		AbstractTheoryGrounder(GroundTheory* gt) : Grounder(gt) { }
		virtual int run() const = 0;
};

class EcnfGrounder : public AbstractTheoryGrounder {
	public:
		EcnfGrounder(GroundTheory* gt) : AbstractTheoryGrounder(gt) { }
		int run() const { return _true; }
};

class TheoryGrounder : public AbstractTheoryGrounder {
	private:
		vector<FormulaGrounder*>	_formulagrounders;
		vector<DefinitionGrounder*>	_definitiongrounders;
	public:
		TheoryGrounder(GroundTheory* gt, const vector<FormulaGrounder*>& fgs, const vector<DefinitionGrounder*>& dgs) :
			AbstractTheoryGrounder(gt), _formulagrounders(fgs), _definitiongrounders(dgs) { }
		int run() const;
};


/*** Term grounders ***/

class TermGrounder : public Grounder {
	public:
		TermGrounder() : Grounder(0) { }
		TermGrounder(GroundTheory* gt) : Grounder(gt) { }
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
		AggType			_type;
		SetGrounder*	_setgrounder;
	public:
		AggTermGrounder(GroundTheory* gt, AggType tp, SetGrounder* gr) : TermGrounder(gt), _type(tp), _setgrounder(gr) { }
		domelement run() const;
};


/*** Formula grounders ***/

class FormulaGrounder : public Grounder {
	private:
		bool			_rulecontext;
		EcnfDefinition* _definition;
		int				_head;
	public:
		FormulaGrounder(GroundTheory* gt): Grounder(gt) { }
		FormulaGrounder(EcnfDefinition* def, int head): Grounder(0), _definition(def), _head(head) { }  
		virtual int	run() const = 0;
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
		bool						_sentence;
		bool						_poscontext;
		int							_certainvalue;
	public:
		AtomGrounder(GroundTheory* gt, bool sign, bool sent, PFSymbol* s,
					const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
					const vector<SortTable*>& vst, bool pc, bool c);
		int run() const;
};

class ClauseGrounder : public FormulaGrounder {
	protected:
		bool				_sign;
		bool				_sentence;
		bool				_conj;
		bool				_poscontext;
	public:
		ClauseGrounder(GroundTheory* gt, bool sign, bool sen, bool conj, bool pos) : 
			FormulaGrounder(gt), _sign(sign), _sentence(sen), _conj(conj), _poscontext(pos) { }
		int		finish(vector<int>&) const;
		bool	check1(int l) const;
		bool	check2(int l) const;
		int		result1() const;
		int		result2() const;
		virtual int	run() const = 0;
};

class BoolGrounder : public ClauseGrounder {
	private:
		vector<FormulaGrounder*>	_subgrounders;
	public:
		BoolGrounder(GroundTheory* gt, const vector<FormulaGrounder*> sub, bool sign, bool sen, bool conj, bool pos):
			ClauseGrounder(gt,sign,sen,conj,pos), _subgrounders(sub) { }
		int	run() const;
};

class QuantGrounder : public ClauseGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;	
	public:
		QuantGrounder(GroundTheory* gt, FormulaGrounder* sub, bool sign, bool sen, bool conj, bool pos, InstGenerator* gen):
			ClauseGrounder(gt,sign,sen,conj,pos), _subgrounder(sub), _generator(gen) { }
		int	run() const;
};

class EquivGrounder : public FormulaGrounder {
	private:
		FormulaGrounder*	_leftgrounder;
		FormulaGrounder*	_rightgrounder;
		bool				_sign;
		bool				_sentence;
		bool				_poscontext;
	public:
		EquivGrounder(GroundTheory* gt, FormulaGrounder* lg, FormulaGrounder* rg, bool sign, bool sen, bool pos):
			FormulaGrounder(gt), _leftgrounder(lg), _rightgrounder(rg), _sign(sign), _sentence(sen), _poscontext(pos) { }
		int run() const;
};


/*** Set grounders ***/

class SetGrounder : public Grounder {
	public:
		SetGrounder(GroundTheory* gt) : Grounder(gt) { }
		virtual int run() const = 0;
};

class QuantSetGrounder : public SetGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;	
		domelement*			_weight;
	public:
		QuantSetGrounder(GroundTheory* gt, FormulaGrounder* gr, InstGenerator* ig, domelement* w) :
			SetGrounder(gt), _subgrounder(gr), _generator(ig), _weight(w) { }
		int run() const;
};

class EnumSetGrounder : public SetGrounder {
	private:
		vector<FormulaGrounder*>	_subgrounders;
		vector<TermGrounder*>		_subtermgrounders;
	public:
		EnumSetGrounder(GroundTheory* gt, const vector<FormulaGrounder*>& subgr, const vector<TermGrounder*>& subtgr) :
			SetGrounder(gt), _subgrounders(subgr), _subtermgrounders(subtgr) { }
		int run() const;
};


/*** Definition grounders ***/

struct EcnfDefinition;

class RuleGrounder : public Grounder {
	private:
		EcnfDefinition*			_definition;
		FormulaGrounder*		_headgrounder;
		FormulaGrounder*		_bodygrounder;
		InstGenerator*			_generator;	
	public:
		RuleGrounder(GroundTheory* gt, EcnfDefinition* def, FormulaGrounder* hgr, FormulaGrounder* bgr, InstGenerator* ig) :
			Grounder(gt), _definition(def), _headgrounder(hgr), _bodygrounder(bgr), _generator(ig) { }
		bool run() const;
};

class DefinitionGrounder : public Grounder {
	private:
		EcnfDefinition*			_definition;
		vector<RuleGrounder*>	_subgrounders;
	public:
		DefinitionGrounder(GroundTheory* gt, EcnfDefinition* def, vector<RuleGrounder*> subgr) :
			Grounder(gt), _definition(def), _subgrounders(subgr) { }
		int run() const;
};


/***********************
	Grounder Factory
***********************/

class GrounderFactory : public Visitor {
	private:
		// Data
		AbstractStructure*	_structure;
		GroundTheory*		_grounding;

		// Context
		bool	_poscontext;
		bool	_truegencontext;
		bool	_sentence;
		bool	_rulecontext;

		// Variable mapping
		map<Variable*,domelement*>	_varmapping;

		// Current ground definition
		EcnfDefinition*			_definition;

		// Return values
		FormulaGrounder*		_grounder;		//TODO: should be renamed to _formgrounder or something for consistency
		TermGrounder*			_termgrounder;
		SetGrounder*			_setgrounder;
		AbstractTheoryGrounder*	_theogrounder;
		RuleGrounder*			_rulegrounder;
		DefinitionGrounder*		_defgrounder;

	public:
		// Constructor
		GrounderFactory(AbstractStructure* structure): _structure(structure) { }

		// Factory method
		AbstractTheoryGrounder* create(AbstractTheory* theory);
		AbstractTheoryGrounder* create(AbstractTheory* theory, MinisatID::WrappedPCSolver* solver);

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
