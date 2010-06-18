/************************************
	print.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINT_HPP
#define PRINT_HPP

#include "visitor.hpp"
#include "theory.hpp"
#include "options.hpp"
#include <cstdio>

extern Options options;

class Printer : public Visitor {

	protected:
		FILE* _out;
	
	public:
		Printer() : Visitor() {
			if(options._outputfile.empty())
				_out = stdout;
			else
				_out = fopen(options._outputfile.c_str(),"w");
		}

		~Printer() {
			fclose(_out);
		}

//		void print(Vocabulary* v) 	{ v->accept(this); }
		void print(Theory* t) 		{ t->accept(this); }
//		void print(Structure* s) 	{ s->accept(this); }

};

class IDPPrinter : public Printer {

	public:
		// Constructors
//		IDPPrinter() : Printer() {
//			_out = fopen(options._outputfile,'w');
//		}

//		IDPPrinter(Theory* t)	: Visitor() { 
//			_out = files._outputfile;
//			t->accept(this);
//		}

		// Print methods
//		void print(Theory* t) { t->accept(this); }

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
