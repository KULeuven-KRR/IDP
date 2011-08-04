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
class AbstractStructure;
class Term;
class SetExpr;
class Query;
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
class lua_State; // TODO break lua connection
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
	ElRange() : _type(ERE_EL) { _value._element = 0;	}
	ElRange(const DomainElement* e) : _type(ERE_EL) { _value._element = e;	}
	ElRange(std::pair<int,int>* r) : _type(ERE_INT) { _value._intrange = r;	}
	ElRange(std::pair<char,char>* r) : _type(ERE_CHAR) { _value._charrange = r;	}
};

struct VarName {
	std::string	_name;
	Variable*	_var;
	VarName(const std::string& n, Variable* v) : _name(n), _var(v) { }
};

enum UTF { UTF_UNKNOWN, UTF_CT, UTF_CF, UTF_ERROR };

class Insert {

	private:
		Options*		_options;

		lua_State*		_state;		//!< the lua state objects are added to

		std::string*	_currfile;	//!< the file that is currently being parsed
		Namespace*		_currspace;	//!< the namespace that is currently being parsed

		Vocabulary*		_currvocabulary;	//!< the vocabulary that is currently being parsed
		Theory*			_currtheory;		//!< the theory that is currently being parsed
		Structure*		_currstructure;		//!< the structure that is currently being parsed
		Options*		_curroptions;		//!< the options that is currently being parsed
		UserProcedure*	_currprocedure;		//!< the procedure that is currently being parsed
		std::string		_currquery;			//!< the name of the named query that is currently being parsed
		std::string		_currterm;			//!< the name of the named term that is currently being parsed

		std::list<VarName>	_curr_vars;

		std::vector<Vocabulary*>	_usingvocab;	//!< the vocabularies currently used to parse
		std::vector<Namespace*>		_usingspace;	//!< the namespaces currently used to parse

		std::vector<unsigned int>	_nrvocabs;		//!< the number of 'using vocabulary' statements in the current block
		std::vector<unsigned int>	_nrspaces;		//!< the number of 'using namespace' statements in the current block

		ParseInfo			parseinfo(YYLTYPE l) const;	//!< Convert a bison parse location to a parseinfo object
		FormulaParseInfo	formparseinfo(Formula*,YYLTYPE) const;
		TermParseInfo		termparseinfo(Term*,const ParseInfo&) const;
		TermParseInfo		termparseinfo(Term*, YYLTYPE) const;
		SetParseInfo		setparseinfo(SetExpr*, YYLTYPE) const;

		Variable*			getVar(const std::string&) const;			//!< Returns the quantified variable with
																		//!< given name in the current scope
		std::set<Variable*>	freevars(const ParseInfo&);					//!< Return all currently free variables
		void				remove_vars(const std::vector<Variable*>&);	//!< Remove the given variables from the 
																		//!< list of free variables
		void				remove_vars(const std::set<Variable*>&);	//!< Remove the given variables from the 
																		//!< list of free variables

		void usenamespace(Namespace*);		//!< add a using namespace statement
		void usevocabulary(Vocabulary*);	//!< add a using vocabulary statement

		void openblock();	//!< open a new block
		void closeblock();	//!< close the current block


