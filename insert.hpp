/************************************
	insert.h	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSERT_H
#define INSERT_H

#include <utility>
#include "theory.hpp"
class LuaProcedure;

struct YYLTYPE;

// Pair of formulas and terms
struct FTTuple {
	Formula* _formula;
	Term*	 _term;
	FTTuple(Formula* f, Term* t) : _formula(f), _term(t) { }
};

// Pair of name and sorts
struct NSTuple {
	vector<string>	_name;
	vector<Sort*>	_sorts;
	bool			_arityincluded;	// true iff the name ends on /arity
	bool			_sortsincluded;	// true iff the sorts are initialized
	bool			_func;			// true if the pointer points to a function
	ParseInfo		_pi;

	NSTuple(const vector<string>& n, const vector<Sort*>& s, bool ari, const ParseInfo& pi) :
		_name(n), _sorts(s), _arityincluded(ari), _sortsincluded(true), _func(false), _pi(pi) { }
	NSTuple(const vector<string>& n, bool ari, const ParseInfo& pi) :
		_name(n), _sorts(0), _arityincluded(ari), _sortsincluded(false), _func(false), _pi(pi) { }
	void func(bool b) { _func = b; }
	void includePredArity() { 
		assert(_sortsincluded && !_arityincluded); 
		_name.back() = _name.back() + '/' + itos(_sorts.size());	
		_arityincluded = true;
	}
	void includeFuncArity() {
		assert(_sortsincluded && !_arityincluded); 
		_name.back() = _name.back() + '/' + itos(_sorts.size() - 1);	
		_arityincluded = true;
	}
	void includeArity(unsigned int n) {
		assert(!_arityincluded); 
		_name.back() = _name.back() + '/' + itos(n);	
		_arityincluded = true;
	}
	string to_string();
};

namespace Insert {

	/** Input files **/
	string*	currfile();					// return the current filename
	void	currfile(const string& s);	// set the current file to s
	void	currfile(string* s);		// set the current file to s
	
	/** Initialize and destruct the parser TODO : avoid these...  **/
	void initialize();	
	void cleanup();		

	/** Namespaces **/
	void closespace();	// set current namespace to its parent
	void openspace(const string& sname,YYLTYPE);	// set the current namespace to its subspace with name 'sname'
	void usingspace(const vector<string>& sname, YYLTYPE);

	/** Options **/
	void openoptions(const string& name, YYLTYPE);
	void closeoptions();
	void externoption(const vector<string>& name, YYLTYPE);
	void option(const string& opt, const string& val,YYLTYPE);
	void option(const string& opt, double val,YYLTYPE);
	void option(const string& opt, int val,YYLTYPE);

	/** Procedures **/
	void			openexec();
	LuaProcedure*	currproc();

	void openproc(const string& name, YYLTYPE);
	void closeproc();
	void procarg(const string& name);
	void luacloseargs();
	void luacode(char*);
	void luacode(const string&);
	void luacode(const vector<string>&);

	/** Vocabulary **/
	void openvocab(const string& vname, YYLTYPE);	// create a new vocabulary with name 'vname' in the current namespace
	void closevocab();								// stop parsing a vocabulary
	void usingvocab(const vector<string>& vname, YYLTYPE);	// use vocabulary 'vname' when parsing
	void setvocab(const vector<string>& vname, YYLTYPE);	// set the vocabulary of the current theory or structure 
	void externvocab(const vector<string>& vname, YYLTYPE);	// add an existing vocabulary

	// Create new sorts
	Sort*	sort(const string& name, YYLTYPE);						
	Sort*	sort(const string& name, const vector<Sort*> supbs, bool p, YYLTYPE);
	Sort*	sort(const string& name, const vector<Sort*> sups, const vector<Sort*> subs, YYLTYPE);

	Sort*		sort(Sort* s);											// add an existing sort
	Predicate*	predicate(Predicate* p);								// add an existing predicate 
	Function*	function(Function* f);									// add an existing function 
	Predicate*	predicate(const string& name, const vector<Sort*>& sorts, YYLTYPE);	// create a new predicate
	Predicate*	predicate(const string& name, YYLTYPE);								// create a new 0-ary predicate
	Function*	function(const string& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE);	// create a new function
	Function*	function(const string& name, const vector<Sort*>& insorts, YYLTYPE);	// create a new function
	Function*	function(const string& name, Sort* outsort, YYLTYPE);									// create a new constant

	Sort*		copysort(const string& name, Sort*, YYLTYPE);		// copy a sort
	Predicate*	copypred(const string& name, Predicate*, YYLTYPE);	// copy a predicate
	Function*	copyfunc(const string& name, Function*, YYLTYPE);	// copy a function

	/** Structure **/
	void openstructure(const string& name, YYLTYPE);
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
	void threepredinter(NSTuple*, const string& utf, FinitePredTable* t);
	void truethreepredinter(NSTuple*, const string& utf);
	void falsethreepredinter(NSTuple*, const string& utf);
	void threefuncinter(NSTuple*, const string& utf, FinitePredTable* t);
	void threeinter(NSTuple*, const string& utf, FiniteSortTable* t);
	void emptythreeinter(NSTuple*, const string& utf);

	// asp atoms
	void predatom(NSTuple*, const vector<Element>&, const vector<FiniteSortTable*>&, const vector<ElementType>&, const vector<bool>&,bool);
	void funcatom(NSTuple*, const vector<Element>&, const vector<FiniteSortTable*>&, const vector<ElementType>&, const vector<bool>&,bool);

	// compound elements
	compound* makecompound(NSTuple*, const vector<TypedElement*>& vte);
	compound* makecompound(NSTuple*);

	/** Theory **/
	void opentheory(const string& tname, YYLTYPE);
	void closetheory();

	void definition(Definition* d);		// add a definition to the current theory
	void sentence(Formula* f);			// add a sentence to the current theory
	void fixpdef(FixpDef* d);			// add a fixpoint defintion to the current theory

	Rule*		rule(const vector<Variable*>&,PredForm* h, Formula* b, YYLTYPE);
	Rule*		rule(const vector<Variable*>&,PredForm* h, YYLTYPE);
	Rule*		rule(PredForm* h, Formula* b, YYLTYPE);
	Rule*		rule(PredForm* h, YYLTYPE);
	Definition*	definition(const vector<Rule*>& r);
	FixpDef*	fixpdef(bool,const vector<pair<Rule*,FixpDef*> >&);

	BoolForm*		trueform(YYLTYPE);		// the formula TRUE
	BoolForm*		falseform(YYLTYPE);		// the formula FALSE
	PredForm*		funcgraphform(NSTuple*, const vector<Term*>&, Term*, YYLTYPE);
	PredForm*		funcgraphform(NSTuple*, Term*, YYLTYPE);
	PredForm*		predform(NSTuple*, const vector<Term*>&, YYLTYPE);
	PredForm*		predform(NSTuple*, YYLTYPE);
	EquivForm*		equivform(Formula*,Formula*,YYLTYPE);
	BoolForm*		disjform(Formula*,Formula*,YYLTYPE);
	BoolForm*		conjform(Formula*,Formula*,YYLTYPE);
	BoolForm*		implform(Formula*,Formula*,YYLTYPE);
	BoolForm*		revimplform(Formula*,Formula*,YYLTYPE);
	QuantForm*		univform(const vector<Variable*>&, Formula*, YYLTYPE l); 
	QuantForm*		existform(const vector<Variable*>&, Formula*, YYLTYPE l); 
	PredForm*		bexform(char,bool,int,const vector<Variable*>&, Formula*, YYLTYPE l); 
	EqChainForm*	eqchain(char,bool,Term*,Term*,YYLTYPE l);
	EqChainForm*	eqchain(char,bool,EqChainForm*,Term*,YYLTYPE l);

	Variable*		quantifiedvar(const string& name, YYLTYPE l);
	Variable*		quantifiedvar(const string& name, Sort* sort, YYLTYPE l);

	FuncTerm*		functerm(NSTuple*, const vector<Term*>&);
	Term*			functerm(NSTuple*);
	Term*			arterm(char,Term*,Term*,YYLTYPE);
	Term*			arterm(const string&,Term*,YYLTYPE);

	AggTerm*		aggregate(AggType, SetExpr*, YYLTYPE);
	FTTuple*		fttuple(Formula* f, Term* t);
	QuantSetExpr*	set(const vector<Variable*>&, Formula*, YYLTYPE);
	EnumSetExpr*	set(const vector<FTTuple*>&, YYLTYPE);
	EnumSetExpr*	set(const vector<Formula*>&, YYLTYPE);

	DomainTerm*		domterm(int,YYLTYPE);
	DomainTerm*		domterm(double,YYLTYPE);
	DomainTerm*		domterm(string*,YYLTYPE);
	DomainTerm*		domterm(char,YYLTYPE);
	DomainTerm*		domterm(string*,Sort*,YYLTYPE);

	/** Pointers to symbols **/

	// Return the sort with the given name
	Sort*	sortpointer(const vector<string>& sname, YYLTYPE);	

	// Return the predicate with the given name and sorts
	Predicate*	predpointer(const vector<string>& pname, const vector<Sort*>&, YYLTYPE);
	Predicate*	predpointer(const vector<string>& pname, YYLTYPE);	

	// Return the function with the given name and sorts
	Function*	funcpointer(const vector<string>& fname, const vector<Sort*>&, YYLTYPE);	
	Function*	funcpointer(const vector<string>& fname, YYLTYPE);	

	/** Pointers to symbols in the current vocabulary **/
	Sort*		theosortpointer(const vector<string>& vs, YYLTYPE l);
	NSTuple*	internpointer(const vector<string>& name, const vector<Sort*>& sorts, YYLTYPE l);
	NSTuple*	internpointer(const vector<string>& name, YYLTYPE l);

}

#endif
