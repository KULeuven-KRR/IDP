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
		virtual int translate(PFSymbol*,const vector<string>&) = 0;
};

class NaiveTranslator : public GroundTranslator {

	private:
		int										_nextnumber;
		map<PFSymbol*,map<vector<string>,int> >	_table;

	public:
		
		NaiveTranslator() : _nextnumber(1) { }
		int translate(PFSymbol*,const vector<string>&);
};


/********************************************************
	Basic top-down, non-optimized grounding algorithm
********************************************************/

class NaiveGrounder : public Visitor {

	private:
		Structure*					_structure;		// The structure to ground
		map<Variable*,TypedElement>	_varmapping;	// The current assignment of domain elements to variables

		Formula*					_returnFormula;	// The return value when grounding a formula
		SetExpr*					_returnSet;		// The return value when grounding a set
		Term*						_returnTerm;	// The return value when grounding a term
		Definition*					_returnDef;		// The return value when grounding a definition
		FixpDef*					_returnFixpDef;	// The return value when grounding a fixpoint definition
		Theory*						_returnTheory;	// The return value when grounding a theory

	public:
		NaiveGrounder(Structure* s) : Visitor(), _structure(s) { }

		Theory* ground(Theory* t) { t->accept(this); return _returnTheory;	}

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