		Sort*					sortInScope(const std::string&, const ParseInfo&) const;
		Sort*					sortInScope(const longname&, const ParseInfo&) const;
		Predicate*				predInScope(const std::string&) const;
		Predicate*				predInScope(const longname&, const ParseInfo&) const;
		Function*				funcInScope(const std::string&) const;
		Function*				funcInScope(const longname&, const ParseInfo&) const;
		std::set<Predicate*>	noArPredInScope(const std::string& name) const;
		std::set<Predicate*>	noArPredInScope(const longname& name, const ParseInfo&) const;
		std::set<Function*>		noArFuncInScope(const std::string& name) const;
		std::set<Function*>		noArFuncInScope(const longname& name, const ParseInfo&) const;
		Vocabulary*				vocabularyInScope(const std::string&, const ParseInfo&) const;
		Vocabulary*				vocabularyInScope(const longname&, const ParseInfo&) const;
		Namespace*				namespaceInScope(const std::string&, const ParseInfo&) const;
		Namespace*				namespaceInScope(const longname&, const ParseInfo&) const;
		Query*					queryInScope(const std::string&, const ParseInfo&) const;
		Term*					termInScope(const std::string&, const ParseInfo&) const;
		AbstractTheory*			theoryInScope(const std::string&, const ParseInfo&) const;
		AbstractTheory*			theoryInScope(const longname&, const ParseInfo&) const;
		AbstractStructure*		structureInScope(const std::string&, const ParseInfo&) const;
		AbstractStructure*		structureInScope(const longname&, const ParseInfo&) const;
		UserProcedure*			procedureInScope(const std::string&, const ParseInfo&) const;
		UserProcedure*			procedureInScope(const longname&, const ParseInfo&) const;
		Options*				optionsInScope(const std::string&, const ParseInfo&) const;
		Options*				optionsInScope(const longname&, const ParseInfo&) const;

		bool	belongsToVoc(Predicate*)	const;
		bool	belongsToVoc(Function*)		const;
		bool	belongsToVoc(Sort*)			const;

		Formula*	boolform(bool,Formula*,Formula*,YYLTYPE) const;
		Formula*	quantform(bool,const std::set<Variable*>&, Formula*, YYLTYPE);

		std::map<Predicate*,PredTable*>	_unknownpredtables;
		std::map<Function*,PredTable*>	_unknownfunctables;
		std::map<Predicate*,UTF>		_cpreds;
		std::map<Function*,UTF>			_cfuncs;
		void	assignunknowntables();

	public:
		Insert();

		std::string*	currfile() const;				//!< return the current filename
		void			currfile(const std::string& s);	//!< set the current filename
		void			currfile(std::string* s);		//!< set the current filename

		void openspace(const std::string& name,YYLTYPE);		//!< Open a new namespace
		void openvocab(const std::string& name,YYLTYPE);		//!< Open a new vocabulary
		void opentheory(const std::string& tname, YYLTYPE);		//!< Open a new theory
		void openquery(const std::string& tname, YYLTYPE);		//!< Open a new named query
		void openterm(const std::string& tname, YYLTYPE);		//!< Open a new named term
		void openstructure(const std::string& name, YYLTYPE);	//!< Open a new structure
		void openprocedure(const std::string& name, YYLTYPE);	//!< Open a procedure
		void openoptions(const std::string& name, YYLTYPE);		//!< Open a new options block
		void openexec();										//!< Start parsing a command
		void closespace();										//!< Close the current namespace
		void closevocab();										//!< Close the current vocabulary
		void closetheory();										//!< Close the current theory
		void closequery(Query*);								//!< Close the current named query
		void closeterm(Term*);									//!< Close the current named term
		void closestructure();									//!< Close the current structure
		void closeprocedure(std::stringstream*);				//!< Close the current procedure
		void closeoptions();									//!< Close the current options

		void usingvocab(const longname& vname, YYLTYPE);	//!< use vocabulary 'vname' when parsing
		void usingspace(const longname& sname, YYLTYPE);	//!< use namespace 'sname' when parsing

		void assignvocab(InternalArgument*, YYLTYPE);		//!< set the current vocabulary to the given vocabulary
		void assigntheory(InternalArgument*, YYLTYPE);		//!< set the current theory to the given theory
		void assignstructure(InternalArgument*, YYLTYPE);	//!< set the current structure to the given structure

		void setvocab(const longname& vname, YYLTYPE);			//!< set the vocabulary of the current theory or structure 
		void externvocab(const longname& vname, YYLTYPE) const;	//!< add all symbols of 'vname' to the current vocabulary
		
		void externoption(const std::vector<std::string>& name, YYLTYPE) const;

