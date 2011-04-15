/************************************
	insert.hpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSERT_HPP
#define INSERT_HPP

#include <set>
#include <vector>
#include <list>
#include <sstream>
#include "commontypes.hpp" 
#include "parseinfo.hpp"

class Sort;
class Predicate;
class Function;
class Vocabulary;
class DomainElement;
class Compound;
class SortTable;
class PredTable;
class FuncTable;
class Structure;
class Term;
class SetExpr;
class EnumSetExpr;
class Formula;
class Rule;
class Definition;
class FixpDef;
class Theory;
class AbstractTheory;
class Options;
class UserProcedure;
class Namespace;

struct YYLTYPE;
class lua_State;
class InternalArgument;

typedef std::vector<std::string> longname;

/**
 * Pair of name and sorts
 */
struct NSPair {
	longname			_name;		//!< the name
	std::vector<Sort*>	_sorts;		//!< the sorts

	bool		_arityincluded;	//!< true iff the name ends on /arity
	bool		_sortsincluded;	//!< true iff the sorts are initialized
	bool		_func;			//!< true if the name is a pointer to a function
	ParseInfo	_pi;			//!< place where the pair was parsed

	NSPair(const longname& n, const std::vector<Sort*>& s, bool ari, const ParseInfo& pi) :
		_name(n), _sorts(s), _arityincluded(ari), _sortsincluded(true), _func(false), _pi(pi) { }
	NSPair(const longname& n, bool ari, const ParseInfo& pi) :
		_name(n), _sorts(0), _arityincluded(ari), _sortsincluded(false), _func(false), _pi(pi) { }

	void includePredArity();
	void includeFuncArity();
	void includeArity(unsigned int n);

	std::string to_string();
};

enum ElRangeEnum { ERE_EL, ERE_INT, ERE_CHAR };

/**
 * Union of a domain element or range
 */
struct ElRange {
	ElRangeEnum _type;
	union {
		const DomainElement*	_element;
		std::pair<int,int>*		_intrange;
		std::pair<char,char>*	_charrange;
	} _value;
	ElRange(const DomainElement* e) : _type(ERE_EL) { _value._element = e;	}
	ElRange(std::pair<int,int>* r) : _type(ERE_INT) { _value._intrange = r;	}
	ElRange(std::pair<char,char>* r) : _type(ERE_CHAR) { _value._charrange = r;	}
};

struct VarName {
	std::string	_name;
	Variable*	_var;
	VarName(const std::string& n, Variable* v) : _name(n), _var(v) { }
};

class Insert {

	private:
		lua_State*		_state;		//!< the lua state objects are added to

		std::string*	_currfile;	//!< the file that is currently being parsed
		Namespace*		_currspace;	//!< the namespace that is currently being parsed

		Vocabulary*		_currvocabulary;	//!< the vocabulary that is currently being parsed
		Theory*			_currtheory;		//!< the theory that is currently being parsed
		Structure*		_currstructure;		//!< the structure that is currently being parsed
		Options*		_curroptions;		//!< the options that is currently being parsed
		UserProcedure*	_currprocedure;		//!< the procedure that is currently being parsed

		std::list<VarName>	curr_vars;

		std::vector<Vocabulary*>	_usingvocab;	//!< the vocabularies currently used to parse
		std::vector<Namespace*>		_usingspace;	//!< the namespaces currently used to parse

		std::vector<unsigned int>	_nrvocabs;		//!< the number of 'using vocabulary' statements in the current block
		std::vector<unsigned int>	_nrspaces;		//!< the number of 'using namespace' statements in the current block

		ParseInfo			parseinfo(YYLTYPE l);	//!< Convert a bison parse location to a parseinfo object
		FormulaParseInfo	formparseinfo(Formula*,YYLTYPE);

		std::set<Variable*>	freevars(const ParseInfo&);					//!< Return all currently free variables
		void				remove_vars(const std::set<Variable*>&);	//!< Remove the given variables from the 
																		//!< list of free variables

		void usenamespace(Namespace*);		//!< add a using namespace statement
		void usevocabulary(Vocabulary*);	//!< add a using vocabulary statement

		void openblock();	//!< open a new block
		void closeblock();	//!< close the current block


		Sort*			sortInScope(const longname&, const ParseInfo&);
		Predicate*		predInScope(const longname&, const ParseInfo&);
		Function*		funcInScope(const longname&, const ParseInfo&);
		Vocabulary*		vocabularyInScope(const std::string&, const ParseInfo&);
		Vocabulary*		vocabularyInScope(const longname&, const ParseInfo&);
		Namespace*		namespaceInScope(const longname&, const ParseInfo&);
		AbstractTheory*	theoryInScope(const std::string&, const ParseInfo&);
		AbstractTheory*	theoryInScope(const longname&, const ParseInfo&);

