/************************************
	visitor.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VISITOR_H
#define VISITOR_H

class Formula;
class Term;
class Rule;
class Definition;
class FixpDef;
class SetExpr;
class Theory;
class PredForm;
class EqChainForm;
class EquivForm;	
class BoolForm;
class QuantForm;
class AggForm;
class VarTerm;
class AggTerm;
class DomainTerm;
class FuncTerm;
class QuantSetExpr;
class EnumSetExpr;
class SortTable;
class PredInter;
class FuncInter;
class Structure;
class EcnfTheory;

/*
 *	class Visitor
 *		This class implements a visitor pattern for namespaces, theories, etc.
 *		By default, if 'visit' is called on an object A, then A is traversed depth-first, from left to right.
 */
class Visitor {
	public:
		Visitor() { }
		virtual ~Visitor() { }

		// Traverse the parse tree
		void traverse(Formula* f);
		void traverse(Term* t);
		void traverse(Rule* r);
		void traverse(Definition* d);
		void traverse(FixpDef* f);
		void traverse(SetExpr* s);
		void traverse(Theory* t);
		void traverse(Structure* s);

		/** Theories **/

		// Formulas 
		virtual void visit(PredForm* a);			
		virtual void visit(EqChainForm* a);
		virtual void visit(EquivForm* a);
		virtual void visit(BoolForm* a);
		virtual void visit(QuantForm* a);
		virtual void visit(AggForm* a);

		// Definitions 
		virtual void visit(Rule* a);
		virtual void visit(Definition* a);
		virtual void visit(FixpDef* a);

		// Terms
		virtual void visit(VarTerm* a);
		virtual void visit(FuncTerm* a);
		virtual void visit(DomainTerm* a);
		virtual void visit(AggTerm* a);

		// Set expressions
		virtual void visit(EnumSetExpr* a);
		virtual void visit(QuantSetExpr* a);

		// Theories
		virtual void visit(Theory* t);
		virtual void visit(EcnfTheory* t);

		/** Structures **/

		virtual void visit(SortTable*);
		virtual void visit(PredInter*);
		virtual void visit(FuncInter*);
		virtual void visit(Structure*);


};

/*
 *	class MutatingVisitor
 *		This class implements a visitor pattern for namespaces, theories, etc. that changes the object it visits.
 *		By default, if 'visit' is called on an object A, then A is traversed depth-first, from left to right.
 *		During the traversal, a call of 'visit' to a direct child B of A gives a returnvalue B'. 
 *		If B' is equal to B, nothing happens. Else, B' becomes a child of A instead of B, and B is deleted non-recursively. 
 */
class MutatingVisitor {
	
	public:

		MutatingVisitor() { }
		virtual ~MutatingVisitor() { }

		/** Theories **/ 

		// Formulas 
		virtual Formula* visit(PredForm* a);	
		virtual Formula* visit(EqChainForm* a);	
		virtual Formula* visit(EquivForm* a);	
		virtual Formula* visit(BoolForm* a);	
		virtual Formula* visit(QuantForm* a);	
		virtual Formula* visit(AggForm* a);

		// Definitions 
		virtual Rule* visit(Rule* a);		// NOTE: the head of a rule is not visited by the default implementation!
		virtual Definition* visit(Definition* a);	
		virtual FixpDef* visit(FixpDef* a);		

		// Terms
		virtual Term* visit(VarTerm* a);		
		virtual Term* visit(FuncTerm* a);	
		virtual Term* visit(DomainTerm* a);	
		virtual Term* visit(AggTerm* a);		

		// Set expressions
		virtual SetExpr* visit(EnumSetExpr* a);	
		virtual SetExpr* visit(QuantSetExpr* a);

		// Theories
		virtual Theory* visit(Theory* t);	
		virtual EcnfTheory* visit(EcnfTheory* t);


};


#endif
