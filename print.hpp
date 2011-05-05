/************************************
	print.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINT_HPP
#define PRINT_HPP

#include <cstdio>
#include <sstream>
#include <string>
#include "theory.hpp"

class Options;
class PredTable;
class Structure;
class Namespace;
class GroundTranslator;
class GroundTermTranslator;
class GroundTheory;
class GroundDefinition;
class PCGroundRuleBody;
class AggGroundRuleBody;
class GroundAggregate;
class GroundSet;
class CPVarTerm;
class CPWSumTerm;
class CPReification;
class CPSumTerm;


/***************************
	Printer base classes
***************************/

class Printer : public TheoryVisitor {
	protected:
		std::stringstream _out;
		unsigned int _indent;
		Printer(); 
	
	public:
		// Factory method
		static Printer* create(Options* opts);

		// Print methods
		virtual std::string print(const Vocabulary*) = 0;
		virtual std::string print(const AbstractStructure*) = 0;
		virtual std::string print(const Namespace*) = 0;
		std::string print(const AbstractTheory*);

		// Indentation
		void indent();
		void unindent();
		void printtab();
};

/******************
	IDP printer
******************/

class IDPPrinter : public Printer {
	private:
		bool 						_printtypes;
		const PFSymbol* 			_currentsymbol;
		const Structure* 			_currentstructure;
		const GroundTranslator*		_translator;
		const GroundTermTranslator*	_termtranslator;

		void print(const PredTable*);
		void printInter(const char*,const char*,const PredTable*,const PredTable*);
		void printAtom(int atomnr);
		void printTerm(unsigned int termnr);
		void printAggregate(double bound, bool lower, AggFunction aggtype, unsigned int setnr);

	public:
		IDPPrinter() : _printtypes(false) { }
		IDPPrinter(bool printtypes) : _printtypes(printtypes) { }

		// Print methods
		std::string print(const Vocabulary*);
		std::string print(const AbstractStructure*);
		std::string print(const Namespace*);

		std::ostream& print(std::ostream&, SortTable*) const;
		std::ostream& print(std::ostream&, const PredTable*) const;
		std::ostream& print(std::ostream&, FuncTable*) const;
		std::ostream& printasfunc(std::ostream&, const PredTable*) const;

		/** Namespace **/
		void visit(const Namespace*); //TODO procedures and options are not printed yet..

		/** Theories **/
		void visit(const Theory*);

		// Definitions
		void visit(const Definition*);
		void visit(const Rule*);
		void visit(const FixpDef*);

		// Formulas
		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);

		// Terms
		void visit(const VarTerm*);
		void visit(const FuncTerm*);
		void visit(const DomainTerm*);
		void visit(const AggTerm*);

		// Set expressions
		void visit(const EnumSetExpr*);
		void visit(const QuantSetExpr*);

		/** Structures **/
		void visit(const Structure*);
		void visit(const SortTable*);
		void visit(const PredInter*);
		void visit(const FuncInter*);

		/** Vocabularies **/
		void visit(const Sort*);
		void visit(const Predicate*);
		void visit(const Function*);

		/** Grounding **/
		void visit(const GroundTheory*);
		void visit(const GroundDefinition*);
		void visit(const PCGroundRuleBody*);
		void visit(const AggGroundRuleBody*);
		void visit(const GroundAggregate*);
		void visit(const GroundSet*);

		/* Constraint Programming */
		void visit(const CPReification*);
		void visit(const CPSumTerm*);
		void visit(const CPWSumTerm*);
		void visit(const CPVarTerm*);
};

class EcnfPrinter : public Printer {
	private:
		int				_currenthead;
		unsigned int 	_currentdefnr;

		void printAggregate(AggFunction aggtype, TsType arrow, unsigned int defnr, bool lower, int head, unsigned int setnr, double bound);

	public:
		std::string print(const Vocabulary*);
		std::string print(const AbstractStructure*);
		std::string print(const Namespace*);

		void visit(const GroundTheory*);
		void visit(const GroundDefinition*);
		void visit(const PCGroundRuleBody*);
		void visit(const AggGroundRuleBody*);
		void visit(const GroundAggregate*);
		void visit(const GroundSet*);
}; 

#endif
