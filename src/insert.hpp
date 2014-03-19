/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef INSERT_HPP
#define INSERT_HPP

#include <set>
#include <vector>
#include <list>
#include <sstream>
#include "commontypes.hpp"
#include "parseinfo.hpp"
#include "vocabulary/vocabulary.hpp"
#include <ostream>
#include "vocabulary/VarCompare.hpp"

class Sort;
class Predicate;
class Function;
class Vocabulary;
class DomainElement;
class Compound;
class SortTable;
class AbstractTable;
class PredTable;
class FuncTable;
class Structure;
class Structure;
class Term;
class FuncTerm;
class AggTerm;
class DomainTerm;
class SetExpr;
class Query;
class EnumSetExpr;
class QuantSetExpr;
class Formula;
class FOBDD;
class FOBDDKernel;
class FOBDDAtomKernel;
class FOBDDQuantKernel;
class FOBDDManager;

class Rule;
class Definition;
class FixpDef;
class Theory;
class AbstractTheory;
class Options;
class UserProcedure;
class Namespace;
struct LTCVocInfo;

struct YYLTYPE;
struct lua_State;
// TODO break lua connection
struct InternalArgument;

typedef std::vector<std::string> longname;

/**
 * Pair of name and sorts
 */
struct NSPair {
	longname _name; //!< the name
	std::vector<Sort*> _sorts; //!< the sorts

	bool _sortsincluded; //!< true iff the sorts are initialized
	bool _func; //!< true if the name is a pointer to a function
	ParseInfo _pi; //!< place where the pair was parsed

	NSPair(const longname& n, const std::vector<Sort*>& s, const ParseInfo& pi)
			: _name(n), _sorts(s), _sortsincluded(true), _func(false), _pi(pi) {
	}
	NSPair(const longname& n, const ParseInfo& pi)
			: _name(n), _sorts(0), _sortsincluded(false), _func(false), _pi(pi) {
	}

	std::ostream& put(std::ostream& output) const;
};

// TODO no idea of meaning
enum ElRangeEnum {
	ERE_EL, ERE_INT, ERE_CHAR
};

/**
 * Union of a domain element or range
 */
struct ElRange {
	ElRangeEnum _type;
	union {
		const DomainElement* _element;
		std::pair<int, int>* _intrange;
		std::pair<char, char>* _charrange;
	} _value;
	ElRange()
			: _type(ERE_EL) {
		_value._element = 0;
	}
	ElRange(const DomainElement* e)
			: _type(ERE_EL) {
		_value._element = e;
	}
	ElRange(std::pair<int, int>* r)
			: _type(ERE_INT) {
		_value._intrange = r;
	}
	ElRange(std::pair<char, char>* r)
			: _type(ERE_CHAR) {
		_value._charrange = r;
	}
};

struct VarName {
	std::string _name;
	Variable* _var;
	VarName(const std::string& n, Variable* v)
			: _name(n), _var(v) {
	}

	std::ostream& put(std::ostream& os) const;
};

/**
 * The Insert class performs the actual construction of IDP-objects such as vocabularies, functions, structures, domain elements etc.
 * Methods from this class are called from the parser (parser.yy).
 */

class Insert {
private:
	lua_State* _state; //!< the lua state objects are added to

	std::string* _currfile; //!< the file that is currently being parsed
	Namespace* _currspace; //!< the namespace that is currently being parsed

	Vocabulary* _currvocabulary; //!< the vocabulary that is currently being parsed
	Theory* _currtheory; //!< the theory that is currently being parsed
	Structure* _currstructure; //!< the structure that is currently being parsed
	UserProcedure* _currprocedure; //!< the procedure that is currently being parsed
	std::shared_ptr<FOBDDManager> _currmanager; //!<the fobddmanager for the fobdd that is currently being parsed

	std::string _currquery; //!< the name of the named query that is currently being parsed
	std::string _currfobdd; //!< the name of the named fobdd that is currently being parsed

	std::string _currterm; //!< the name of the named term that is currently being parsed

	Sort* parsingType; // Type currently being parsed

	std::list<VarName> _curr_vars;

	std::vector<Vocabulary*> _usingvocab; //!< the vocabularies currently used to parse
	std::vector<Namespace*> _usingspace; //!< the namespaces currently used to parse

	std::vector<unsigned int> _nrvocabs; //!< the number of 'using vocabulary' statements in the current block
	std::vector<unsigned int> _nrspaces; //!< the number of 'using namespace' statements in the current block

