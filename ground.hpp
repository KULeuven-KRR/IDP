/************************************
	ground.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUND_HPP
#define GROUND_HPP

#include "theory.hpp"
#include "checker.hpp"

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

enum TsType { TS_EQ, TS_RULE, TS_IMPL, TS_RIMPL };

struct TsBody {
	vector<int> _body;
	TsType		_type;
	bool		_conj;
};

class GroundTranslator {

	public:
		virtual int			translate(PFSymbol*,const vector<TypedElement>&) = 0;		// translate an atom to an integer
				int			translate(PFSymbol*,const vector<TypedElement*>&);			// translate an atom to an integer
		virtual int			translate(const vector<int>& cl, bool conj, TsType tp) = 0;	// translate a clause to an integer
		virtual int			nextNumber() = 0;
		virtual bool		isTseitin(int l) const = 0;
		virtual PFSymbol*	symbol(int n)	const = 0; 
		virtual const vector<TypedElement>&	args(int n)	const = 0; 
		virtual const TsBody&	tsbody(int l)	const = 0;
};

class NaiveTranslator : public GroundTranslator {

	private:

		map<PFSymbol*,map<vector<TypedElement>,int> >	_table;			// maps atoms to integers
		vector<PFSymbol*>								_backsymbtable;	// map integer to the symbol of its corresponding atom
		vector<vector<TypedElement> >					_backargstable;	// map integer to the terms of its corresponding atom
		set<int>										_freenumbers;	// keeps free numbers
		map<int,TsBody>									_tsbodies;

	public:
		
		NaiveTranslator() : _backsymbtable(1), _backargstable(1) { }

		int							translate(PFSymbol*,const vector<TypedElement>&);
		int							translate(const vector<int>& cl, bool conj, TsType tp);
		int							nextNumber();
		PFSymbol*					symbol(int n)	const	{ return _backsymbtable[abs(n)];	}
		const vector<TypedElement>&	args(int n)		const	{ return _backargstable[abs(n)];	}
		bool						isTseitin(int l) const	{ return symbol(l) == 0;			}
		const TsBody&				tsbody(int l)	const	{ return _tsbodies.find(abs(l))->second;			}

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

class TermGrounder {
	
	protected:
		TypedElement*			_result;

	public:
		TermGrounder(TypedElement* r) :
			_result(r) { }

		virtual void run() const = 0;
};

class VarTermGrounder : public TermGrounder {
	
	private:
		SortTable*		_table;
		TypedElement*	_arg;

	public:
		VarTermGrounder(TypedElement* r, SortTable* t, TypedElement* a) :
			TermGrounder(r), _table(t), _arg(a) { }

		void run() const;

};

class FuncTermGrounder : public TermGrounder {

	private:
		FuncTable*				_function;
		vector<TypedElement*>	_args;
		vector<TermGrounder*>	_subtermgrounders;

	public:
		FuncTermGrounder(TypedElement* r, const vector<TermGrounder*>& sub, FuncTable* f, const vector<TypedElement*>& a) :
			TermGrounder(r), _function(f), _args(a), _subtermgrounders(sub) { }

		void run() const;


		// TODO? Optimisation:
		//			Keep all values of the args + result of the previous call to calc().
		//			If the values of the args did not change, return the result immediately instead of doing the
		//			table lookup
};

class Grounder {

	protected:
		EcnfTheory*	_grounding;

		static int	_true;
		static int	_false;

	public:

		// Constructor
		Grounder(EcnfTheory* g): _grounding(g) { 	}

		virtual int			run()		const = 0;
				EcnfTheory*	grounding()	const { return _grounding;	}

};

class EcnfGrounder : public Grounder {
	
	public:
		
		// Constructor
		EcnfGrounder(EcnfTheory* g): Grounder(g) { }

		int run() const { return _true; }

};

class TheoryGrounder : public Grounder {

	private:
		vector<Grounder*>	_children;

	public:
		
		// Constructor
		TheoryGrounder(EcnfTheory* g, const vector<Grounder*>& vg): Grounder(g), _children(vg) { }

		int run() const;

};

class AtomGrounder : public Grounder {

	private:
		bool					_sign;
		bool					_sentence;
		PFSymbol*				_symbol;
		vector<TypedElement*>	_args;
		vector<TermGrounder*>	_subtermgrounders;
		vector<SortTable*>		_tables;

		InstanceChecker*		_pchecker;
		InstanceChecker*		_cchecker;
		int						_certainvalue;
		bool					_poscontext;

	
	public:

		// Constructor
		AtomGrounder(EcnfTheory* g, bool sign, bool sent, PFSymbol* s, const vector<TypedElement*>& args, 
			const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic, const vector<SortTable*>& vst, bool pc, bool c) :
				Grounder(g), _sign(sign), _sentence(sent), _symbol(s), _args(args), _subtermgrounders(sg), 
				_tables(vst), _poscontext(pc), _pchecker(pic), _cchecker(cic)
				{ _certainvalue = c ? _true : _false; }

		int run() const;
};

class BoolGrounder : public Grounder {

	private:
		vector<Grounder*>	_subgrounders;
		bool				_sign;
		bool				_sentence;
		bool				_conj;
		bool				_poscontext;

	public:
		BoolGrounder(EcnfTheory* g, const vector<Grounder*> sub, bool sign, bool sen, bool conj, bool pos):
			Grounder(g), _subgrounders(sub), _sign(sign), _sentence(sen), _conj(conj), _poscontext(pos) { }

		int		run() const;
		bool	check1(int l) const;
		bool	check2(int l) const;
		int		result1() const;
		int		result2() const;

};

class GrounderFactory : public Visitor {
	
	private:
		AbstractStructure*	_structure;
		EcnfTheory*			_grounding;

		// Context
		bool	_poscontext;
		bool	_truegencontext;
		bool	_sentence;

		// Variable mapping
		map<Variable*,TypedElement*>	_varmapping;

		// Return values
		Grounder*			_grounder;
		TermGrounder*		_termgrounder;
		TypedElement*		_value;

	public:

		// Constructor
		GrounderFactory(AbstractStructure* structure): _structure(structure) { }

		// Factory method
		Grounder* create(AbstractTheory* theory) { theory->accept(this); return _grounder; }

		// Visitors
		void visit(EcnfTheory*);
		void visit(Theory*);

		void visit(PredForm*);
		void visit(BoolForm*);

		void visit(VarTerm*);
		void visit(DomainTerm*);
		void visit(FuncTerm*);
		void visit(AggTerm*);

};

#endif