		Sort*		sort(Sort* s) const;			//!< add an existing sort to the current vocabulary
		Predicate*	predicate(Predicate* p) const;	//!< add an existing predicate to the current vocabulary
		Function*	function(Function* f) const;	//!< add an existing function to the current vocabulary

		Sort*		sortpointer(const longname&, YYLTYPE) const;	
			//!< return the sort with the given name
		Predicate*	predpointer(longname&, const std::vector<Sort*>&, YYLTYPE) const;
			//!< return the predicate with the given name and sorts
		Predicate*	predpointer(longname&, int arity, YYLTYPE) const;	
			//!< return the predicate with the given name and arity
		Function*	funcpointer(longname&, const std::vector<Sort*>&, YYLTYPE) const;	
			//!< return the function with the given name and sorts
		Function*	funcpointer(longname&, int arity, YYLTYPE) const;	
			//!< return the function with the given name and arity

		NSPair*	internpredpointer(const longname&, const std::vector<Sort*>&, YYLTYPE) const;
		NSPair*	internfuncpointer(const longname&, const std::vector<Sort*>&, Sort*, YYLTYPE) const;
		NSPair*	internpointer(const longname& name, YYLTYPE) const;

		Sort*	sort(const std::string& name, YYLTYPE) const;						
		Sort*	sort(const std::string& name, const std::vector<Sort*> supbs, bool p, YYLTYPE) const;
		Sort*	sort(const std::string& name, const std::vector<Sort*> sups, const std::vector<Sort*> subs, YYLTYPE) const;

		Predicate*	predicate(const std::string& name, const std::vector<Sort*>& sorts, YYLTYPE) const;	
			//!< create a new predicate with the given sorts in the current vocabulary
		Predicate*	predicate(const std::string& name, YYLTYPE) const;	
			//!< create a new 0-ary predicate in the current vocabulary
			
		Function*	function(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort, YYLTYPE) const;	
			//!< create a new function in the current vocabulary
		Function*	function(const std::string& name, Sort* outsort, YYLTYPE) const;	
			//!< create a new constant in the current vocabulary
		Function*	aritfunction(const std::string& name, const std::vector<Sort*>& sorts, YYLTYPE) const;	
			//!< create a new arithmetic function in the current vocabulary
			
		void	partial(Function* f) const;
			//!< make a function partial

		InternalArgument* call(const longname& proc, const std::vector<longname>& args, YYLTYPE) const;
			//!< call a procedure
		InternalArgument* call(const longname& proc, YYLTYPE) const;
			//!< call a procedure

		void definition(Definition* d) const;		//!< add a definition to the current theory
		void sentence(Formula* f);					//!< add a sentence to the current theory
		void fixpdef(FixpDef* d) const;				//!< add a fixpoint defintion to the current theory


		Definition*	definition(const std::vector<Rule*>& r) const; //!< create a new definition
			
		Rule*	rule(const std::set<Variable*>&,Formula* h, Formula* b, YYLTYPE);
			//!< create a new rule 
		Rule*	rule(const std::set<Variable*>&,Formula* h, YYLTYPE);
			//!< create a new rule with an empty body
		Rule*	rule(Formula* h, Formula* b, YYLTYPE);
			//!< create a rule without quantified variables
		Rule*	rule(Formula* h, YYLTYPE);
			//!< create a rule without quantified variables and with an empty body
			
		void		addRule(FixpDef*, Rule*) const ;	//!< add a rule to a fixpoint definition
		void		addDef(FixpDef*,FixpDef*) const;	//!< add a fixpoint definition to a fixpoint definition
		FixpDef*	createFD() const;					//!< create a new fixpoint definition
		void		makeLFD(FixpDef*,bool) const;	//!< make the fixpointdefinition a least or greatest fixpoint definition