	ParseInfo parseinfo(YYLTYPE l) const; //!< Convert a bison parse location to a parseinfo object
	FormulaParseInfo formparseinfo(Formula*, YYLTYPE) const;
	TermParseInfo termparseinfo(Term*, const ParseInfo&) const;
	TermParseInfo termparseinfo(Term*, YYLTYPE) const;
	SetParseInfo setparseinfo(SetExpr*, YYLTYPE) const;

	Variable* getVar(const std::string&) const; //!< Returns the quantified variable with
												//!< given name in the current scope
	varset freevars(const ParseInfo&, bool critical = false); //!< Return all currently free variables; if critical: throw an error for free variables
	void remove_vars(const std::vector<Variable*>&); //!< Remove the given variables from the
													 //!< list of free variables
	void remove_vars(const varset&); //!< Remove the given variables from the
												  //!< list of free variables

	void usenamespace(Namespace*); //!< add a using namespace statement
	void usevocabulary(Vocabulary*); //!< add a using vocabulary statement

	void openblock(); //!< open a new block
	void closeblock(); //!< close the current block

	Sort* sortInScope(const std::string&, const ParseInfo&) const;
	Sort* sortInScope(const longname&, const ParseInfo&) const;
	Predicate* predInScope(const std::string&, int arity) const;
	Predicate* predInScope(const longname&, int arity, const ParseInfo&) const;
	Function* funcInScope(const std::string&, int arity) const;
	Function* funcInScope(const longname&, int arity, const ParseInfo&) const;
	std::set<Predicate*> noArPredInScope(const std::string& name) const;
	std::set<Predicate*> noArPredInScope(const longname& name, const ParseInfo&) const;
	std::set<Function*> noArFuncInScope(const std::string& name) const;
	std::set<Function*> noArFuncInScope(const longname& name, const ParseInfo&) const;
	Vocabulary* vocabularyInScope(const std::string&, const ParseInfo&) const;
	Vocabulary* vocabularyInScope(const longname&, const ParseInfo&) const;
	Namespace* namespaceInScope(const std::string&, const ParseInfo&) const;
	Namespace* namespaceInScope(const longname&, const ParseInfo&) const;
	Query* queryInScope(const std::string&, const ParseInfo&) const;
	const FOBDD* fobddInScope(const std::string&, const ParseInfo&) const;
	Term* termInScope(const std::string&, const ParseInfo&) const;
	AbstractTheory* theoryInScope(const std::string&, const ParseInfo&) const;
	AbstractTheory* theoryInScope(const longname&, const ParseInfo&) const;
	Structure* structureInScope(const std::string&, const ParseInfo&) const;
	Structure* structureInScope(const longname&, const ParseInfo&) const;
	UserProcedure* procedureInScope(const std::string&, const ParseInfo&) const;
	UserProcedure* procedureInScope(const longname&, const ParseInfo&) const;

	bool belongsToVoc(PFSymbol*) const;
	bool belongsToVoc(Sort*) const;

	Formula* boolform(bool, Formula*, Formula*, YYLTYPE) const;
	Formula* quantform(bool, const varset&, Formula*, YYLTYPE);

	void assignunknowntables();

	std::set<Predicate*> parsedpreds;
	std::set<Function*> parsedfuncs;

public:
	Insert(Namespace* ns);

	std::string* currfile() const; //!< return the current filename
	void currfile(const std::string& s); //!< set the current filename
	void currfile(std::string* s); //!< set the current filename
	varset varVectorToSet(std::vector<Variable*>* v);

	void openNamespace(const std::string& name, YYLTYPE); //!< Open a new namespace
	void openvocab(const std::string& name, YYLTYPE); //!< Open a new vocabulary
	void opentheory(const std::string& tname, YYLTYPE); //!< Open a new theory
	void openquery(const std::string& tname, YYLTYPE); //!< Open a new named query
	void openfobdd(const std::string& tname, YYLTYPE); //!< Open a new named fobdd
	void openterm(const std::string& tname, YYLTYPE); //!< Open a new named term
	void openstructure(const std::string& name, YYLTYPE); //!< Open a new structure
	void openprocedure(const std::string& name, YYLTYPE); //!< Open a procedure
	void openexec(); //!< Start parsing a command
	void closeNamespace(); //!< Close the current namespace
	void closevocab(); //!< Close the current vocabulary
	void closeLTCvocab(); //!< Close the current LTC-vocabulary: close it and finish it by performing all needed LTC transformations
	void closeLTCvocab(NSPair* time, NSPair* start, NSPair* next); //!< Close the current LTC-vocabulary: close it and finish it by performing all needed LTC transformations using the provided ltc info
	void closetheory(); //!< Close the current theory
	void closequery(Query*); //!< Close the current named query
	void closefobdd(const FOBDD*); //!< Close the current named query
	void closeterm(Term*); //!< Close the current named term
	void closestructure(bool assumeClosedWorld = false); //!< Close the current structure
	void closeprocedure(std::stringstream*); //!< Close the current procedure

