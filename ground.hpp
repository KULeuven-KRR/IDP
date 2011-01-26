/************************************
	ground.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUND_HPP
#define GROUND_HPP

#include "theory.hpp"

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

class GroundTranslator {

	public:
		virtual int translate(PFSymbol*,const vector<TypedElement>&) = 0;	// translate an atom to an integer
		virtual int nextTseitin() = 0;								// return a new tseitin atom
		virtual PFSymbol*				symbol(unsigned int n)	const = 0; 
		virtual const vector<TypedElement>&	args(unsigned int n)	const = 0; 
};

class NaiveTranslator : public GroundTranslator {

	private:
		int	_nextnumber;	// lowest number that currently has no corresponding atom

		map<PFSymbol*,map<vector<TypedElement>,int> >	_table;			// maps atoms to integers
		vector<PFSymbol*>								_backsymbtable;	// map integer to the symbol of its corresponding atom
		vector<vector<TypedElement> >					_backargstable;	// map integer to the terms of its corresponding atom

	public:
		
		NaiveTranslator() : _nextnumber(1) { }

		int							translate(PFSymbol*,const vector<TypedElement>&);
		int							nextTseitin();	
		PFSymbol*					symbol(unsigned int n)	const { return _backsymbtable[n-1];	}
		const vector<TypedElement>&	args(unsigned int n)	const { return _backargstable[n-1];	}

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

#endif