		Formula*	trueform(YYLTYPE) const;		
			//!< create a new true formula 
		Formula*	falseform(YYLTYPE) const;	
			//!< create a new false formula 
		Formula*	funcgraphform(NSPair*, const std::vector<Term*>&, Term*, YYLTYPE) const;
			//!< create a new formula of the form F(t1,...,tn) = t
		Formula*	funcgraphform(NSPair*, Term*, YYLTYPE) const;
			//!< create a new formula of the form C = t
		Formula*	predform(NSPair*, const std::vector<Term*>&, YYLTYPE) const;
			//!< create a new formula of the form P(t1,...,tn)
		Formula*	predform(NSPair*, YYLTYPE) const;
			//!< create a new formula of the form P
		Formula*	equivform(Formula*,Formula*,YYLTYPE) const;
			//!< create a new formula of the form (phi1 <=> phi2)
		Formula*	disjform(Formula*,Formula*,YYLTYPE) const;
			//!< create a new formula of the form (phi1 | phi2)
		Formula*	conjform(Formula*,Formula*,YYLTYPE) const;
			//!< create a new formula of the form (phi1 & phi2)
		Formula*	implform(Formula*,Formula*,YYLTYPE) const;
			//!< create a new formula of the form (phi1 => phi2)
		Formula*	revimplform(Formula*,Formula*,YYLTYPE) const;
			//!< create a new formula of the form (phi1 <= phi2)
		Formula*	univform(const std::set<Variable*>&, Formula*, YYLTYPE l); 
			//!< create a new formula of the form (! x1 ... xn : phi)
		Formula*	existform(const std::set<Variable*>&, Formula*, YYLTYPE l); 
			//!< create a new formula of the form (? x1 ... xn : phi)
		Formula*	bexform(CompType, int, const std::set<Variable*>&, Formula*, YYLTYPE);
			//!< create a new formula of the form (?_op_n x1 ... xn : phi)
		Formula*	eqchain(CompType,Formula*,Term*,YYLTYPE) const;
			//!< add a term to an equation chain
		Formula*	eqchain(CompType,Term*,Term*,YYLTYPE) const;
			//!< create a new equation chain
		void negate(Formula*) const; 
			//!< negate a formula
			
		Variable*	quantifiedvar(const std::string& name, YYLTYPE l); 
			//!< create a new quantified variable
		Variable*	quantifiedvar(const std::string& name, Sort* sort, YYLTYPE l);
			//!< create a new quantified variable with a given sort
		Sort*		theosortpointer(const longname& vs, YYLTYPE l) const;
			//!< get a sort with a given name in the current vocabulary

		Term*	functerm(NSPair*, const std::vector<Term*>&);		//!< create a new function term
		Term*	functerm(NSPair*);									//!< create a new constant term
		Term*	arterm(char,Term*,Term*,YYLTYPE) const;				//!< create a new binary arithmetic term
		Term*	arterm(const std::string&,Term*,YYLTYPE) const;		//!< create a new unary arithmetic term
		Term*	domterm(int,YYLTYPE) const;							//!< create a new domain element term
		Term*	domterm(double,YYLTYPE) const;						//!< create a new domain element term
		Term*	domterm(std::string*,YYLTYPE) const;				//!< create a new domain element term
		Term*	domterm(char,YYLTYPE) const;						//!< create a new domain element term
		Term*	domterm(std::string*,Sort*,YYLTYPE) const;			//!< create a new domain element term of a given sort
		Term*	aggregate(AggFunction, SetExpr*, YYLTYPE) const;	//!< create a new aggregate term

		Query*		query(const std::vector<Variable*>&, Formula*, YYLTYPE);
		SetExpr*	set(const std::set<Variable*>&, Formula*, YYLTYPE);
			//!< Create a new set of the form { x1 ... xn : phi }
		SetExpr*	set(const std::set<Variable*>&, Formula*, Term*, YYLTYPE);
			//!< Create a new set of the form { x1 ... xn : phi : t }
		SetExpr*	set(EnumSetExpr*) const;
			//!< Cast EnumSetExpr to SetExpr