	void usingvocab(const longname& vname, YYLTYPE); //!< use vocabulary 'vname' when parsing
	void usingspace(const longname& sname, YYLTYPE); //!< use namespace 'sname' when parsing

	void setvocab(const longname& vname, YYLTYPE); //!< set the vocabulary of the current theory or structure
	void externvocab(const longname& vname, YYLTYPE) const; //!< add all symbols of 'vname' to the current vocabulary

	Sort* sort(Sort* s) const; //!< add an existing sort to the current vocabulary
	Predicate* predicate(Predicate* p) const; //!< add an existing predicate to the current vocabulary
	Function* function(Function* f) const; //!< add an existing function to the current vocabulary

	Sort* sortpointer(const longname&, YYLTYPE) const;
	//!< return the sort with the given name
	Predicate* predpointer(longname&, const std::vector<Sort*>&, YYLTYPE) const;
	//!< return the predicate with the given name and sorts
	Predicate* predpointer(longname&, int arity, YYLTYPE) const;
	//!< return the predicate with the given name and arity
	Function* funcpointer(longname&, const std::vector<Sort*>&, YYLTYPE) const;
	//!< return the function with the given name and sorts
	Function* funcpointer(longname&, int arity, YYLTYPE) const;
	//!< return the function with the given name and arity

	NSPair* internpredpointer(const longname&, const std::vector<Sort*>&, YYLTYPE) const;
	NSPair* internfuncpointer(const longname&, const std::vector<Sort*>&, Sort*, YYLTYPE) const;
	NSPair* internpointer(const longname& name, YYLTYPE) const;

	Sort* sort(const std::string& name, YYLTYPE, SortTable* fixedInter = NULL);
	Sort* sort(const std::string& name, const std::vector<Sort*> supbs, bool p, YYLTYPE, SortTable* fixedInter = NULL);
	Sort* sort(const std::string& name, const std::vector<Sort*> sups, const std::vector<Sort*> subs, YYLTYPE l, SortTable* fixedInter = NULL);
	void addConstructors(const std::vector<Function*>* functionlist) const;

	Predicate* predicate(const std::string& name, const std::vector<Sort*>& sorts, YYLTYPE) const;
	//!< create a new predicate with the given sorts in the current vocabulary
	Predicate* predicate(const std::string& name, YYLTYPE) const;
	//!< create a new 0-ary predicate in the current vocabulary

	// Create new functions in the current voc
private:
	Function* createfunction(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort, bool isConstructor, YYLTYPE) const;
public:
	Function* function(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort, YYLTYPE) const;
	Function* constructorfunction(const std::string& name, const std::vector<Sort*>& insorts, YYLTYPE) const;

	void partial(Function* f) const;
	//!< make a function partial

	void definition(Definition* d) const; //!< add a definition to the current theory
	void sentence(Formula* f); //!< add a sentence to the current theory
	void fixpdef(FixpDef* d) const; //!< add a fixpoint defintion to the current theory

	Definition* definition(const std::vector<Rule*>& r) const; //!< create a new definition

	Rule* rule(const varset&, Formula* h, Formula* b, YYLTYPE);
	//!< create a new rule
	Rule* rule(const varset&, Formula* h, YYLTYPE);
	//!< create a new rule with an empty body

	void addRule(FixpDef*, Rule*) const; //!< add a rule to a fixpoint definition
	void addDef(FixpDef*, FixpDef*) const; //!< add a fixpoint definition to a fixpoint definition
	FixpDef* createFD() const; //!< create a new fixpoint definition
	void makeLFD(FixpDef*, bool) const; //!< make the fixpointdefinition a least or greatest fixpoint definition

	Formula* equalityhead(Term* left, Term* right, YYLTYPE) const;