		bool	belongsToVoc(Predicate*);
		bool	belongsToVoc(Function*);

		Formula*	boolform(bool,Formula*,Formula*,YYLTYPE);
		Formula*	quantform(bool,const std::set<Variable*>&, Formula*, YYLTYPE);

	public:
		Insert();

		std::string*	currfile() const;				//!< return the current filename
		void			currfile(const std::string& s);	//!< set the current filename
		void			currfile(std::string* s);		//!< set the current filename

		void openspace(const std::string& name,YYLTYPE);		//!< Open a new namespace
		void openvocab(const std::string& name,YYLTYPE);		//!< Open a new vocabulary
		void opentheory(const std::string& tname, YYLTYPE);		//!< Open a new theory
		void openstructure(const std::string& name, YYLTYPE);	//!< Open a new structure
		void openprocedure(const std::string& name, YYLTYPE);	//!< Open a procedure
		void openoptions(const std::string& name, YYLTYPE);		//!< Open a new options block
		void openexec();										//!< Start parsing a command
		void closespace();										//!< Close the current namespace
		void closevocab();										//!< Close the current vocabulary
		void closetheory();										//!< Close the current theory
		void closestructure();									//!< Close the current structure
		void closeprocedure(std::stringstream*);				//!< Close the current procedure
		void closeoptions();									//!< Close the current options

		void usingvocab(const longname& vname, YYLTYPE);	//!< use vocabulary 'vname' when parsing
		void usingspace(const longname& sname, YYLTYPE);	//!< use namespace 'sname' when parsing

		void assignvocab(InternalArgument*, YYLTYPE);		//!< set the current vocabulary to the given vocabulary
		void assigntheory(InternalArgument*, YYLTYPE);		//!< set the current theory to the given theory
		void assignstructure(InternalArgument*, YYLTYPE);	//!< set the current structure to the given structure

		void setvocab(const longname& vname, YYLTYPE);		//!< set the vocabulary of the current theory or structure 
		void externvocab(const longname& vname, YYLTYPE);	//!< add all symbols of 'vname' to the current vocabulary
		
		void externoption(const std::vector<std::string>& name, YYLTYPE);

		Sort*		sort(Sort* s);				//!< add an existing sort to the current vocabulary
		Predicate*	predicate(Predicate* p);	//!< add an existing predicate to the current vocabulary
		Function*	function(Function* f);		//!< add an existing function to the current vocabulary

		Sort*		sortpointer(const longname&, YYLTYPE);	
			//!< return the sort with the given name
		Predicate*	predpointer(longname&, const std::vector<Sort*>&, YYLTYPE);
			//!< return the predicate with the given name and sorts
		Predicate*	predpointer(longname&, int arity, YYLTYPE);	
			//!< return the predicate with the given name and arity
		Function*	funcpointer(longname&, const std::vector<Sort*>&, YYLTYPE);	
			//!< return the function with the given name and sorts
		Function*	funcpointer(longname&, int arity, YYLTYPE);	
			//!< return the function with the given name and arity

		NSPair*	internpredpointer(const longname&, const std::vector<Sort*>&, YYLTYPE);
		NSPair*	internfuncpointer(const longname&, const std::vector<Sort*>&, Sort*, YYLTYPE);
		NSPair*	internpointer(const longname& name, YYLTYPE);

		Sort*	sort(const std::string& name, YYLTYPE);						
		Sort*	sort(const std::string& name, const std::vector<Sort*> supbs, bool p, YYLTYPE);
		Sort*	sort(const std::string& name, const std::vector<Sort*> sups, const std::vector<Sort*> subs, YYLTYPE);

		Predicate*	predicate(const std::string& name, const std::vector<Sort*>& sorts, YYLTYPE);	
			//!< create a new predicate with the given sorts in the current vocabulary
		Predicate*	predicate(const std::string& name, YYLTYPE);	
			//!< create a new 0-ary predicate in the current vocabulary
			
		Function*	function(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort, YYLTYPE);	
			//!< create a new function in the current vocabulary
		Function*	function(const std::string& name, Sort* outsort, YYLTYPE);	
			//!< create a new constant in the current vocabulary
		Function*	aritfunction(const std::string& name, const std::vector<Sort*>& sorts, YYLTYPE);	
			//!< create a new arithmetic function in the current vocabulary
			
		void	partial(Function* f);
			//!< make a function partial

		InternalArgument* call(const longname& proc, const std::vector<longname>& args, YYLTYPE);
			//!< call a procedure
		InternalArgument* call(const longname& proc, YYLTYPE);
			//!< call a procedure

		void definition(Definition* d);		//!< add a definition to the current theory
		void sentence(Formula* f);			//!< add a sentence to the current theory
		void fixpdef(FixpDef* d);			//!< add a fixpoint defintion to the current theory


		Definition*	definition(const std::vector<Rule*>& r); //!< create a new definition
			
