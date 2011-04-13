/************************************
	insert.hpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSERT_HPP
#define INSERT_HPP

#include <vector>
#include "commontypes.hpp" 
#include "parseinfo.hpp"

struct YYLTYPE;
class lua_State;
class Options;
class UserProcedure;
class InternalArgument;

typedef std::vector<std::string> longname;

/**
 * Pair of a formula and a tuple
 */
struct FTPair {
	Formula* _formula;
	Term*	 _term;
	FTPair(Formula* f, Term* t) : _formula(f), _term(t) { }
};

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

		std::vector<Vocabulary*>	_usingvocab;	//!< the vocabularies currently used to parse
		std::vector<Namespace*>		_usingspace;	//!< the namespaces currently used to parse

		std::vector<unsigned int>	_nrvocabs;		//!< the number of 'using vocabulary' statements in the current block
		std::vector<unsigned int>	_nrspaces;		//!< the number of 'using namespace' statements in the current block

		ParseInfo	parseinfo(YYLTYPE l);	//!< Convert a bison parse location to a parseinfo object

		void usenamespace(Namespace*);		//!< add a using namespace statement
		void usevocabulary(Vocabulary*);	//!< add a using vocabulary statement

		void openblock();	//!< open a new block
		void closeblock();	//!< close the current block


	public:
		Insert();

		void openspace(const std::string& name,YYLTYPE);	//!< Open a new namespace;
		void openvocab(const std::string& name,YYLTYPE);	//!< Open a new vocabulary
		void closespace();									//!< Close the current namespace
		void closevocab();									//!< Close the current vocabulary

		void usingvocab(const longname& vname, YYLTYPE);	//!< use vocabulary 'vname' when parsing
		void usingspace(const longname& sname, YYLTYPE);	//!< use namespace 'sname' when parsing

		void assignvocab(const InternalArgument&, YYLTYPE);	//!< set the current vocabulary to the given vocabulary
		void setvocab(const longname& vname, YYLTYPE);		//!< set the vocabulary of the current theory or structure 
		void externvocab(const longname& vname, YYLTYPE);	//!< add all symbols of 'vname' to the current vocabulary

		Sort*	sort(Sort* s);	//!< add an existing sort to the current vocabulary
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
	void openoptions(const std::string& name, YYLTYPE);
	void closeoptions();
	void externoption(const std::vector<std::string>& name, YYLTYPE);
	void option(const std::string& opt, const std::string& val,YYLTYPE);
	void option(const std::string& opt, double val,YYLTYPE);
	void option(const std::string& opt, int val,YYLTYPE);

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

	/** Vocabulary **/

	// Create new sorts
	Sort*	sort(const std::string& name, YYLTYPE);						
	Sort*	sort(const std::string& name, const std::vector<Sort*> supbs, bool p, YYLTYPE);
	Sort*	sort(const std::string& name, const std::vector<Sort*> sups, const std::vector<Sort*> subs, YYLTYPE);

	Predicate*	predicate(Predicate* p);								// add an existing predicate 
	Function*	function(Function* f);									// add an existing function 
	Predicate*	predicate(const std::string& name, const std::vector<Sort*>& sorts, YYLTYPE);	// create a new predicate
	Predicate*	predicate(const std::string& name, YYLTYPE);								// create a new 0-ary predicate
	Function*	function(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort, YYLTYPE);	// create a new function
	Function*	function(const std::string& name, const std::vector<Sort*>& insorts, YYLTYPE);	// create a new function
	Function*	function(const std::string& name, Sort* outsort, YYLTYPE);									// create a new constant

	Sort*		copysort(const std::string& name, Sort*, YYLTYPE);		// copy a sort
	Predicate*	copypred(const std::string& name, Predicate*, YYLTYPE);	// copy a predicate
	Function*	copyfunc(const std::string& name, Function*, YYLTYPE);	// copy a function

	/** Structure **/
	void openstructure(const std::string& name, YYLTYPE);
	void closestructure();
	void closeaspstructure();
	void closeaspbelief();

	// two-valued interpretations
	void sortinter(NSTuple*, FiniteSortTable* t);
	void predinter(NSTuple*, FinitePredTable* t);
	void funcinter(NSTuple*, FinitePredTable* t);
	void emptyinter(NSTuple*);
	void truepredinter(NSTuple*);
	void falsepredinter(NSTuple*);

	// three-valued interpretations
	void threepredinter(NSTuple*, const std::string& utf, FinitePredTable* t);
	void truethreepredinter(NSTuple*, const std::string& utf);
	void falsethreepredinter(NSTuple*, const std::string& utf);
	void threefuncinter(NSTuple*, const std::string& utf, FinitePredTable* t);
	void threeinter(NSTuple*, const std::string& utf, FiniteSortTable* t);
	void emptythreeinter(NSTuple*, const std::string& utf);

	// asp atoms
