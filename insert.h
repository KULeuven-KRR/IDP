/************************************
	insert.h	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSERT_H
#define INSERT_H

#include <utility>
#include "theory.h"

struct YYLTYPE;

// Pair of formulas and terms
struct FTTuple {
	Formula* _formula;
	Term*	 _term;

	FTTuple(Formula* f, Term* t) : _formula(f), _term(t) { }
};

namespace Insert {

	/** Data **/
	string*	currfile();					// return the current filename
	void	currfile(const string& s);	// set the current file to s
	void	currfile(string* s);		// set the current file to s
	
	/** Global structure **/
	void initialize();	// initialize current namespace to the global namespace
	void cleanup();		
	void closespace();	// set current namespace to its parent
	void openspace(const string& sname,YYLTYPE);	// set the current namespace to its subspace with name 'sname'

	/** Vocabulary **/
	void openvocab(const string& vname, YYLTYPE);				// create a new vocabulary with name 'vname' 
																	// in the current namespace
	void closevocab();												// stop parsing a vocabulary
	void usingvocab(const vector<string>& vname, YYLTYPE);	// Use vocabulary 'vname' when parsing

	Sort*		sortpointer(const vector<string>& sname, YYLTYPE);		// return sort with name 'sname'
	Sort*		theosortpointer(const vector<string>& vs, YYLTYPE l);	// return sort with name 'sname'
	Predicate*	predpointer(const vector<string>& pname, YYLTYPE);		// return predicate with name 'pname'
	Function*	funcpointer(const vector<string>& fname, YYLTYPE);		// return function with name 'fname'

	Sort*		sort(const string& name, YYLTYPE);				// create a new base sort
	Sort*		sort(const string& name, Sort* sups, YYLTYPE);	// create a new subsort
	Predicate*	predicate(Predicate* p);								// add an existing predicate 
	Function*	function(Function* f);									// add an existing function 
	Predicate*	predicate(const string& name, const vector<Sort*>& sorts, YYLTYPE);	// create a new predicate
	Predicate*	predicate(const string& name, YYLTYPE);								// create a new 0-ary predicate
	Function*	function(const string& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE);	// create a new function
	Function*	function(const string& name, Sort* outsort, YYLTYPE);									// create a new constant

	Sort*		copysort(const string& name, Sort*, YYLTYPE);		// copy a sort
	Predicate*	copypred(const string& name, Predicate*, YYLTYPE);	// copy a predicate
	Function*	copyfunc(const string& name, Function*, YYLTYPE);	// copy a function

	/** Structure **/
	void openstructure(const string& name, YYLTYPE);
	void closestructure();
	void closeaspstructure();

	// two-valued interpretations
	void sortinter(const vector<string>& sname, UserSortTable* t, YYLTYPE);
	void predinter(const vector<string>& pname, UserPredTable* t, YYLTYPE);
	void funcinter(const vector<string>& fname, UserPredTable* t, YYLTYPE);
	void emptyinter(const vector<string>&,YYLTYPE);
	void truepredinter(const vector<string>&, YYLTYPE);
	void falsepredinter(const vector<string>&, YYLTYPE);

	// three-valued interpretations
	void threepredinter(const vector<string>& pname, const string& utf, FinitePredTable* t, YYLTYPE);
	void truethreepredinter(const vector<string>& pname, const string& utf, YYLTYPE);
	void falsethreepredinter(const vector<string>& pname, const string& utf, YYLTYPE);
	void threefuncinter(const vector<string>& fname, const string& utf, FinitePredTable* t, YYLTYPE);
	void threeinter(const vector<string>& name, const string& utf, UserSortTable* t, YYLTYPE);
	void emptythreeinter(const vector<string>& name, const string& utf, YYLTYPE);

	// asp atoms
	void predatom(const vector<string>&,YYLTYPE);
	void predatom(const vector<string>&, const vector<ElementType>&, const vector<Element>&, const vector<UserSortTable*>&, YYLTYPE);
	void funcatom(const vector<string>&, const vector<ElementType>&, const vector<Element>&, const vector<UserSortTable*>&, YYLTYPE);

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
	PredForm*		funcgraphform(const vector<string>&, const vector<Term*>&, Term*, YYLTYPE);
	PredForm*		funcgraphform(const vector<string>&, Term*, YYLTYPE);
	PredForm*		predform(const vector<string>&, const vector<Term*>&, YYLTYPE);
	PredForm*		predform(const vector<string>&, YYLTYPE);
	PredForm*		succform(Term*, Term*, YYLTYPE);
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

	FuncTerm*		functerm(const vector<string>&, const vector<Term*>&, YYLTYPE);
	FuncTerm*		functerm(const vector<string>&, YYLTYPE);
	Term*			funcvar(const vector<string>&, YYLTYPE);
	Term*			arterm(char,Term*,Term*,YYLTYPE);
	Term*			arterm(const string&,Term*,YYLTYPE);

	AggTerm*		aggregate(AggType, SetExpr*, YYLTYPE);
	FTTuple*		fttuple(Formula* f, Term* t);
	QuantSetExpr*	set(const vector<Variable*>&, Formula*, YYLTYPE);
	EnumSetExpr*	set(const vector<FTTuple*>&, YYLTYPE);
	EnumSetExpr*	set(const vector<Formula*>&, YYLTYPE);

	DomainTerm*		domterm(int,YYLTYPE);
	DomainTerm*		domterm(double*,YYLTYPE);
	DomainTerm*		domterm(string*,YYLTYPE);
	DomainTerm*		domterm(char,YYLTYPE);
	DomainTerm*		domterm(string*,Sort*,YYLTYPE);

	FuncTerm*		minterm(YYLTYPE l);
	FuncTerm*		minterm(Sort*,YYLTYPE l);
	FuncTerm*		maxterm(YYLTYPE l);
	FuncTerm*		maxterm(Sort*,YYLTYPE l);

	/** Statements **/
	void	command(InfArgType,const string& cname, const vector<string>& args, const string& res, YYLTYPE l);
	void	command(const string& cname, const vector<string>& args, YYLTYPE);
	void	command(InfArgType,const string& cname, const string& res, YYLTYPE);
	void	command(const string& cname, YYLTYPE);
}

#endif
