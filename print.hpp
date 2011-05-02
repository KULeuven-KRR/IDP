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
#include <vector>
#include <set>

#include "visitor.hpp"
#include "commontypes.hpp"

class PFSymbol;
class AbstractStructure;
class PredTable;
class GroundTranslator;
class InfOptions;
class GroundTermTranslator;

/*************************
	Printer base class
*************************/

class Printer : public Visitor {
	protected:
		std::stringstream _out;
		unsigned int _indent;
		Printer(); 
	
	public:
		// Factory method
		static Printer* create(InfOptions* opts);

		// Print methods
		std::string print(const Vocabulary*);
		std::string print(const AbstractTheory*);
		std::string print(const AbstractStructure*);
		std::string print(const Namespace*);

		// Indentation
		void indent();
		void unindent();
		void printtab();
};

/*********************
	Simple printer
*********************/

class SimplePrinter : public Printer {
	public:
		void visit(const Vocabulary*);
		void visit(const Theory*);
		void visit(const GroundTheory*);
		void visit(const Structure*);
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
		void printAggregate(double bound, bool lower, AggType aggtype, unsigned int setnr);

	public:
		IDPPrinter() : _printtypes(false) { }
		IDPPrinter(bool printtypes) : _printtypes(printtypes) { }

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
		void visit(const Vocabulary*);
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

		// Constraint Programming
		void visit(const CPReification*);
		void visit(const CPSumTerm*);
		void visit(const CPWSumTerm*);
		void visit(const CPVarTerm*);
};

class EcnfPrinter : public Printer {
	private:
		int							_currenthead;
		unsigned int 				_currentdefnr;
		AbstractStructure*			_structure;
		const GroundTermTranslator*	_termtranslator;
		std::set<unsigned int> 		_printedvarids;

		void printAggregate(AggType aggtype, TsType arrow, unsigned int defnr, bool lower, int head, unsigned int setnr, double bound);
		void printCPVariable(unsigned int varid);
		void printCPVariables(std::vector<unsigned int> varids);
		void printCPReification(std::string type, int head, unsigned int left, CompType comp, int right);
		void printCPReification(std::string type, int head, std::vector<unsigned int> left, CompType comp, int right);
		void printCPReification(std::string type, int head, std::vector<unsigned int> left, std::vector<int> weights, CompType comp, int right);

	public:
		void visit(const GroundTheory*);
		void visit(const GroundDefinition*);
		void visit(const PCGroundRuleBody*);
		void visit(const AggGroundRuleBody*);
		void visit(const GroundAggregate*);
		void visit(const GroundSet*);
		void visit(const CPReification*);
}; 

#endif
