/************************************
	print.h	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINT_H
#define PRINT_H

#include "files.h"
#include "visitor.h"
#include "theory.h"
#include <cstdio>

extern Files files;

//class Printer : public Visitor {
//
//	protected:
//		FILE* _out;
//	
//	public:
//		Printer() : Visitor() {
//			_out = fopen(options._outputfile,'w');
//		}
//
//		~Printer() {
//			fclose(_out);
//		}
//
//		void print(Vocabulary* v) 	{ v->accept(this) }
//		void print(Theory* t) 		{ t->accept(this) }
//		void print(Structure* s) 	{ s->accept(this) }
//
//};
//
//class IDPPrinter : public Printer { ... }

class IDPPrinter : public Visitor {

	protected:
		FILE* _out;

	public:
		// Constructors
		IDPPrinter()			: Visitor() {
			_out = files._outputfile;
		}

		IDPPrinter(Theory* t)	: Visitor() { 
			_out = files._outputfile;
			t->accept(this);
		}

		// Print methods
		void print(Theory* t) { t->accept(this); }

		// Theory
		void visit(Theory*);

		// Formulas
		void visit(PredForm*);
		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(BoolForm*);
		void visit(QuantForm*);

		// Definitions
		void visit(Rule*);
		void visit(Definition*);
		void visit(FixpDef*);

		// Terms
		void visit(VarTerm*);
		void visit(FuncTerm*);
		void visit(DomainTerm*);
		void visit(AggTerm*);

		// Set expressions
		void visit(EnumSetExpr*);
		void visit(QuantSetExpr*);

};

#endif