		Rule*	rule(const std::set<Variable*>&,Formula* h, Formula* b, YYLTYPE);
			//!< create a new rule 
		Rule*	rule(const std::set<Variable*>&,Formula* h, YYLTYPE);
			//!< create a new rule with an empty body
		Rule*	rule(Formula* h, Formula* b, YYLTYPE);
			//!< create a rule without quantified variables
		Rule*	rule(Formula* h, YYLTYPE);
			//!< create a rule without quantified variables and with an empty body
			
		void		addRule(FixpDef*, Rule*);	//!< add a rule to a fixpoint definition
		void		addDef(FixpDef*,FixpDef*);	//!< add a fixpoint definition to a fixpoint definition
		FixpDef*	createFD();					//!< create a new fixpoint definition
		void		makeLFD(FixpDef*,bool);		//!< make the fixpointdefinition a least or greatest fixpoint definition

		Formula*	trueform(YYLTYPE);		
			//!< create a new true formula 
		Formula*	falseform(YYLTYPE);	
			//!< create a new false formula 
		Formula*	funcgraphform(NSPair*, const std::vector<Term*>&, Term*, YYLTYPE);
			//!< create a new formula of the form F(t1,...,tn) = t
		Formula*	funcgraphform(NSPair*, Term*, YYLTYPE);
			//!< create a new formula of the form C = t
		Formula*	predform(NSPair*, const std::vector<Term*>&, YYLTYPE);
			//!< create a new formula of the form P(t1,...,tn)
		Formula*	predform(NSPair*, YYLTYPE);
			//!< create a new formula of the form P
		Formula*	equivform(Formula*,Formula*,YYLTYPE);
			//!< create a new formula of the form (phi1 <=> phi2)
		Formula*	disjform(Formula*,Formula*,YYLTYPE);
			//!< create a new formula of the form (phi1 | phi2)
		Formula*	conjform(Formula*,Formula*,YYLTYPE);
			//!< create a new formula of the form (phi1 & phi2)
		Formula*	implform(Formula*,Formula*,YYLTYPE);
			//!< create a new formula of the form (phi1 => phi2)
		Formula*	revimplform(Formula*,Formula*,YYLTYPE);
			//!< create a new formula of the form (phi1 <= phi2)
		Formula*	univform(const std::set<Variable*>&, Formula*, YYLTYPE l); 
			//!< create a new formula of the form (! x1 ... xn : phi)
		Formula*	existform(const std::set<Variable*>&, Formula*, YYLTYPE l); 
			//!< create a new formula of the form (? x1 ... xn : phi)
		Formula*	bexform(CompType, int, const std::set<Variable*>&, Formula*, YYLTYPE);
			//!< create a new formula of the form (?_op_n x1 ... xn : phi)
		Formula*	eqchain(CompType,Formula*,Term*,YYLTYPE);
			//!< add a term to an equation chain
		Formula*	eqchain(CompType,Term*,Term*,YYLTYPE);
			//!< create a new equation chain
		void negate(Formula*); 
			//!< negate a formula
			
		Variable*	quantifiedvar(const std::string& name, YYLTYPE l); 
			//!< create a new quantified variable
		Variable*	quantifiedvar(const std::string& name, Sort* sort, YYLTYPE l);
			//!< create a new quantified variable with a given sort
		Sort*		theosortpointer(const longname& vs, YYLTYPE l);
			//!< get a sort with a given name in the current vocabulary

		Term*	functerm(NSPair*, const std::vector<Term*>&);	//!< create a new function term
		Term*	functerm(NSPair*);								//!< create a new constant term
		Term*	arterm(char,Term*,Term*,YYLTYPE);				//!< create a new binary arithmetic term
		Term*	arterm(const std::string&,Term*,YYLTYPE);		//!< create a new unary arithmetic term
		Term*	domterm(int,YYLTYPE);							//!< create a new domain element term
		Term*	domterm(double,YYLTYPE);						//!< create a new domain element term
		Term*	domterm(std::string*,YYLTYPE);					//!< create a new domain element term
		Term*	domterm(char,YYLTYPE);							//!< create a new domain element term
		Term*	domterm(std::string*,Sort*,YYLTYPE);			//!< create a new domain element term of a given sort
		Term*	aggregate(AggFunction, SetExpr*, YYLTYPE);		//!< create a new aggregate term

		SetExpr*	set(const std::set<Variable*>&, Formula*, YYLTYPE);
			//!< Create a new set of the form { x1 ... xn : phi }
		SetExpr*	set(const std::set<Variable*>&, Formula*, Term*, YYLTYPE);
			//!< Create a new set of the form { x1 ... xn : phi : t }
		SetExpr*	set(EnumSetExpr*);
			//!< Cast EnumSetExpr to SetExpr