//	void predatom(NSTuple*, const std::vector<const DomainElement*>&, const std::vector<FiniteSortTable*>&, const std::vector<ElementType>&, const std::vector<bool>&,bool);
//	void funcatom(NSTuple*, const std::vector<Element>&, const std::vector<FiniteSortTable*>&, const std::vector<ElementType>&, const std::vector<bool>&,bool);

	// compound elements
	//Compound* makecompound(NSTuple*, const std::vector<TypedElement*>& vte);
	//Compound* makecompound(NSTuple*);

	/** Theory **/
	void opentheory(const std::string& tname, YYLTYPE);
	void closetheory();

	void definition(Definition* d);		// add a definition to the current theory
	void sentence(Formula* f);			// add a sentence to the current theory
	void fixpdef(FixpDef* d);			// add a fixpoint defintion to the current theory

	Rule*		rule(const std::vector<Variable*>&,PredForm* h, Formula* b, YYLTYPE);
	Rule*		rule(const std::vector<Variable*>&,PredForm* h, YYLTYPE);
	Rule*		rule(PredForm* h, Formula* b, YYLTYPE);
	Rule*		rule(PredForm* h, YYLTYPE);
	Definition*	definition(const std::vector<Rule*>& r);
	FixpDef*	fixpdef(bool,const std::vector<std::pair<Rule*,FixpDef*> >&);

	BoolForm*		trueform(YYLTYPE);		// the formula TRUE
	BoolForm*		falseform(YYLTYPE);		// the formula FALSE
	PredForm*		funcgraphform(NSTuple*, const std::vector<Term*>&, Term*, YYLTYPE);
	PredForm*		funcgraphform(NSTuple*, Term*, YYLTYPE);
	PredForm*		predform(NSTuple*, const std::vector<Term*>&, YYLTYPE);
	PredForm*		predform(NSTuple*, YYLTYPE);
	EquivForm*		equivform(Formula*,Formula*,YYLTYPE);
	BoolForm*		disjform(Formula*,Formula*,YYLTYPE);
	BoolForm*		conjform(Formula*,Formula*,YYLTYPE);
	BoolForm*		implform(Formula*,Formula*,YYLTYPE);
	BoolForm*		revimplform(Formula*,Formula*,YYLTYPE);
	QuantForm*		univform(const std::vector<Variable*>&, Formula*, YYLTYPE l); 
	QuantForm*		existform(const std::vector<Variable*>&, Formula*, YYLTYPE l); 
	PredForm*		bexform(char,bool,int,const std::vector<Variable*>&, Formula*, YYLTYPE l); 
	EqChainForm*	eqchain(char,bool,Term*,Term*,YYLTYPE l);
	EqChainForm*	eqchain(char,bool,EqChainForm*,Term*,YYLTYPE l);

	Variable*		quantifiedvar(const std::string& name, YYLTYPE l);
	Variable*		quantifiedvar(const std::string& name, Sort* sort, YYLTYPE l);

	FuncTerm*		functerm(NSTuple*, const std::vector<Term*>&);
	Term*			functerm(NSTuple*);
	Term*			arterm(char,Term*,Term*,YYLTYPE);
	Term*			arterm(const std::string&,Term*,YYLTYPE);

	AggTerm*		aggregate(AggType, SetExpr*, YYLTYPE);
	FTTuple*		fttuple(Formula* f, Term* t);
	QuantSetExpr*	set(const std::vector<Variable*>&, Formula*, YYLTYPE);
	EnumSetExpr*	set(const std::vector<FTTuple*>&, YYLTYPE);
	EnumSetExpr*	set(const std::vector<Formula*>&, YYLTYPE);

	DomainTerm*		domterm(int,YYLTYPE);
	DomainTerm*		domterm(double,YYLTYPE);
	DomainTerm*		domterm(std::string*,YYLTYPE);
	DomainTerm*		domterm(char,YYLTYPE);
	DomainTerm*		domterm(std::string*,Sort*,YYLTYPE);

	/** Pointers to symbols **/

	// Return the sort with the given name
	Sort*	sortpointer(const std::vector<std::string>& sname, YYLTYPE);	

	// Return the predicate with the given name and sorts
	Predicate*	predpointer(const std::vector<std::string>& pname, const std::vector<Sort*>&, YYLTYPE);
	Predicate*	predpointer(const std::vector<std::string>& pname, YYLTYPE);	

	// Return the function with the given name and sorts
	Function*	funcpointer(const std::vector<std::string>& fname, const std::vector<Sort*>&, YYLTYPE);	
	Function*	funcpointer(const std::vector<std::string>& fname, YYLTYPE);	

	/** Pointers to symbols in the current vocabulary **/
	Sort*		theosortpointer(const std::vector<std::string>& vs, YYLTYPE l);
	NSTuple*	internpointer(const std::vector<std::string>& name, const std::vector<Sort*>& sorts, YYLTYPE l);
	NSTuple*	internpointer(const std::vector<std::string>& name, YYLTYPE l);

};

#endif 
#endif
