/************************************
	insert.hpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSERT_HPP
#define INSERT_HPP

#include <utility>
#include <string>
#include <vector>

#include "common.hpp" // FIXME: need include for enum AggType
#include "vocabulary.hpp" //FIXME: need include for ParseInfo

class Formula;
class Term;
class Sort;
class FiniteSortTable;
class FinitePredTable;

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
	std::vector<std::string>	_name;
	std::vector<Sort*>			_sorts;
	bool						_arityincluded;	// true iff the name ends on /arity
	bool						_sortsincluded;	// true iff the sorts are initialized
	bool						_func;			// true if the pointer points to a function
	ParseInfo					_pi;

	NSTuple(const std::vector<std::string>& n, const std::vector<Sort*>& s, bool ari, const ParseInfo& pi) :
		_name(n), _sorts(s), _arityincluded(ari), _sortsincluded(true), _func(false), _pi(pi) { }
	NSTuple(const std::vector<std::string>& n, bool ari, const ParseInfo& pi) :
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
	std::string to_string();
};

namespace Insert {

	/** Input files **/
	std::string*	currfile();						// return the current filename
	void			currfile(const std::string& s);	// set the current file to s
	void			currfile(std::string* s);		// set the current file to s
	
	/** Initialize and destruct the parser TODO : avoid these...  **/
	void initialize();	
	void cleanup();		

	/** Namespaces **/
	void closespace();	// set current namespace to its parent
	void openspace(const std::string& sname,YYLTYPE);	// set the current namespace to its subspace with name 'sname'
	void usingspace(const std::vector<std::string>& sname, YYLTYPE);

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
	void openvocab(const std::string& vname, YYLTYPE);	// create a new vocabulary with name 'vname' in the current namespace
	void closevocab();								// stop parsing a vocabulary
	void usingvocab(const std::vector<std::string>& vname, YYLTYPE);	// use vocabulary 'vname' when parsing
	void setvocab(const std::vector<std::string>& vname, YYLTYPE);	// set the vocabulary of the current theory or structure 
	void externvocab(const std::vector<std::string>& vname, YYLTYPE);	// add an existing vocabulary

	// Create new sorts
	Sort*	sort(const std::string& name, YYLTYPE);						
	Sort*	sort(const std::string& name, const std::vector<Sort*> supbs, bool p, YYLTYPE);
	Sort*	sort(const std::string& name, const std::vector<Sort*> sups, const std::vector<Sort*> subs, YYLTYPE);

	Sort*		sort(Sort* s);											// add an existing sort
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
	void predatom(NSTuple*, const std::vector<Element>&, const std::vector<FiniteSortTable*>&, const std::vector<ElementType>&, const std::vector<bool>&,bool);
	void funcatom(NSTuple*, const std::vector<Element>&, const std::vector<FiniteSortTable*>&, const std::vector<ElementType>&, const std::vector<bool>&,bool);

	// compound elements
	compound* makecompound(NSTuple*, const std::vector<TypedElement*>& vte);
	compound* makecompound(NSTuple*);

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

}

#endif