		EnumSetExpr*	createEnum();
			//!< Create a new EnumSetExpr
		void			addFormula(EnumSetExpr*,Formula*);
			//!< Add a tuple (phi,1) to an EnumSetExpr
		void			addFT(EnumSetExpr*,Formula*,Term*);
			//!< Add a tuple (phi,t) to an EnumSetExpr

		void emptyinter(NSPair*);				//!< Assign the empty interpretation
		void sortinter(NSPair*, SortTable* t);	//!< Assign a one dimensional table
		void predinter(NSPair*, PredTable* t);	//!< Assign a predicate table
		void funcinter(NSPair*, FuncTable* t);	//!< Assign a function table
		void truepredinter(NSPair*);			//!< Assign true
		void falsepredinter(NSPair*);			//!< Assign false
		void inter(NSPair*,InternalArgument*,YYLTYPE);	//!< Assign the result of a procedural call

		void threeprocinter(NSPair*, const std::string& utf, InternalArgument*);
		void threepredinter(NSPair*, const std::string& utf, PredTable* t);
		void threepredinter(NSPair*, const std::string& utf, SortTable* t);
		void truethreepredinter(NSPair*, const std::string& utf);
		void falsethreepredinter(NSPair*, const std::string& utf);
		void threefuncinter(NSPair*, const std::string& utf, FuncTable* t);
		void emptythreeinter(NSPair*, const std::string& utf);

		SortTable*	createSortTable();
		void	addElement(SortTable*,int);
		void	addElement(SortTable*,double);
		void	addElement(SortTable*,std::string*);
		void	addElement(SortTable*,const Compound*);
		void	addElement(SortTable*,int,int);
		void	addElement(SortTable*,char,char);

		PredTable*	createPredTable();
		void	addTuple(PredTable*, std::vector<const DomainElement*>&, YYLTYPE);
		void	addTuple(PredTable*, YYLTYPE);

		FuncTable*	createFuncTable();
		void	addTupleVal(FuncTable*, std::vector<const DomainElement*>&, YYLTYPE);
		void	addTupleVal(FuncTable*, const DomainElement*, YYLTYPE);

		const DomainElement*	element(int);
		const DomainElement*	element(double);
		const DomainElement*	element(char);
		const DomainElement*	element(std::string*);
		const DomainElement*	element(const Compound*);

		std::pair<int,int>*		range(int,int);
		std::pair<char,char>*	range(char,char);

		const Compound*	compound(NSPair*);
		const Compound*	compound(NSPair*,const std::vector<const DomainElement*>&);

		void predatom(NSPair*, const std::vector<ElRange>&, bool);
		void predatom(NSPair*, bool);
		void funcatom(NSPair*, const std::vector<ElRange>&, const DomainElement*, bool);
		void funcatom(NSPair*, const DomainElement*, bool);

		std::vector<ElRange>* domaintuple(const DomainElement*);
		std::vector<ElRange>* domaintuple(std::pair<int,int>*);
		std::vector<ElRange>* domaintuple(std::pair<char,char>*);
		std::vector<ElRange>* domaintuple(std::vector<ElRange>*,const DomainElement*);
		std::vector<ElRange>* domaintuple(std::vector<ElRange>*,std::pair<int,int>*);
		std::vector<ElRange>* domaintuple(std::vector<ElRange>*,std::pair<char,char>*);

		void procarg(const std::string&);	//!< Add an argument to the current procedure

		void exec(std::stringstream*);

		void option(const std::string& opt, const std::string& val,YYLTYPE);
		void option(const std::string& opt, double val,YYLTYPE);
		void option(const std::string& opt, int val,YYLTYPE);
};

















#ifdef OLD
	/** Input files **/
	std::string*	currfile();						// return the current filename
	void			currfile(const std::string& s);	// set the current file to s
	void			currfile(std::string* s);		// set the current file to s
	
	/** Initialize and destruct the parser TODO : avoid these...  **/
	void initialize();	
	void cleanup();		

	/** Namespaces **/

	/** Options **/

	/** Procedures **/
	void			openexec();
	LuaProcedure*	currproc();

	void openproc(const std::string& name, YYLTYPE);
	void closeproc();
	void procarg(const std::string& name);
	void luacloseargs();
	void luacode(char*);
	void luacode(const std::string&);
	void luacode(const std::vector<std::string>&);

	/** Structure **/
	void closestructure();
	void closeaspstructure();
	void closeaspbelief();

	// two-valued interpretations

	// three-valued interpretations

	// asp atoms

	// compound elements
	//Compound* makecompound(NSPair*, const std::vector<TypedElement*>& vte);
	//Compound* makecompound(NSPair*);

	/** Theory **/


	FTTuple*		fttuple(Formula* f, Term* t);


	/** Pointers to symbols **/

	/** Pointers to symbols in the current vocabulary **/

};

#endif 
#endif