	Formula* trueform(YYLTYPE) const;
	//!< create a new true formula
	Formula* falseform(YYLTYPE) const;
	//!< create a new false formula
	Formula* funcgraphform(NSPair*, const std::vector<Term*>&, Term*, YYLTYPE) const;
	//!< create a new formula of the form F(t1,...,tn) = t
	Formula* funcgraphform(NSPair*, Term*, YYLTYPE) const;
	//!< create a new formula of the form C = t
	Formula* predformVar(NSPair*, const std::vector<Variable*>&, YYLTYPE) const;
	//!< create a new formula of the form P(t1,...,tn) where ti is a var
	Formula* predform(NSPair*, const std::vector<Term*>&, YYLTYPE) const;
	//!< create a new formula of the form P(t1,...,tn)
	Formula* predform(NSPair*, YYLTYPE) const;
	//!< create a new formula of the form P
	Formula* equivform(Formula*, Formula*, YYLTYPE) const;
	//!< create a new formula of the form (phi1 <=> phi2)
	Formula* disjform(Formula*, Formula*, YYLTYPE) const;
	//!< create a new formula of the form (phi1 | phi2)
	Formula* conjform(Formula*, Formula*, YYLTYPE) const;
	//!< create a new formula of the form (phi1 & phi2)
	Formula* implform(Formula*, Formula*, YYLTYPE) const;
	//!< create a new formula of the form (phi1 => phi2)
	Formula* revimplform(Formula*, Formula*, YYLTYPE) const;
	//!< create a new formula of the form (phi1 <= phi2)
	Formula* univform(const varset&, Formula*, YYLTYPE l);
	//!< create a new formula of the form (! x1 ... xn : phi)
	Formula* existform(const varset&, Formula*, YYLTYPE l);
	//!< create a new formula of the form (? x1 ... xn : phi)
	Formula* bexform(CompType, int, const varset&, Formula*, YYLTYPE);
	//!< create a new formula of the form (?_op_n x1 ... xn : phi)
	Formula* eqchain(CompType, Formula*, Term*, YYLTYPE) const;
	//!< add a term to an equation chain
	Formula* eqchain(CompType, Term*, Term*, YYLTYPE) const;
	//!< create a new equation chain
	void negate(Formula*) const;
	//!< negate a formula

	Variable* quantifiedvar(const std::string& name, YYLTYPE l);
	//!< create a new quantified variable
	Variable* quantifiedvar(const std::string& name, Sort* sort, YYLTYPE l);
	//!< create a new quantified variable with a given sort
	Sort* theosortpointer(const longname& vs, YYLTYPE l) const;
	//!< get a sort with a given name in the current vocabulary

	FuncTerm* functerm(NSPair*, const std::vector<Term*>&); //!< create a new function term
	Term* term(NSPair*); //
	FuncTerm* arterm(char, Term*, Term*, YYLTYPE) const; //!< create a new binary arithmetic term
	FuncTerm* arterm(const std::string&, Term*, YYLTYPE) const; //!< create a new unary arithmetic term
	DomainTerm* domterm(int, YYLTYPE) const; //!< create a new domain element term
	DomainTerm* domterm(double, YYLTYPE) const; //!< create a new domain element term
	DomainTerm* domterm(std::string*, YYLTYPE) const; //!< create a new domain element term
	DomainTerm* domterm(char, YYLTYPE) const; //!< create a new domain element term
	DomainTerm* domterm(std::string*, Sort*, YYLTYPE) const; //!< create a new domain element term of a given sort
	AggTerm* aggregate(AggFunction, EnumSetExpr*, YYLTYPE) const; //!< create a new aggregate term


	Query* query(const std::vector<Variable*>&, Formula*, YYLTYPE);

	const FOBDD* fobdd(const FOBDDKernel*, const FOBDD*, const FOBDD*) const;
	const FOBDDKernel* atomkernel(Formula*) const;
	const FOBDDKernel* quantkernel(Variable* var, const FOBDD* bdd) const;
	const FOBDD* truefobdd() const;
	const FOBDD* falsefobdd() const;

	EnumSetExpr* set(Formula*, YYLTYPE,const varset& vv);
	//!< Create a new set of the form { x1 ... xn : phi }
	EnumSetExpr* set(Formula*, Term*, YYLTYPE,const varset& vv);
	//!< Create a new set of the form { x1 ... xn : phi : t }

	EnumSetExpr* trueset(Term* t, YYLTYPE l);
	//!< Create a new set of the form {:true:t}

	//take the union of 2 EnumSets by adding everything in s2 to s1
	void addToFirst(EnumSetExpr* s1, EnumSetExpr* s2);

	EnumSetExpr* createEnum(YYLTYPE) const;
	//!< Create a new EnumSetExpr
	EnumSetExpr* addFormula(EnumSetExpr*, Formula*) const;
	//!< Add a tuple (phi,1) to an EnumSetExpr
	EnumSetExpr* addFT(EnumSetExpr*, Formula*, Term*) const;
	//!< Add a tuple (phi,t) to an EnumSetExpr