		EnumSetExpr*	createEnum(YYLTYPE) const;
			//!< Create a new EnumSetExpr
		void			addFormula(EnumSetExpr*,Formula*) const;
			//!< Add a tuple (phi,1) to an EnumSetExpr
		void			addFT(EnumSetExpr*,Formula*,Term*) const;
			//!< Add a tuple (phi,t) to an EnumSetExpr

		void emptyinter(NSPair*) const;							//!< Assign the empty interpretation
		void sortinter(NSPair*, SortTable* t) const;			//!< Assign a one dimensional table
		void predinter(NSPair*, PredTable* t) const;			//!< Assign a predicate table
		void funcinter(NSPair*, FuncTable* t) const;			//!< Assign a function table
		void truepredinter(NSPair*) const;						//!< Assign true
		void falsepredinter(NSPair*) const;						//!< Assign false
		void inter(NSPair*, const longname& ,YYLTYPE) const;	//!< Assign a procedure
		void constructor(NSPair*) const;						//!< Assign a constructor 

		void threepredinter(NSPair*, const std::string& utf, PredTable* t);
		void threepredinter(NSPair*, const std::string& utf, SortTable* t);
		void truethreepredinter(NSPair*, const std::string& utf);
		void falsethreepredinter(NSPair*, const std::string& utf);
		void threefuncinter(NSPair*, const std::string& utf, PredTable* t);
		void emptythreeinter(NSPair*, const std::string& utf);

		SortTable*	createSortTable()					const;
		void	addElement(SortTable*,int)				const;
		void	addElement(SortTable*,double)			const;
		void	addElement(SortTable*,std::string*)		const;
		void	addElement(SortTable*,const Compound*)	const;
		void	addElement(SortTable*,int,int)			const;
		void	addElement(SortTable*,char,char)		const;

		PredTable*	createPredTable(unsigned int arity)												const;
		void	addTuple(PredTable*, std::vector<const DomainElement*>&, YYLTYPE)	const;
		void	addTuple(PredTable*, YYLTYPE)										const;

		FuncTable*	createFuncTable(unsigned int arity)													const;
		void	addTupleVal(FuncTable*, std::vector<const DomainElement*>&, YYLTYPE)	const;
		void	addTupleVal(FuncTable*, const DomainElement*, YYLTYPE)					const;

		const DomainElement*	element(int)					const;
		const DomainElement*	element(double)					const;
		const DomainElement*	element(char)					const;
		const DomainElement*	element(std::string*)			const;
		const DomainElement*	element(const Compound*)		const;

		std::pair<int,int>*		range(int,int,YYLTYPE)		const;
		std::pair<char,char>*	range(char,char,YYLTYPE)	const;

		const Compound*	compound(NSPair*)											const;
		const Compound*	compound(NSPair*,const std::vector<const DomainElement*>&)	const;

		void predatom(NSPair*, const std::vector<ElRange>&, bool)							const;
		void predatom(NSPair*, bool)														const;
		void funcatom(NSPair*, const std::vector<ElRange>&, const DomainElement*, bool)		const;
		void funcatom(NSPair*, const DomainElement*, bool)									const;

		std::vector<ElRange>* domaintuple(const DomainElement*)								const;
		std::vector<ElRange>* domaintuple(std::pair<int,int>*)								const;
		std::vector<ElRange>* domaintuple(std::pair<char,char>*)							const;
		std::vector<ElRange>* domaintuple(std::vector<ElRange>*,const DomainElement*)		const;
		std::vector<ElRange>* domaintuple(std::vector<ElRange>*,std::pair<int,int>*)		const;
		std::vector<ElRange>* domaintuple(std::vector<ElRange>*,std::pair<char,char>*)		const;

		void procarg(const std::string&)	const;	//!< Add an argument to the current procedure

		void exec(std::stringstream*)	const;

		void option(const std::string& opt, const std::string& val,YYLTYPE)	const;
		void option(const std::string& opt, double val,YYLTYPE)				const;
		void option(const std::string& opt, int val,YYLTYPE)				const;
		void option(const std::string& opt, bool val, YYLTYPE)				const;
};

#endif