	// Interpretations
private:
	enum class UTF {
		TWOVAL,
		U,
		CT,
		CF
	};
	std::string printUTF(UTF utf)const;
	mutable std::map<Structure*, std::set<Sort*>> sortsOccurringInUserDefinedStructure;
	std::set<Structure*, std::set<PFSymbol*> > symbolsOccurringInUserDefinedStructure;
	mutable std::map<PFSymbol*, std::map<UTF, PredTable*> > _pendingAssignments;
	void finalizePendingAssignments();
	bool basicSymbolCheck(PFSymbol* symbol, NSPair* nst)const;
	bool basicSymbolCheck(PFSymbol* symbol, NSPair* nst, UTF utf)const;
	bool isValidTruthType(const std::string& utf)const;
	UTF getTruthType(const std::string& utf)const;
	PFSymbol* retrieveSymbolNoChecks(NSPair* nst, bool expectsFunc, int arity) const;
	template<class Table>
	void setInter(NSPair* nst, bool expectsFunc, UTF truthvalue, Table* t, int arity) const;
	PFSymbol* findUniqueMatch(NSPair* nst)const;
	//Finishes an LTC vocabulary: adds derived vocabularies to the current space.
	void finishLTCVocab(Vocabulary* voc, const LTCVocInfo* ltcVocInfo);

public:
	/**
	 * Returns true if this sort occurred in the user provided theory.
	 */
	bool interpretationSpecifiedByUser(Structure* structure, Sort* sort) const;
	void constructor(NSPair* nst) const; //!< allows for the declaration of constructor functions in structure. TODO: test + evaluate usefulness
	void sortinter(NSPair*, SortTable* t)const; //!< Assign a one dimensional table
	void interByProcedure(NSPair*, const longname&, YYLTYPE) const; //!< Assign a procedure
	void predinter(NSPair*, PredTable* t, const std::string& utf = "tv")const;
	void predinter(NSPair*, SortTable* t, const std::string& utf = "tv")const;
	void truepredinter(NSPair*, const std::string& utf = "tv") const;
	void falsepredinter(NSPair*, const std::string& utf = "tv") const;
	void funcinter(NSPair*, PredTable* t, const std::string& utf = "tv")const;
	void funcinter(NSPair*, FuncTable* t, const std::string& utf = "tv")const;
	void emptyinter(NSPair*, const std::string& utf = "tv")const; //!<Inserts an empty interpretation for three-valued symbols (e.g. P<ct> = {})

	SortTable* createSortTable() const;
	void addElement(SortTable*, int) const;
	void addElement(SortTable*, double) const;
	void addElement(SortTable*, const std::string&) const;
	void addElement(SortTable*, const Compound*) const;
	void addElement(SortTable*, int, int) const;
	void addElement(SortTable*, char, char) const;

	// TODO in this code, it is not assumed that the sorts are already filled, this is not even checked.
	// 		apparently, this happens later, which introduces big holes in the semantics:
	//		addtuple, add interpretation to structure and check sorts, add more tuples => not in sorts!
	PredTable* createPredTable(unsigned int arity) const;
	void addTuple(PredTable*, std::vector<const DomainElement*>&, YYLTYPE) const;
	void addTuple(PredTable*, YYLTYPE) const;

	FuncTable* createFuncTable(unsigned int arity) const;
	void addTupleVal(FuncTable*, std::vector<const DomainElement*>&, YYLTYPE) const;
	void addTupleVal(FuncTable*, const DomainElement*, YYLTYPE) const;

	const DomainElement* element(int) const;
	const DomainElement* element(double) const;
	const DomainElement* element(char) const;
	const DomainElement* element(const std::string&) const;
	const DomainElement* element(const Compound*) const;

	std::pair<int, int>* range(int, int, YYLTYPE) const;
	std::pair<char, char>* range(char, char, YYLTYPE) const;

	const Compound* compound(NSPair*) const;
	const Compound* compound(NSPair*, const std::vector<const DomainElement*>&) const;

	void predatom(NSPair*, const std::vector<ElRange>&, bool);
	void predatom(NSPair*, bool);

	std::vector<ElRange>* domaintuple(const DomainElement*) const;
	std::vector<ElRange>* domaintuple(std::pair<int, int>*) const;
	std::vector<ElRange>* domaintuple(std::pair<char, char>*) const;
	std::vector<ElRange>* domaintuple(std::vector<ElRange>*, const DomainElement*) const;
	std::vector<ElRange>* domaintuple(std::vector<ElRange>*, std::pair<int, int>*) const;
	std::vector<ElRange>* domaintuple(std::vector<ElRange>*, std::pair<char, char>*) const;

	void procarg(const std::string&) const; //!< Add an argument to the current procedure

	static const DomainElement* exec(const std::string&);
};

#endif
