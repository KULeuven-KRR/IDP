/************************************
	insert.cpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cassert>
#include <sstream>
#include "common.hpp"
#include "insert.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "namespace.hpp"
#include "parse.tab.hh"
#include "error.hpp"
#include "execute.hpp"
using namespace std;

class SortDeriver : public TheoryMutatingVisitor {

	private:
		map<Variable*,set<Sort*> >	_untyped;			// The untyped variables, with their possible types
		map<FuncTerm*,Sort*>		_overloadedterms;	// The terms with an overloaded function
		set<PredForm*>				_overloadedatoms;	// The atoms with an overloaded predicate
		set<DomainTerm*>			_domelements;		// The untyped domain elements
		bool						_changed;
		bool						_firstvisit;
		Sort*						_assertsort;
		Vocabulary*					_vocab;

	public:
		// Constructor
		SortDeriver(Formula* f,Vocabulary* v) : _vocab(v) { run(f); }
		SortDeriver(Rule* r,Vocabulary* v)	: _vocab(v) { run(r); }

		// Run sort derivation 
		void run(Formula*);
		void run(Rule*);

		// Visit 
		Formula*	visit(QuantForm*);
		Formula*	visit(PredForm*);
		Formula*	visit(EqChainForm*);
		Rule*		visit(Rule*);
		Term*		visit(VarTerm*);
		Term*		visit(DomainTerm*);
		Term*		visit(FuncTerm*);
		SetExpr*	visit(QuantSetExpr*);

		// Traversal
		Formula*	traverse(Formula*);
		Rule*		traverse(Rule*);
		Term*		traverse(Term*);
		SetExpr*	traverse(SetExpr*);

	private:
		// Auxiliary methods
		void derivesorts();		// derive the sorts of the variables, based on the sorts in _untyped
		void derivefuncs();		// disambiguate the overloaded functions
		void derivepreds();		// disambiguate the overloaded predicates

		// Check
		void check();
		
};

class SortChecker : public TheoryVisitor {

	private:
		Vocabulary* _vocab;

	public:
		SortChecker(Formula* f,Vocabulary* v)		: _vocab(v) { f->accept(this);	}
		SortChecker(Definition* d,Vocabulary* v)	: _vocab(v) { d->accept(this);	}
		SortChecker(FixpDef* d,Vocabulary* v)		: _vocab(v) { d->accept(this);	}

		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const FuncTerm*);
		void visit(const AggTerm*);

};

/**
 * Rewrite a vector of strings s1,s2,...,sn to the single string s1::s2::...::sn
 */
string oneName(const longname& vs) {
	stringstream sstr;
	if(!vs.empty()) {
		sstr << vs[0];
		for(unsigned int n = 1; n < vs.size(); ++n) sstr << "::" << vs[n];
	}
	return sstr.str();
}

string predName(const longname& name, const vector<Sort*>& vs) {
	stringstream sstr;
	sstr << oneName(name);
	if(!vs.empty()) {
		sstr << '[' << vs[0]->name();
		for(unsigned int n = 1; n < vs.size(); ++n) sstr << ',' << vs[n]->name();
		sstr << ']';
	}
	return sstr.str();
}

string funcName(const longname& name, const vector<Sort*>& vs) {
	assert(!vs.empty());
	stringstream sstr;
	sstr << oneName(name) << '[';
	if(vs.size() > 1) {
		sstr << vs[0]->name();
		for(unsigned int n = 1; n < vs.size()-1; ++n) sstr << ',' << vs[n]->name();
	}
	sstr << ':' << vs.back()->name() << ']';
	return sstr.str();
}

/*************
	NSPair
*************/

void NSPair::includePredArity() {
	assert(_sortsincluded && !_arityincluded); 
	_name.back() = _name.back() + '/' + itos(_sorts.size());	
	_arityincluded = true;
}

void NSPair::includeFuncArity() {
	assert(_sortsincluded && !_arityincluded); 
	_name.back() = _name.back() + '/' + itos(_sorts.size() - 1);	
	_arityincluded = true;
}

void NSPair::includeArity(unsigned int n) {
	assert(!_arityincluded); 
	_name.back() = _name.back() + '/' + itos(n);	
	_arityincluded = true;
}

/*************
	Insert
*************/

Insert::Insert() {
	openblock();
	_currfile = 0;
	_currspace = Namespace::global();
	usenamespace(_currspace);
}

string* Insert::currfile() const {
	return _currfile;
}

void Insert::currfile(const string& s) {
	_currfile = StringPointer(s);
}

void Insert::currfile(string* s) {
	_currfile = s;
}

void Insert::partial(Function* f) const {
	f->partial(true);
}

void Insert::makeLFD(FixpDef* d, bool lfp) const {
	if(d) d->lfp(lfp);
}

void Insert::addRule(FixpDef* d, Rule* r) const {
	if(d && r) d->add(r);
}

void Insert::addDef(FixpDef* d, FixpDef* sd) const {
	if(d && sd) d->add(sd);
}

FixpDef* Insert::createFD() const {
	return new FixpDef;
}

ParseInfo Insert::parseinfo(YYLTYPE l) const { 
	return ParseInfo(l.first_line,l.first_column,_currfile);	
}

set<Variable*> Insert::freevars(const ParseInfo& pi) {
	std::set<Variable*> vv;
	string vs;
	for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
		vv.insert(i->_var);
		vs = vs + ' ' + i->_name;
	}
	if(!vv.empty()) Warning::freevars(vs,pi);
	curr_vars.clear();
	return vv;
}

void Insert::remove_vars(const std::set<Variable*>& v) {
	for(std::set<Variable*>::const_iterator it = v.begin(); it != v.end(); ++it) {
		for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
			if(i->_name == (*it)->name()) {
				curr_vars.erase(i);
				break;
			}
		}
	}
}

void Insert::usenamespace(Namespace* s) {
	++_nrspaces.back();
	_usingspace.push_back(s);
}

void Insert::usevocabulary(Vocabulary* v) {
	++_nrvocabs.back();
	_usingvocab.push_back(v);
}

void Insert::usingvocab(const longname& vs, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vs,pi);
	if(v) usevocabulary(v);
	else Error::undeclvoc(oneName(vs),pi);
}

void Insert::usingspace(const longname& vs, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	Namespace* s = namespaceInScope(vs,pi);
	if(s) usenamespace(s);
	else Error::undeclspace(oneName(vs),pi);
}

void Insert::openblock() {
	_nrvocabs.push_back(0);
	_nrspaces.push_back(0);
}

void Insert::closeblock() {
	for(unsigned int n = 0; n < _nrvocabs.back(); ++n) _usingvocab.pop_back();
	for(unsigned int n = 0; n < _nrspaces.back(); ++n) _usingspace.pop_back();
	_nrvocabs.pop_back();
	_nrspaces.pop_back();
	_currvocabulary = 0;
	_currtheory = 0;
	_currstructure = 0;
	_curroptions = 0;
	_currprocedure = 0;
}

void Insert::openspace(const string& sname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	Namespace* ns = new Namespace(sname,_currspace,pi);
	_currspace = ns;
	usenamespace(ns);
}

void Insert::closespace() {
	if(_currspace->super()->isGlobal()) LuaConnection::addGlobal(_currspace);
	_currspace = _currspace->super(); assert(_currspace);
	closeblock();
}

void Insert::openvocab(const string& vname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vname,pi);
	if(v) Error::multdeclvoc(vname,pi,v->pi());
	_currvocabulary = new Vocabulary(vname,pi);
	_currspace->add(_currvocabulary);
	usevocabulary(_currvocabulary);
}

void Insert::assignvocab(InternalArgument* arg, YYLTYPE l) {
	Vocabulary* v = LuaConnection::vocabulary(arg);
	if(v) {
		_currvocabulary->addVocabulary(v);
	}
	else {
		ParseInfo pi = parseinfo(l);
		Error::vocabexpected(pi);
	}
}

void Insert::closevocab() {
	assert(_currvocabulary);
	if(_currspace->isGlobal()) LuaConnection::addGlobal(_currvocabulary);
	closeblock();
}

void Insert::setvocab(const longname& vs, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vs,pi);
	if(v) {
		usevocabulary(v);
		_currvocabulary = v;
		if(_currstructure) _currstructure->vocabulary(v);
		else if(_currtheory) _currtheory->vocabulary(v);
		else assert(false);
	}
	else {
		Error::undeclvoc(oneName(vs),pi);
		_currvocabulary = Vocabulary::std();
		if(_currstructure) _currstructure->vocabulary(Vocabulary::std());
		else if(_currtheory) _currtheory->vocabulary(Vocabulary::std());
		else assert(false);
	}
}

void Insert::externvocab(const vector<string>& vname, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vname,pi);
	if(v) _currvocabulary->addVocabulary(v); 
	else Error::undeclvoc(oneName(vname),pi);
}

void Insert::opentheory(const string& tname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	AbstractTheory* t = theoryInScope(tname,pi);
	if(t) Error::multdecltheo(tname,pi,t->pi());
	_currtheory = new Theory(tname,pi);
	_currspace->add(_currtheory);
}

void Insert::assigntheory(InternalArgument* arg, YYLTYPE l) {
	AbstractTheory* t = LuaConnection::theory(arg);
	if(t) _currtheory->addTheory(t);
	else {
		ParseInfo pi = parseinfo(l);
		Error::theoryexpected(pi);
	}
}

void Insert::closetheory() {
	assert(_currtheory);
	if(_currspace->isGlobal()) LuaConnection::addGlobal(_currtheory);
	closeblock();
}

Sort* Insert::sort(Sort* s) const {
	if(s) _currvocabulary->addSort(s);
	return s;
}

Predicate* Insert::predicate(Predicate* p) const {
	if(p) _currvocabulary->addPred(p);
	return p;
}

Function* Insert::function(Function* f) const {
	if(f) _currvocabulary->addFunc(f);
	return f;
}

Sort* Insert::sortpointer(const longname& vs, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	Sort* s = sortInScope(vs,pi);
	if(!s) Error::undeclsort(oneName(vs),pi);
	return s;
}

Predicate* Insert::predpointer(longname& vs, int arity, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	vs.back() = vs.back() + '/' + itos(arity);
	Predicate* p = predInScope(vs,pi);
	if(!p) Error::undeclpred(oneName(vs),pi);
	return p;
}

Predicate* Insert::predpointer(longname& vs, const vector<Sort*>& va, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	longname copyvs = vs;
	Predicate* p = predpointer(copyvs,va.size(),l);
	if(p) p = p->resolve(va);
	if(!p) Error::undeclpred(predName(vs,va),pi);
	return p;
}

Function* Insert::funcpointer(longname& vs, int arity, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	vs.back() = vs.back() + '/' + itos(arity);
	Function* f = funcInScope(vs,pi);
	if(!f) Error::undeclfunc(oneName(vs),pi);
	return f;
}

Function* Insert::funcpointer(longname& vs, const vector<Sort*>& va, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	longname copyvs = vs;
	Function* f = funcpointer(copyvs,va.size()-1,l);
	if(f) f = f->resolve(va);
	if(!f) Error::undeclfunc(funcName(vs,va),pi);
	return f;
}

NSPair* Insert::internpredpointer(const vector<string>& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return new NSPair(name,sorts,false,pi);
}

NSPair* Insert::internfuncpointer(const vector<string>& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	NSPair* nsp = new NSPair(name,insorts,false,pi);
	nsp->_sorts.push_back(outsort);
	nsp->_func = true;
	return nsp;
}

NSPair* Insert::internpointer(const vector<string>& name, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return new NSPair(name,false,pi);
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name		the name of the new sort	
 * \param sups		the supersorts of the new sort
 * \param subs		the subsorts of the new sort
 */
Sort* Insert::sort(const string& name, const vector<Sort*> sups, const vector<Sort*> subs, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);

	// Create the sort
	Sort* s = new Sort(name,pi);

	// Add the sort to the current vocabulary
	_currvocabulary->addSort(s);

	// Collect the ancestors of all super- and subsorts
	vector<std::set<Sort*> > supsa(sups.size());
	vector<std::set<Sort*> > subsa(subs.size());
	for(unsigned int n = 0; n < sups.size(); ++n) {
		if(sups[n]) {
			supsa[n] = sups[n]->ancestors(0);
			supsa[n].insert(sups[n]);
		}
	}
	for(unsigned int n = 0; n < subs.size(); ++n) {
		if(subs[n]) {
			subsa[n] = subs[n]->ancestors(0);
			subsa[n].insert(subs[n]);
		}
	}

	// Add the supersorts
	for(unsigned int n = 0; n < sups.size(); ++n) {
		if(sups[n]) {
			for(unsigned int m = 0; m < subs.size(); ++m) {
				if(subs[m]) {
					if(subsa[m].find(sups[n]) == subsa[m].end()) {
						Error::notsubsort(subs[m]->name(),sups[n]->name(),parseinfo(l));
					}
				}
			}
			s->addParent(sups[n]);
		}
	}

	// Add the subsorts
	for(unsigned int n = 0; n < subs.size(); ++n) {
		if(subs[n]) {
			for(unsigned int m = 0; m < sups.size(); ++m) {
				if(sups[m]) {
					if(supsa[m].find(subs[n]) != supsa[m].end()) {
						Error::cyclichierarchy(subs[n]->name(),sups[m]->name(),parseinfo(l));
					}
				}
			}
			s->addChild(subs[n]);
		}
	}
	return s;
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name	the name of the sort
 */
Sort* Insert::sort(const string& name, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return sort(name,vs,vs,l);
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name		the name of the sort
 * \param supbs		the super- or subsorts of the sort
 * \param p			true if supbs are the supersorts, false if supbs are the subsorts
 */
Sort* Insert::sort(const string& name, const vector<Sort*> supbs, bool p, YYLTYPE l) const {
	vector<Sort*> vs(0);
	if(p) return sort(name,supbs,vs,l);
	else return sort(name,vs,supbs,l);
}


Predicate* Insert::predicate(const string& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	string nar = string(name) + '/' + itos(sorts.size());
	for(unsigned int n = 0; n < sorts.size(); ++n) {
		if(!sorts[n]) return 0;
	}
	Predicate* p = new Predicate(nar,sorts,pi);
	_currvocabulary->addPred(p);
	return p;
}

Predicate* Insert::predicate(const string& name, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return predicate(name,vs,l);
}

Function* Insert::function(const string& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	string nar = string(name) + '/' + itos(insorts.size());
	for(unsigned int n = 0; n < insorts.size(); ++n) {
		if(!insorts[n]) return 0;
	}
	if(!outsort) return 0;
	Function* f = new Function(nar,insorts,outsort,pi);
	_currvocabulary->addFunc(f);
	return f;
}

Function* Insert::function(const string& name, Sort* outsort, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return function(name,vs,outsort,l);
}

Function* Insert::aritfunction(const string& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	for(unsigned int n = 0; n < sorts.size(); ++n) {
		if(!sorts[n]) return 0;
	}
	Function* orig = _currvocabulary->func(name);
	unsigned int binding = orig ? orig->binding() : 0;
	Function* f = new Function(name,sorts,pi,binding);
	_currvocabulary->addFunc(f);
	return f;
}

InternalArgument* Insert::call(const longname& proc, const vector<longname>& args, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return LuaConnection::call(proc,args,pi);
}

InternalArgument* Insert::call(const longname& proc, YYLTYPE l) const {
	vector<longname> vl(0);
	return call(proc,vl,l);
}

void Insert::definition(Definition* d) const {
	if(d) _currtheory->add(d);
}

void Insert::sentence(Formula* f) {
	if(f) {
		// 1. Quantify the free variables universally
		std::set<Variable*> vv = freevars(f->pi());
		if(!vv.empty()) f =  new QuantForm(true,true,vv,f,f->pi());
		// 2. Sort derivation & checking
		SortDeriver sd(f,_currvocabulary); 
		SortChecker sc(f,_currvocabulary);
		// Add the formula to the current theory
		_currtheory->add(f);
	}
	else curr_vars.clear();
}

void Insert::fixpdef(FixpDef* d) const {
	if(d) _currtheory->add(d);
}

Definition* Insert::definition(const vector<Rule*>& rules) const {
	Definition* d = new Definition();
	for(unsigned int n = 0; n < rules.size(); ++n) {
		if(rules[n]) d->add(rules[n]);
	}
	return d;
}

Rule* Insert::rule(const std::set<Variable*>& qv,Formula* head, Formula* body,YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	remove_vars(qv);
	if(head && body) {
		// Quantify the free variables
		std::set<Variable*> vv = freevars(head->pi());
		remove_vars(vv);
		// Split quantified variables in head and body variables
		std::set<Variable*> hv;
		std::set<Variable*> bv;
		for(std::set<Variable*>::const_iterator it = qv.begin(); it != qv.end(); ++it) {
			if(head->contains(*it)) hv.insert(*it);
			else bv.insert(*it);
		}
		for(std::set<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) {
			if(head->contains(*it)) hv.insert(*it);
			else bv.insert(*it);
		}
		// Create a new rule
		if(!(bv.empty())) body = new QuantForm(true,false,bv,body,FormulaParseInfo((body->pi())));
		assert(typeid(*head) == typeid(PredForm));
		PredForm* pfhead = dynamic_cast<PredForm*>(head);
		Rule* r = new Rule(hv,pfhead,body,pi);
		// Sort derivation
		SortDeriver sd(r,_currvocabulary);
		// Return the rule
		return r;
	}
	else {
		curr_vars.clear();
		if(head) head->recursiveDelete();
		if(body) body->recursiveDelete();
		for(std::set<Variable*>::const_iterator it = qv.begin(); it != qv.end(); ++it) delete(*it);
		return 0;
	}
}

Rule* Insert::rule(const std::set<Variable*>& qv, Formula* head, YYLTYPE l) {
	Formula* body = FormulaUtils::trueform();
	return rule(qv,head,body,l);
}

Rule* Insert::rule(Formula* head, Formula* body, YYLTYPE l) {
	std::set<Variable*> vv;
	return rule(vv,head,body,l);
}

Rule* Insert::rule(Formula* head,YYLTYPE l) {
	Formula* body = FormulaUtils::trueform();
	return rule(head,body,l);
}

Formula* Insert::trueform(YYLTYPE l) const {
	vector<Formula*> vf(0);
	FormulaParseInfo pi = formparseinfo(new BoolForm(true,true,vf,FormulaParseInfo()),l);
	return new BoolForm(true,true,vf,pi);
}

Formula* Insert::falseform(YYLTYPE l) const {
	vector<Formula*> vf(0);
	FormulaParseInfo pi = formparseinfo(new BoolForm(true,false,vf,FormulaParseInfo()),l);
	return new BoolForm(true,false,vf,pi);
}

Formula* Insert::predform(NSPair* nst, const vector<Term*>& vt, YYLTYPE l) const {
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != vt.size()) Error::incompatiblearity(nst->to_string(),nst->_pi);
		if(nst->_func) Error::prednameexpected(nst->_pi);
	}
	nst->includeArity(vt.size());
	Predicate* p = predInScope(nst->_name,nst->_pi);
	if(p && nst->_sortsincluded && (nst->_sorts).size() == vt.size()) p = p->resolve(nst->_sorts);

	PredForm* pf = 0;
	if(p) {
		if(belongsToVoc(p)) {
			unsigned int n = 0;
			for(; n < vt.size(); ++n) { if(!vt[n]) break; }
			if(n == vt.size()) {
				vector<Term*> vtpi;
				for(vector<Term*>::const_iterator it = vt.begin(); it != vt.end(); ++it) {
					if((*it)->pi().original()) vtpi.push_back((*it)->pi().original()->clone());
					else vtpi.push_back((*it)->clone());
				}
				FormulaParseInfo pi = formparseinfo(new PredForm(true,p,vtpi,FormulaParseInfo()),l);
				pf = new PredForm(true,p,vt,pi);
			}
		}
		else Error::prednotintheovoc(p->name(),_currtheory->name(),nst->_pi);
	}
	else Error::undeclpred(nst->to_string(),nst->_pi);
	
	// Cleanup
	if(!pf) {
		for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) vt[n]->recursiveDelete();	}
	}
	delete(nst);

	return pf;
}

Formula* Insert::predform(NSPair* t, YYLTYPE l) const {
	vector<Term*> vt(0);
	return predform(t,vt,l);
}

Formula* Insert::funcgraphform(NSPair* nst, const vector<Term*>& vt, Term* t, YYLTYPE l) const {
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != vt.size() + 1) Error::incompatiblearity(nst->to_string(),nst->_pi);
		if(!nst->_func) Error::funcnameexpected(nst->_pi);
	}
	nst->includeArity(vt.size());
	Function* f = funcInScope(nst->_name,nst->_pi);
	if(f && nst->_sortsincluded && (nst->_sorts).size() == vt.size()+1) f = f->resolve(nst->_sorts);

	PredForm* pf = 0;
	if(f) {
		if(belongsToVoc(f)) {
			unsigned int n = 0;
			for(; n < vt.size(); ++n) { if(!vt[n]) break; }
			if(n == vt.size() && t) {
				vector<Term*> vt2(vt); vt2.push_back(t);
				vector<Term*> vtpi;
				for(vector<Term*>::const_iterator it = vt2.begin(); it != vt2.end(); ++it) {
					if((*it)->pi().original()) vtpi.push_back((*it)->pi().original()->clone());
					else vtpi.push_back((*it)->clone());
				}
				FormulaParseInfo pi = formparseinfo(new PredForm(true,f,vtpi,FormulaParseInfo()),l);
				pf = new PredForm(true,f,vt2,pi);	
			}
		}
		else Error::funcnotintheovoc(f->name(),_currtheory->name(),nst->_pi);
	}
	else Error::undeclfunc(nst->to_string(),nst->_pi);

	// Cleanup
	if(!pf) {
		for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
		if(t) delete(t);
	}
	delete(nst);

	return pf;
}

Formula* Insert::funcgraphform(NSPair* nst, Term* t, YYLTYPE l) const {
	vector<Term*> vt;
	return funcgraphform(nst,vt,t,l);
}

Formula* Insert::equivform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if(lf && rf) {
		Formula* lfpi = lf->pi().original() ? lf->pi().original()->clone() : lf->clone();
		Formula* rfpi = rf->pi().original() ? rf->pi().original()->clone() : rf->clone();
		FormulaParseInfo pi = formparseinfo(new EquivForm(true,lfpi,rfpi,FormulaParseInfo()),l);
		return new EquivForm(true,lf,rf,pi);
	}
	else {
		if(lf) delete(lf);
		if(rf) delete(rf);
		return 0;
	}
}

Formula* Insert::boolform(bool conj, Formula* lf, Formula* rf, YYLTYPE l) const {
	if(lf && rf) {
		vector<Formula*> vf(2);
		vector<Formula*> pivf(2);
		vf[0] = lf; vf[1] = rf;
		pivf[0] = lf->pi().original() ? lf->pi().original()->clone() : lf->clone();
		pivf[1] = rf->pi().original() ? rf->pi().original()->clone() : rf->clone();
		FormulaParseInfo pi = formparseinfo(new BoolForm(true,conj,pivf,FormulaParseInfo()),l);
		return new BoolForm(true,conj,vf,pi);
	}
	else {
		if(lf) delete(lf);
		if(rf) delete(rf);
		return 0;
	}
}

Formula* Insert::disjform(Formula* lf, Formula* rf, YYLTYPE l) const {
	return boolform(false,lf,rf,l);
}

Formula* Insert::conjform(Formula* lf, Formula* rf, YYLTYPE l) const {
	return boolform(true,lf,rf,l);
}

Formula* Insert::implform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if(lf) lf->swapsign();
	return boolform(false,lf,rf,l);
}

Formula* Insert::revimplform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if(rf) rf->swapsign();
	return boolform(false,rf,lf,l);
}

Formula* Insert::quantform(bool univ, const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	remove_vars(vv);
	if(f) {
		std::set<Variable*> pivv;
		map<Variable*,Variable*> mvv;
		for(std::set<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) {
			Variable* v = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
			pivv.insert(v);
			mvv[*it] = v;
		}
		Formula* pif = f->pi().original() ? f->pi().original()->clone(mvv) : f->clone(mvv);
		FormulaParseInfo pi = formparseinfo(new QuantForm(true,univ,pivv,pif,FormulaParseInfo()),l);
		return new QuantForm(true,univ,vv,f,pi);
	}
	else {
		for(std::set<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) delete(*it);
		delete(f);
		return 0;
	}
}

Formula* Insert::univform(const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	return quantform(true,vv,f,l);
}

Formula* Insert::existform(const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	return quantform(false,vv,f,l);
}

Formula* Insert::bexform(CompType c, int bound, const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	if(f) {
		SetExpr* se = set(vv,f,l);
		AggTerm* a = dynamic_cast<AggTerm*>(aggregate(AGG_CARD,se,l));
		Term* b = domterm(bound,l);
		AggTerm* pia = a->pi().original() ? dynamic_cast<AggTerm*>(a->pi().original()->clone()) : a->clone();
		Term* pib = b->pi().original() ? b->pi().original()->clone() : b->clone();
		FormulaParseInfo pi = formparseinfo(new AggForm(true,pib,c,pia,FormulaParseInfo()),l);
		return new AggForm(true,b,c,a,pi);
	}
	else return 0;
}

void Insert::negate(Formula* f) const {
	f->swapsign();
}


Formula* Insert::eqchain(CompType c, Formula* f, Term* t, YYLTYPE l) {
	if(f && t) {
		// TODO HIER BEZIG
	}
}






#ifdef OLD


#include <iostream>
#include <list>
#include <set>

#include "insert.hpp"
#include "namespace.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "builtin.hpp"
#include "term.hpp"
#include "options.hpp"
#include "execute.hpp"
#include "data.hpp"
#include "error.hpp"
#include "visitor.hpp"
#include "parse.h"

using namespace std;

/********************
	Sort checking
********************/

void SortChecker::visit(const PredForm* pf) {
	PFSymbol* s = pf->symb();
	for(unsigned int n = 0; n < s->nrSorts(); ++n) {
		Sort* s1 = s->sort(n);
		Sort* s2 = pf->subterm(n)->sort();
		if(s1 && s2) {
			if(!SortUtils::resolve(s1,s2,_vocab)) {
				Error::wrongsort(pf->subterm(n)->to_string(),s2->name(),s1->name(),pf->subterm(n)->pi());
			}
		}
	}
	traverse(pf);
}

void SortChecker::visit(const FuncTerm* ft) {
	Function* f = ft->func();
	for(unsigned int n = 0; n < f->arity(); ++n) {
		Sort* s1 = f->insort(n);
		Sort* s2 = ft->subterm(n)->sort();
		if(s1 && s2) {
			if(!SortUtils::resolve(s1,s2,_vocab)) {
				Error::wrongsort(ft->subterm(n)->to_string(),s2->name(),s1->name(),ft->subterm(n)->pi());
			}
		}
	}
	traverse(ft);
}

void SortChecker::visit(const EqChainForm* ef) {
	Sort* s = 0;
	unsigned int n = 0;
	while(!s && n < ef->nrSubterms()) {
		s = ef->subterm(n)->sort();
		++n;
	}
	for(; n < ef->nrSubterms(); ++n) {
		Sort* t = ef->subterm(n)->sort();
		if(t) {
			if(!SortUtils::resolve(s,t,_vocab)) {
				Error::wrongsort(ef->subterm(n)->to_string(),t->name(),s->name(),ef->subterm(n)->pi());
			}
		}
	}
	traverse(ef);
}

void SortChecker::visit(const AggTerm* at) {
	if(at->type() != AGGCARD) {
		SetExpr* s = at->set();
		if(s->nrQvars() && s->qvar(0)->sort()) {
			if(!SortUtils::resolve(s->qvar(0)->sort(),*(StdBuiltin::instance()->sort("float")->begin()),_vocab)) {
				Error::wrongsort(s->qvar(0)->name(),s->qvar(0)->sort()->name(),"int or float",s->qvar(0)->pi());
			}
		}
		for(unsigned int n = 0; n < s->nrSubterms(); ++n) {
			if(s->subterm(n)->sort() && !SortUtils::resolve(s->subterm(n)->sort(),*(StdBuiltin::instance()->sort("float")->begin()),_vocab)) {
				Error::wrongsort(s->subterm(n)->to_string(),s->subterm(n)->sort()->name(),"int or float",s->subterm(n)->pi());
			}
		}
	}
	traverse(at);
}

/**********************
	Sort derivation
**********************/

Formula* SortDeriver::traverse(Formula* f) {
	for(unsigned int n = 0; n < f->nrSubforms(); ++n)
		f->subform(n)->accept(this);
	for(unsigned int n = 0; n < f->nrSubterms(); ++n)
		f->subterm(n)->accept(this);
	return f;
}

Rule* SortDeriver::traverse(Rule* r) {
	r->head()->accept(this);
	r->body()->accept(this);
	return r;
}

Term* SortDeriver::traverse(Term* t) {
	for(unsigned int n = 0; n < t->nrSubforms(); ++n)
		t->subform(n)->accept(this);
	for(unsigned int n = 0; n < t->nrSubterms(); ++n)
		t->subterm(n)->accept(this);
	return t;
}

SetExpr* SortDeriver::traverse(SetExpr* s) {
	for(unsigned int n = 0; n < s->nrSubforms(); ++n)
		s->subform(n)->accept(this);
	for(unsigned int n = 0; n < s->nrSubterms(); ++n)
		s->subterm(n)->accept(this);
	return s;
}

Formula* SortDeriver::visit(QuantForm* qf) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
			if(!(qf->qvar(n)->sort())) _untyped[qf->qvar(n)] = set<Sort*>();
			_changed = true;
		}
	}
	return traverse(qf);
}

Formula* SortDeriver::visit(PredForm* pf) {
	PFSymbol* p = pf->symb();

	// At first visit, insert the atoms over overloaded predicates
	if(_firstvisit && p->overloaded()) {
		_overloadedatoms.insert(pf);
		_changed = true;
	}

	// Visit the children while asserting the sorts of the predicate
	for(unsigned int n = 0; n < p->nrSorts(); ++n) {
		_assertsort = p->sort(n);
		pf->subterm(n)->accept(this);
	}
	return pf;
}

Formula* SortDeriver::visit(EqChainForm* ef) {
	Sort* s = 0;
	if(!_firstvisit) {
		for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
			Sort* temp = ef->subterm(n)->sort();
			if(temp && temp->nrParents() == 0 && temp->nrChildren() == 0) {
				s = temp;
				break;
			}
		}
	}
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		_assertsort = s;
		ef->subterm(n)->accept(this);
	}
	return ef;
}

Rule* SortDeriver::visit(Rule* r) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < r->nrQvars(); ++n) {
			if(!(r->qvar(n)->sort())) _untyped[r->qvar(n)] = set<Sort*>();
			_changed = true;
		}
	}
	return traverse(r);
}

Term* SortDeriver::visit(VarTerm* vt) {
	if((!(vt->sort())) && _assertsort) {
		_untyped[vt->var()].insert(_assertsort);
	}
	return vt;
}

Term* SortDeriver::visit(DomainTerm* dt) {
	if(_firstvisit && (!(dt->sort()))) {
		_domelements.insert(dt);
	}

	if((!(dt->sort())) && _assertsort) {
		dt->sort(_assertsort);
		_changed = true;
		_domelements.erase(dt);
	}
	return dt;
}

Term* SortDeriver::visit(FuncTerm* ft) {
	Function* f = ft->func();

	// At first visit, insert the terms over overloaded functions
	if(f->overloaded()) {
		if(_firstvisit || _assertsort != _overloadedterms[ft]) {
			_changed = true;
			_overloadedterms[ft] = _assertsort;
		}
	}

	// Visit the children while asserting the sorts of the predicate
	for(unsigned int n = 0; n < f->arity(); ++n) {
		_assertsort = f->insort(n);
		ft->subterm(n)->accept(this);
	}
	return ft;
}

SetExpr* SortDeriver::visit(QuantSetExpr* qs) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < qs->nrQvars(); ++n) {
			if(!(qs->qvar(n)->sort())) {
				_untyped[qs->qvar(n)] = set<Sort*>();
				_changed = true;
			}
		}
	}
	return traverse(qs);
}

void SortDeriver::derivesorts() {
	for(map<Variable*,set<Sort*> >::iterator it = _untyped.begin(); it != _untyped.end(); ) {
		map<Variable*,set<Sort*> >::iterator jt = it; ++jt;
		if(!((it->second).empty())) {
			set<Sort*>::iterator kt = (it->second).begin(); 
			Sort* s = *kt;
			++kt;
			for(; kt != (it->second).end(); ++kt) {
				s = SortUtils::resolve(s,*kt,_vocab);
				if(!s) { // In case of conflicting sorts, assign the first sort. 
						 // Error message will be given during final check.
					s = *((it->second).begin());
					break;
				}
			}
			assert(s);
			if((it->second).size() > 1 || s->builtin()) {	// Warning when the sort was resolved or builtin
				Warning::derivevarsort(it->first->name(),s->name(),it->first->pi());
			}
			it->first->sort(s);
			_untyped.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::derivefuncs() {
	for(map<FuncTerm*,Sort*>::iterator it = _overloadedterms.begin(); it != _overloadedterms.end(); ) {
		map<FuncTerm*,Sort*>::iterator jt = it; ++jt;
		Function* f = it->first->func();
		vector<Sort*> vs(f->arity(),0);
		for(unsigned int n = 0; n < vs.size(); ++n) {
			vs[n] = it->first->subterm(n)->sort();
		}
		vs.push_back(it->second);
		Function* rf = f->disambiguate(vs,_vocab);
		if(rf) {
			it->first->func(rf);
			if(!rf->overloaded()) _overloadedterms.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::derivepreds() {
	for(set<PredForm*>::iterator it = _overloadedatoms.begin(); it != _overloadedatoms.end(); ) {
		set<PredForm*>::iterator jt = it; ++jt;
		PFSymbol* p = (*it)->symb();
		vector<Sort*> vs(p->nrSorts(),0);
		for(unsigned int n = 0; n < vs.size(); ++n) {
			vs[n] = (*it)->subterm(n)->sort();
		}
		PFSymbol* rp = p->disambiguate(vs,_vocab);
		if(rp) {
			(*it)->symb(rp);
			if(!rp->overloaded()) _overloadedatoms.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::run(Formula* f) {
	_changed = false;
	_firstvisit = true;
	f->accept(this);	// First visit: collect untyped symbols, set types of variables that occur in typed positions.
	_firstvisit = false;
	while(_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		f->accept(this);	// Next visit: type derivation over overloaded predicates or functions.
	}
	check();
}

void SortDeriver::run(Rule* r) {
	// Set the sort of the variables in the head
	for(unsigned int n = 0; n < r->head()->nrSubterms(); ++n) 
		r->head()->subterm(n)->sort(r->head()->symb()->sort(n));
	// Rest of the algorithm
	_changed = false;
	_firstvisit = true;
	r->accept(this);
	_firstvisit = false;
	while(_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		r->accept(this);
	}
	check();
}

void SortDeriver::check() {
	for(map<Variable*,set<Sort*> >::iterator it = _untyped.begin(); it != _untyped.end(); ++it) {
		assert((it->second).empty());
		Error::novarsort(it->first->name(),it->first->pi());
	}
	for(set<PredForm*>::iterator it = _overloadedatoms.begin(); it != _overloadedatoms.end(); ++it) {
		if((*it)->symb()->ispred()) Error::nopredsort((*it)->symb()->name(),(*it)->pi());
		else Error::nofuncsort((*it)->symb()->name(),(*it)->pi());
	}
	for(map<FuncTerm*,Sort*>::iterator it = _overloadedterms.begin(); it != _overloadedterms.end(); ++it) {
		Error::nofuncsort(it->first->func()->name(),it->first->pi());
	}
	for(set<DomainTerm*>::iterator it = _domelements.begin(); it != _domelements.end(); ++it) {
		Error::nodomsort((*it)->to_string(),(*it)->pi());
	}
}

/**************
	Parsing
**************/

string NSPair::to_string() {
	assert(!_name.empty());
	string str = _name[0];
	for(unsigned int n = 1; n < _name.size(); ++n) str = str + "::" + _name[n];
	if(_sortsincluded) {
		if(_arityincluded) str = str.substr(0,str.find('/'));
		str = str + '[';
		if(!_sorts.empty()) {
			if(_func && _sorts.size() == 1) str = str + ':';
			if(_sorts[0]) str = str + _sorts[0]->name();
			for(unsigned int n = 1; n < _sorts.size()-1; ++n) {
				if(_sorts[n]) str = str + ',' + _sorts[n]->name();
			}
			if(_sorts.size() > 1) {
				if(_func) str = str + ':';
				else str = str + ',';
				if(_sorts[_sorts.size()-1]) str = str + _sorts[_sorts.size()-1]->name();
			}
		}
		str = str + ']';
	}
	return str;
}

namespace Insert {

	/***********
		Data
	***********/
	
	string*					_currfile = 0;	// The current file
	vector<string*>			_allfiles;		// All the parsed files
	Namespace*				_currspace;		// The current namespace
	Vocabulary*				_currvocabulary;		// The current vocabulary
	Theory*					_currtheory;	// The current theory
	Structure*				_currstructure;	// The current structure
	InfOptions*				_curroptions;	// The current options
	LuaProcedure*			_currproc;		// The current procedure

	vector<Vocabulary*>		_usingvocab;	// The vocabularies currently used to parse
	vector<unsigned int>	_nrvocabs;		// The number of using vocabulary statements in the current block

	vector<Namespace*>		_usingspace;	// The namespaces currently used to parse
	vector<unsigned int>	_nrspaces;		// The number of using namespace statements in the current block

	void closeblock() {
	}


	string*		currfile()					{ return _currfile;	}
	void		currfile(const string& s)	{ _allfiles.push_back(_currfile); _currfile = new string(s);	}
	void		currfile(string* s)			{ _allfiles.push_back(_currfile); if(s) _currfile = new string(*s); else _currfile = 0;	}
	FormulaParseInfo	formparseinfo(Formula* f, YYLTYPE l)	{ return FormulaParseInfo(l.first_line,l.first_column,_currfile,f);	}
	FormulaParseInfo	formparseinfo(YYLTYPE l)	{ return FormulaParseInfo(l.first_line,l.first_column,_currfile,0);	}

	// Three-valued interpretations
	enum UTF { UTF_UNKNOWN, UTF_CT, UTF_CF, UTF_ERROR };
	string _utf2string[4] = { "u", "ct", "cf", "error" };

	map<Predicate*,FinitePredTable*>	_unknownpredtables;
	map<Function*,FinitePredTable*>		_unknownfunctables;

	
	/******************************************
		Convert vector of names to one name
	******************************************/

	/*****************
		Find names
	*****************/

	bool belongsToVoc(Sort* s) {
		if(_currvocabulary->contains(s)) return true;
		return false;
	}

	bool belongsToVoc(Predicate* p) {
		if(_currvocabulary->contains(p)) return true;
		return false;
	}

	bool belongsToVoc(Function* f) {
		if(_currvocabulary->contains(f)) return true;
		return false;
	}

	Namespace* namespaceInScope(const string& name, const ParseInfo& pi) {
		Namespace* ns = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isSubspace(name)) {
				if(ns) Error::overloadedspace(name,_usingspace[n]->pi(),ns->pi(),pi);
				else ns = _usingspace[n]->subspace(name);
			}
		}
		return ns;
	}

	AbstractTheory* theoInScope(const string& name, const ParseInfo& pi) {
		AbstractTheory* th = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isTheory(name)) {
				if(th) Error::overloadedtheory(name,_usingspace[n]->theory(name)->pi(),th->pi(),pi);
				else th = _usingspace[n]->theory(name);
			}
		}
		return th;
	}

	Vocabulary* vocabInScope(const string& name, const ParseInfo& pi) {
		Vocabulary* v = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isVocab(name)) {
				if(v) Error::overloadedvocab(name,_usingspace[n]->vocabulary(name)->pi(),v->pi(),pi);
				else v = _usingspace[n]->vocabulary(name);
			}
		}
		return v;
	}

	AbstractStructure* structInScope(const string& name, const ParseInfo& pi) {
		AbstractStructure* s = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isStructure(name)) {
				if(s) Error::overloadedstructure(name,_usingspace[n]->structure(name)->pi(),s->pi(),pi);
				else s = _usingspace[n]->structure(name);
			}
		}
		return s;
	}

	InfOptions* optionsInScope(const string& name, const ParseInfo& pi) {
		InfOptions* opt = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isOption(name)) {
				if(opt) Error::overloadedopt(name,_usingspace[n]->option(name)->_pi,opt->_pi,pi);
				else opt = _usingspace[n]->option(name);
			}
		}
		return opt;
	}

	LuaProcedure* procInScope(const string& name, const ParseInfo& pi) {
		LuaProcedure* lp = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isProc(name)) {
				if(lp) Error::overloadedproc(name,_usingspace[n]->procedure(name)->pi(),lp->pi(),pi);
				else lp = _usingspace[n]->procedure(name);
			}
		}
		return lp;
	}

	Namespace* namespaceInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return namespaceInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			for(unsigned int n = 1; n < vs.size(); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			return ns;
		}
	}

	Vocabulary* vocabInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return vocabInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			if(ns) {
				for(unsigned int n = 1; n < (vs.size()-1); ++n) {
					if(ns->isSubspace(vs[n])) {
						ns = ns->subspace(vs[n]);
					}
					else return 0;
				}
				if(ns->isVocab(vs.back())) {
					return ns->vocabulary(vs.back());
				}
				else return 0;
			}
			else return 0;
		}
	}

	AbstractStructure* structInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return structInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			for(unsigned int n = 1; n < (vs.size()-1); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			if(ns->isStructure(vs.back())) {
				return ns->structure(vs.back());
			}
			else return 0;
		}
	}

	InfOptions* optionsInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return optionsInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			if(ns) {
				for(unsigned int n = 1; n < (vs.size()-1); ++n) {
					if(ns->isSubspace(vs[n])) {
						ns = ns->subspace(vs[n]);
					}
					else return 0;
				}
				if(ns->isOption(vs.back())) {
					return ns->option(vs.back());
				}
				else return 0;
			}
			else return 0;
		}
	}

	LuaProcedure* procInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return procInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			for(unsigned int n = 1; n < (vs.size()-1); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			if(ns->isProc(vs.back())) {
				return ns->procedure(vs.back());
			}
			else return 0;
		}
	}

	Sort* sortInScope(const string& name, const ParseInfo& pi) {
		Sort* s = 0;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			const std::set<Sort*>* temp = _usingvocab[n]->sort(name);
			if(temp) {
				if(s) {
					Error::overloadedsort(s->name(),s->pi(),(*(temp->begin()))->pi(),pi);
				}
				else if(temp->size() > 1) {
					std::set<Sort*>::iterator it = temp->begin();
					Sort* s1 = *it; ++it; Sort* s2 = *it;
					Error::overloadedsort(s1->name(),s1->pi(),s2->pi(),pi);
				}
				else s = *(temp->begin());
			}
		}
		return s;
	}

	Sort* sortInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return sortInScope(vs[0],pi);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) {
				const std::set<Sort*>* ss = v->sort(vs.back());
				if(ss) {
					if(ss->size() > 1) {
						std::set<Sort*>::iterator it = ss->begin();
						Sort* s1 = *it; ++it; Sort* s2 = *it;
						Error::overloadedsort(s1->name(),s1->pi(),s2->pi(),pi);
					}
					return *(ss->begin());
				}
				else return 0;
			}
			else return 0;
		}
	}

	Predicate* predInScope(const string& name) {
		vector<Predicate*> vp;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			Predicate* p = _usingvocab[n]->pred(name);
			if(p) vp.push_back(p);
		}
		if(vp.empty()) return 0;
		else return PredUtils::overload(vp);
	}

	Predicate* predInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return predInScope(vs[0]);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) return v->pred(vs.back());
			else return 0;
		}
	}

	vector<Predicate*> noArPredInScope(const string& name) {
		vector<Predicate*> vp;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			vector<Predicate*> nvp = _usingvocab[n]->pred_no_arity(name);
			for(unsigned int m = 0; m < nvp.size(); ++m) vp.push_back(nvp[m]);
		}
		return vp;
	}

	vector<Predicate*> noArPredInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return noArPredInScope(vs[0]);
		}
		else {
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) return v->pred_no_arity(vs.back());
			else return vector<Predicate*>(0);
		}
	}

	Function* funcInScope(const string& name) {
		vector<Function*> vf;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			Function* f = _usingvocab[n]->func(name);
			if(f) vf.push_back(f);
		}
		if(vf.empty()) return 0;
		else return FuncUtils::overload(vf);
	}

	Function* funcInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return funcInScope(vs[0]);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) return v->func(vs.back());
			else return 0;
		}
	}

	vector<Function*> noArFuncInScope(const string& name) {
		vector<Function*> vf;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			vector<Function*> nvf = _usingvocab[n]->func_no_arity(name);
			for(unsigned int m = 0; m < nvf.size(); ++m) vf.push_back(nvf[m]);
		}
		return vf;
	}


	vector<Function*> noArFuncInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return noArFuncInScope(vs[0]);
		}
		else {
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) return v->func_no_arity(vs.back());
			else return vector<Function*>(0);
		}
	}


	/******************************************************************* 
		Data structures and methods to link variables during parsing 
	*******************************************************************/


	Variable* getVar(const string& name) {
		for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
			if(name == i->_name) return i->_var;
		}
		return 0;
	}


	/***********************
		Global structure
	***********************/

	void initialize() {

		_currspace = Namespace::global();
		_nrspaces.push_back(1);
		_usingspace.push_back(_currspace);

	}

	void cleanup() {
		for(unsigned int n = 0; n < _allfiles.size(); ++n) {
			if(_allfiles[n]) delete(_allfiles[n]);
		}
		if(_currfile) delete(_currfile);
	}

	/*************
		Options
	*************/
	
	void openoptions(const string& name, YYLTYPE l) {
		Info::print("Parsing options " + name);
		ParseInfo pi = parseinfo(l);
		InfOptions* opt = optionsInScope(name,pi);
		if(opt) Error::multdeclopt(name,pi,opt->_pi);
		_curroptions = new InfOptions(name,pi);
		_currspace->add(_curroptions);
		_nrvocabs.push_back(0);
		_nrspaces.push_back(0);
	}

	void closeoptions() {
		closeblock();
	}

	void externoption(const vector<string>& name, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		InfOptions* opt = optionsInScope(name,pi);
		if(opt) _curroptions->set(opt);
		else Error::undeclopt(oneName(name),pi);
	}

	void option(const string& opt, const string& val,YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		_curroptions->set(opt,val,&pi);
	}
	void option(const string& opt, double val,YYLTYPE l) { 
		ParseInfo pi = parseinfo(l);
		_curroptions->set(opt,val,&pi);
	}
	void option(const string& opt, int val,YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		_curroptions->set(opt,val,&pi);
	}

	/*****************
		Procedures
	*****************/

	void openexec() {
		_currproc = new LuaProcedure();
	}

	LuaProcedure* currproc() {
		LuaProcedure* r = _currproc;
		_currproc = 0;
		return r;
	}

	void openproc(const string& name, YYLTYPE l) {
		Info::print("Parsing procedure " + name);
		ParseInfo pi = parseinfo(l);
		_currproc = new LuaProcedure(name,pi);
		_nrvocabs.push_back(0);
		_nrspaces.push_back(0);
		_currproc->add(string("function ("));
	}

	void closeproc() {
		LuaProcedure* lp = procInScope(_currproc->name(),_currproc->pi());
		if(lp) Error::multdeclproc(_currproc->name(),_currproc->pi(),lp->pi());
		_currproc->add(string("end"));
		_currspace->add(_currproc);
		closeblock();
	}

	void procarg(const string& name) {
		if(_currproc->arity()) _currproc->add(string(","));
		_currproc->add(name);
		_currproc->addarg(name);
	}

	void luacloseargs() {
		_currproc->add(string(")"));
		// include using namespace statements
		for(unsigned int n = 1; n < _usingspace.size(); ++n) {	// n=1 to skip over the global namespace
			Namespace* ns = _usingspace[n];
			vector<string> fn = ns->fullnamevector();
			stringstream common;
			for(unsigned int m = 0; m < fn.size(); ++m) {
				common << fn[m] << ".";
			}
			string comstr = common.str();
			stringstream toadd;
			for(unsigned int m = 0; m < ns->nrSubs(); ++m) {
				toadd << "local " << ns->subspace(m)->name() << " = " << comstr << ns->subspace(m)->name() << "\n";
			}
			for(unsigned int m = 0; m < ns->nrVocs(); ++m) {
				toadd << "local " << ns->vocabulary(m)->name() << " = " << comstr << ns->vocabulary(m)->name() << "\n";
			}
			for(unsigned int m = 0; m < ns->nrStructs(); ++m) {
				toadd << "local " << ns->structure(m)->name() << " = " << comstr << ns->structure(m)->name() << "\n";
			}
			for(unsigned int m = 0; m < ns->nrTheos(); ++m) {
				toadd << "local " << ns->theory(m)->name() << " = " << comstr << ns->theory(m)->name() << "\n";
			}
			for(unsigned int m = 0; m < ns->nrProcs(); ++m) {
				toadd << "local " << ns->procedure(m)->name() << " = " << comstr << ns->procedure(m)->name() << "\n";
			}
		}
		// include using vocabulary statements
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {	
			// TODO
			cerr << "WARNING: using vocabulary statements not yet supported in lua\n";
		}
	}

	void luacode(char* s) {
		_currproc->add(s);
	}

	void luacode(const string& s) {
		_currproc->add(s);
	}

	void luacode(const vector<string>& vs) {
		assert(!vs.empty());
		_currproc->add(vs[0]);
		for(unsigned int n = 1; n < vs.size(); ++n) {
			_currproc->add(string("."));
			_currproc->add(vs[n]);
		}
	}

	/*******************
		Vocabularies
	*******************/

	/** Open and close vocabularies **/


	/** Pointers to symbols **/

	Sort* theosortpointer(const vector<string>& vs, YYLTYPE l) {
		Sort* s = sortpointer(vs,l);
		if(s) {
			if(belongsToVoc(s)) {
				return s;
			}
			else {
				ParseInfo pi = parseinfo(l);
				string uname = oneName(vs);
				if(_currtheory) Error::sortnotintheovoc(uname,_currtheory->name(),pi);
				else if(_currstructure) Error::sortnotinstructvoc(uname,_currstructure->name(),pi);
				return 0;
			}
		}
		else return 0;
	}

	/** Add symbols to the current vocabulary **/



	/*****************
		Structures
	*****************/

	void openstructure(const string& sname, YYLTYPE l) {
		Info::print("Parsing structure " + sname);
		ParseInfo pi = parseinfo(l);
		AbstractStructure* s = structInScope(sname,pi);
		if(s) {
			Error::multdeclstruct(sname,pi,s->pi());
		}
		_currstructure = new Structure(sname,pi);
		_currspace->add(_currstructure);
		_nrvocabs.push_back(0);
		_nrspaces.push_back(0);
	}

	void assignunknowntables() {
		// Assign the unknown predicate interpretations
		for(map<Predicate*,FinitePredTable*>::iterator it = _unknownpredtables.begin(); it != _unknownpredtables.end(); ++it) {
			PredInter* pt = _currstructure->inter(it->first);
			if(pt) {
				if(pt->ctpf()) {
					if(pt->cfpt()) {
						Error::threethreepred(it->first->name(),_currstructure->name());
						delete(it->second);
					}
					else {
						assert(pt->ct()); 
						assert(pt->ctpf()->finite());
						FinitePredTable* fpt = it->second;
						for(unsigned int n = 0; n < pt->ctpf()->size(); ++n) {
							fpt->addRow(pt->ctpf()->tuple(n),pt->ctpf()->types());
						}
						fpt->sortunique();
						pt->replace(fpt,false,false);
					}
				}
				else {
					assert(pt->cfpt()); 
					assert(pt->cf()); 
					assert(pt->cfpt()->finite());
					FinitePredTable* fpt = it->second;
					for(unsigned int n = 0; n < pt->cfpt()->size(); ++n) {
						fpt->addRow(pt->cfpt()->tuple(n),pt->cfpt()->types());
					}
					fpt->sortunique();
					pt->replace(fpt,true,false);
				}
			}
			else {
				Error::onethreepred(it->first->name(),_currstructure->name());
				delete(it->second);
			}
		}
		// Assign the unknown function interpretations
		for(map<Function*,FinitePredTable*>::iterator it = _unknownfunctables.begin(); it != _unknownfunctables.end(); ++it) {
			FuncInter* ft = _currstructure->inter(it->first);
			PredInter* pt = 0;
			if(ft) pt = ft->predinter();
			if(pt) {
				if(pt->ctpf()) {
					if(pt->cfpt()) {
						Error::threethreefunc(it->first->name(),_currstructure->name());
						delete(it->second);
					}
					else {
						assert(pt->ct()); 
						assert(pt->ctpf()->finite());
						FinitePredTable* fpt = it->second;
						for(unsigned int n = 0; n < pt->ctpf()->size(); ++n) {
							fpt->addRow(pt->ctpf()->tuple(n),pt->ctpf()->types());
						}
						fpt->sortunique();
						pt->replace(fpt,false,false);
					}
				}
				else {
					assert(pt->cfpt()); 
					assert(pt->cf()); 
					assert(pt->cfpt()->finite());
					FinitePredTable* fpt = it->second;
					for(unsigned int n = 0; n < pt->cfpt()->size(); ++n) {
						fpt->addRow(pt->cfpt()->tuple(n),pt->cfpt()->types());
					}
					fpt->sortunique();
					pt->replace(fpt,true,false);
				}
			}
			else {
				Error::onethreefunc(it->first->name(),_currstructure->name());
				delete(it->second);
			}
		}
	}

	void closestructure() {
		assignunknowntables();
		_currstructure->autocomplete();
		_currstructure->functioncheck();
		_unknownpredtables.clear();
		_unknownfunctables.clear();
		closeblock();
	}

	void closeaspstructure() {
/*		for(unsigned int n = 0; n < _currstructure->nrSortInters(); ++n) {
			SortTable* st = _currstructure->sortinter(n);
			if(st) st->sortunique();
		}
		for(unsigned int n = 0; n < _currstructure->nrPredInters(); ++n) {
			PredInter* pt = _currstructure->predinter(n);
			if(!pt) {
				Predicate* p = _currstructure->vocabulary()->nbpred(n);
				pt = TableUtils::leastPredInter(p->arity());
				_currstructure->inter(p,pt);
			}
			if(pt->cfpt()) {
				// TODO: warning?
				delete(pt->cfpt());
				pt->replace(pt->ctpf(),false,false);
			}
			pt->sortunique();
		}
		for(unsigned int n = 0; n < _currstructure->nrFuncInters(); ++n) {
			FuncInter* ft = _currstructure->funcinter(n);
			if(!ft) {
				Function* f = _currstructure->vocabulary()->nbfunc(n);
				ft = TableUtils::leastFuncInter(f->arity()+1);
				_currstructure->inter(f,ft);
			}
			if(ft->predinter()->cfpt()) {
				// TODO: warning?
				delete(ft->predinter()->cfpt());
				ft->predinter()->replace(ft->predinter()->ctpf(),false,false);
			}
			ft->sortunique();
		} */
		_currstructure->sortall();
		Structure* tmp = _currstructure;
		closestructure();
		tmp->forcetwovalued();
	}

	void closeaspbelief() {
/*		for(unsigned int n = 0; n < _currstructure->nrSortInters(); ++n) {
			SortTable* st = _currstructure->sortinter(n);
			if(st) st->sortunique();
		}
		for(unsigned int n = 0; n < _currstructure->nrPredInters(); ++n) {
			PredInter* pt = _currstructure->predinter(n);
			if(pt) pt->sortunique();
		}
		for(unsigned int n = 0; n < _currstructure->nrFuncInters(); ++n) {
			FuncInter* ft = _currstructure->funcinter(n);
			if(ft) ft->sortunique();
		} */
		_currstructure->sortall();
		closestructure();
	}

	/** Two-valued interpretations **/

	void sortinter(NSPair* nst, FiniteSortTable* t) {
		ParseInfo pi = nst->_pi;
		Sort* s = sortInScope(nst->_name,pi);
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != 1) Error::incompatiblearity(nst->to_string(),pi);
			if(nst->_func) Error::prednameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + "/1";
		Predicate* p = predInScope(nst->_name,pi);
		if(p && nst->_sortsincluded && (nst->_sorts).size() == 1) p = p->resolve(nst->_sorts);
		if(s) {
			assert(p);
			t->sortunique();
			PredInter* i = new PredInter(t,true);
			_currstructure->inter(s,t);
			_currstructure->inter(p,i);
		}
		else if(p) {
			t->sortunique();
			PredInter* i = new PredInter(t,true);
			_currstructure->inter(p,i);
		}
		else Error::prednotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		delete(nst);
	}

	void predinter(NSPair* nst, FinitePredTable* t) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->to_string(),pi);
			if(nst->_func) Error::prednameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(t->arity());
		Predicate* p = predInScope(nst->_name,pi);
		if(p && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) p = p->resolve(nst->_sorts);
		if(p) {
			if(belongsToVoc(p)) {
				if(_currstructure->hasInter(p)) Error::multpredinter(nst->to_string(),pi);
				else {
					t->sortunique();
					PredInter* pt = new PredInter(t,true);
					_currstructure->inter(p,pt);
				}
			}
			else Error::prednotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclpred(nst->to_string(),pi);
		delete(nst);
	}


	void funcinter(NSPair* nst, FinitePredTable* t) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->to_string(),pi);
			if(!(nst->_func)) Error::funcnameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(t->arity()-1);
		Function* f = funcInScope(nst->_name,pi);
		if(f && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				if(_currstructure->hasInter(f)) Error::multfuncinter(f->name(),pi);
				else {
					t->sortunique();
					PredInter* pt = new PredInter(t,true);
					FiniteFuncTable* fft = new FiniteFuncTable(t);
					FuncInter* ft = new FuncInter(fft,pt);
					_currstructure->inter(f,ft);
				}
			}
			else Error::funcnotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclfunc(nst->to_string(),pi);
	}

	void truepredinter(NSPair* nst) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		upt->addRow();
		predinter(nst,upt);
	}

	void falsepredinter(NSPair* nst) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		predinter(nst,upt);
	}

	void emptyinter(NSPair* nst) {
		if(nst->_sortsincluded) {
			FinitePredTable* upt = new FinitePredTable(vector<ElementType>((nst->_sorts).size(),ELINT));
			if(nst->_func) funcinter(nst,upt);
			else predinter(nst,upt);
		}
		else {
			ParseInfo pi = nst->_pi;
			vector<Predicate*> vp = noArPredInScope(nst->_name,pi);
			if(vp.empty()) Error::undeclpred(nst->to_string(),pi);
			else if(vp.size() > 1) {
				Error::overloadedpred(nst->to_string(),vp[0]->pi(),vp[1]->pi(),pi);
			}
			else {
				FinitePredTable* upt = new FinitePredTable(vector<ElementType>((vp[0]->arity(),ELINT)));
				predinter(nst,upt);
			}
		}
	}

	/** Three-valued interpretations **/

	UTF getUTF(const string& utf, const ParseInfo& pi) {
		if(utf == "u") return UTF_UNKNOWN;
		else if(utf == "ct") return UTF_CT;
		else if(utf == "cf") return UTF_CF;
		else {
			Error::expectedutf(utf,pi);
			return UTF_ERROR;
		}
	}

	void threepredinter(NSPair* nst, const string& utf, FinitePredTable* t) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->to_string(),pi);
			if(nst->_func) Error::prednameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(t->arity());
		Predicate* p = predInScope(nst->_name,pi);
		if(p && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) p = p->resolve(nst->_sorts);
		if(p) {
			if(p->arity() == 1 && p->sort(0)->pred() == p) {
				Error::threevalsort(p->name(),pi);
			}
			else {
				if(belongsToVoc(p)) {
					t->sortunique();
					switch(getUTF(utf,pi)) {
						case UTF_UNKNOWN:
							if(_unknownpredtables.find(p) == _unknownpredtables.end() && !(p->builtin()))  _unknownpredtables[p] = t;
							else Error::multunknpredinter(p->name(),pi);
							break;
						case UTF_CT:
						{	
							bool hasinter = _currstructure->hasInter(p);
							if(hasinter) {
								PredInter* pt = _currstructure->inter(p);
								if(pt->ctpf()) Error::multctpredinter(p->name(),pi);
								else pt->replace(t,true,true);
							}
							else {
								PredInter* pt = new PredInter(t,0,true,true);
								_currstructure->inter(p,pt);
							}
							break;
						}
						case UTF_CF:
						{
							bool hasinter = _currstructure->hasInter(p);
							if(hasinter) {
								PredInter* pt = _currstructure->inter(p);
								if(pt->cfpt()) Error::multcfpredinter(p->name(),pi);
								else pt->replace(t,false,true);
							}
							else {
								PredInter* pt = new PredInter(0,t,true,true);
								_currstructure->inter(p,pt);
							}
							break;
						}
						case UTF_ERROR:
							break;
						default:
							assert(false);
					}
				}
				else Error::prednotinstructvoc(nst->to_string(),_currstructure->name(),pi);
			}
		}
		else Error::undeclpred(nst->to_string(),pi);
	}

	void truethreepredinter(NSPair* nst, const string& utf) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		upt->addRow();
		threepredinter(nst,utf,upt);
	}

	void falsethreepredinter(NSPair* nst, const string& utf) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		threepredinter(nst,utf,upt);
	}

	void threefuncinter(NSPair* nst, const string& utf, FinitePredTable* t) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->to_string(),pi);
			if(!(nst->_func)) Error::funcnameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(t->arity()-1);
		Function* f = funcInScope(nst->_name,pi);
		if(f && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				t->sortunique();
				switch(getUTF(utf,pi)) {
					case UTF_UNKNOWN:
						if(_unknownfunctables.find(f) == _unknownfunctables.end() && !(f->builtin()))  _unknownfunctables[f] = t;
						else Error::multunknfuncinter(f->name(),pi);
						break;
					case UTF_CT:
					{	
						bool hasinter = _currstructure->hasInter(f);
						if(hasinter) {
							FuncInter* ft = _currstructure->inter(f);
							if(ft->predinter()->ctpf()) Error::multctfuncinter(f->name(),pi);
							else ft->predinter()->replace(t,true,true);
						}
						else {
							PredInter* pt = new PredInter(t,0,true,true);
							FuncInter* ft = new FuncInter(0,pt);
							_currstructure->inter(f,ft);
						}
						break;
					}
					case UTF_CF:
					{
						bool hasinter = _currstructure->hasInter(f);
						if(hasinter) {
							FuncInter* ft = _currstructure->inter(f);
							if(ft->predinter()->cfpt()) Error::multcffuncinter(f->name(),pi);
							else ft->predinter()->replace(t,false,true);
						}
						else {
							PredInter* pt = new PredInter(0,t,true,true);
							FuncInter* ft = new FuncInter(0,pt);
							_currstructure->inter(f,ft);
						}
						break;
					}
					case UTF_ERROR:
						break;
					default:
						assert(false);
				}
			}
			else Error::funcnotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclfunc(nst->to_string(),pi);
	}

	void threeinter(NSPair* nst, const string& utf, FiniteSortTable* t) {
		FinitePredTable* spt = new FinitePredTable(*t);
		if(nst->_func) threefuncinter(nst,utf,spt);
		else threepredinter(nst,utf,spt);
	}

	void emptythreeinter(NSPair* nst, const string& utf) {
		if(nst->_sortsincluded) {
			FinitePredTable* upt = new FinitePredTable(vector<ElementType>((nst->_sorts).size(),ELINT));
			if(nst->_func) threefuncinter(nst,utf,upt);
			else threepredinter(nst,utf,upt);
		}
		else {
			ParseInfo pi = nst->_pi;
			vector<Predicate*> vp = noArPredInScope(nst->_name,pi);
			if(vp.empty()) Error::undeclpred(nst->to_string(),pi);
			else if(vp.size() > 1) {
				Error::overloadedpred(nst->to_string(),vp[0]->pi(),vp[1]->pi(),pi);
			}
			else {
				FinitePredTable* upt = new FinitePredTable(vector<ElementType>((vp[0]->arity(),ELINT)));
				threepredinter(nst,utf,upt);
			}
		}
	}

	/** ASP atoms **/

	void predatom(Predicate* p, const vector<ElementType>& vet, const vector<Element>& ve, bool ct) {
		FinitePredTable* pt;
		if(ct) pt = dynamic_cast<FinitePredTable*>(_currstructure->inter(p)->ctpf());
		else pt = dynamic_cast<FinitePredTable*>(_currstructure->inter(p)->cfpt());
		pt->addRow(ve,vet);
	}

	void predatom(Predicate* p, ElementType t, Element e, bool ct) {
		PredInter* pt = _currstructure->inter(p);
		if(pt) {
			FiniteSortTable* spt;
			if(ct) spt = dynamic_cast<FiniteSortTable*>(pt->ctpf());
			else spt = dynamic_cast<FiniteSortTable*>(pt->cfpt());
			FiniteSortTable* ust = 0;
			switch(t) {
				case ELINT:
					ust = spt->add(e._int); break;
				case ELDOUBLE:
					ust = spt->add(e._double); break;
				case ELSTRING:
					ust = spt->add(e._string); break;
				case ELCOMPOUND:
					ust = spt->add(e._compound); break;
				default:
					assert(false);
			}
			if(ust != spt) {
				delete(spt);
				pt->replace(ust,ct,true);
			}
		}
		else {
			FiniteSortTable* ust = 0;
			FiniteSortTable* ost = new IntSortTable();
			switch(t) {
				case ELINT: ust = new IntSortTable(); ust->add(e._int); break;
				case ELDOUBLE: ust = new FloatSortTable(); ust->add(e._double); break;
				case ELSTRING: ust = new StrSortTable(); ust->add(e._string); break;
				case ELCOMPOUND: ust = new MixedSortTable(); ust->add(e._compound); break;
				default: assert(false);
			}
			if(ct) pt = new PredInter(ust,ost,true,true);
			else pt = new PredInter(ost,ust,true,true);
			_currstructure->inter(p,pt);
		}
	}

	void predatom(Predicate* p, FiniteSortTable* st, bool ct) {
		PredInter* pt = _currstructure->inter(p);
		if(!pt) {
			FiniteSortTable* ost = new IntSortTable();
			if(ct) pt = new PredInter(st,ost,true,true);
			else pt = new PredInter(ost,st,true,true);
			_currstructure->inter(p,pt);
		}
		else {
			switch(st->type()) {
				case ELINT: {
					RanSortTable* rst = dynamic_cast<RanSortTable*>(st);
					for(unsigned int n = 0; n < rst->size(); ++n) {
						Element e; e._int = (*rst)[n];
						predatom(p,ELINT,e,ct);
					}
					break;
				}
				case ELSTRING: {
					StrSortTable* sst = dynamic_cast<StrSortTable*>(st);
					for(unsigned int n = 0; n < sst->size(); ++n) {
						Element e; e._string = (*sst)[n];
						predatom(p,ELSTRING,e,ct);
					}
					break;
				}
				default: assert(false);
			}
			delete(st);
		}
	}

	void predatom(Predicate* p, const vector<ElementType>& vet, const vector<Element>& ve, const vector<FiniteSortTable*>& vst, const vector<bool>& vb, bool ct) {

		vector<ElementType> vet2(vb.size());
		unsigned int skipcounter = 0;
		for(unsigned int n = 0; n < vb.size(); ++n) {
			if(vb[n]) vet2[n] = vet[n-skipcounter];
			else {
				vet2[n] = vst[skipcounter]->type();
				++skipcounter;
			}
		}

		vector<Element> ve2(vb.size());
		vector<SortTable*> vst2; for(unsigned int n = 0; n < vst.size(); ++n) vst2.push_back(vst[n]);
		SortTableTupleIterator stti(vst2);
		if(!stti.empty()) {
			do {
				unsigned int skip = 0;
				for(unsigned int n = 0; n < vb.size(); ++n) {
					if(vb[n]) ve2[n] = ve[n-skip];
					else {
						ve2[n] = stti.value(skip);
						++skip;
					}
				}
				predatom(p,vet2,ve2,ct);
			} while(stti.nextvalue());
		}

		for(unsigned int n = 0; n < vst.size(); ++n) {
			delete(vst[n]);
		}
	}

	void predatom(NSPair* nst, const vector<Element>& ve, const vector<FiniteSortTable*>& vt, const vector<ElementType>& vet, const vector<bool>& vb,bool ct) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != vb.size()) Error::incompatiblearity(nst->to_string(),pi);
			if(nst->_func) Error::prednameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(vb.size());
		Predicate* p = predInScope(nst->_name,pi);
		if(p && nst->_sortsincluded && (nst->_sorts).size() == vb.size()) p = p->resolve(nst->_sorts);
		if(p) {
			if(belongsToVoc(p)) {
				if(p->arity() == 1) {
					if(vt.empty()) predatom(p,vet[0],ve[0],ct);
					else predatom(p,vt[0],ct);
				}
				else {
					PredInter* pt = _currstructure->inter(p);
					if(!pt) {
						vector<ElementType> vet2(vb.size(),ELINT);
						FinitePredTable* ctt = new FinitePredTable(vet2);
						FinitePredTable* cft = new FinitePredTable(vet2);
						pt = new PredInter(ctt,cft,true,true);
						_currstructure->inter(p,pt);
					}
					if(vt.empty()) predatom(p,vet,ve,ct);
					else predatom(p,vet,ve,vt,vb,ct);
				}
			}
			else Error::prednotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclpred(nst->to_string(),pi);
		delete(nst);
	}

	void funcatom(Function* f, const vector<ElementType>& vet, const vector<Element>& ve, bool ct) {
		FuncInter* ft = _currstructure->inter(f);
		FinitePredTable* pt;
		if(ct) pt = dynamic_cast<FinitePredTable*>(ft->predinter()->ctpf());
		else pt = dynamic_cast<FinitePredTable*>(ft->predinter()->cfpt());
		pt->addRow(ve,vet);
	}

	void funcatom(Function* f, const vector<ElementType>& vet, const vector<Element>& ve, const vector<FiniteSortTable*>& vst, const vector<bool>& vb, bool ct) {
		vector<ElementType> vet2(vb.size());
		unsigned int skipcounter = 0;
		for(unsigned int n = 0; n < vb.size(); ++n) {
			if(vb[n]) vet2[n] = vet[n-skipcounter];
			else {
				vet2[n] = vst[skipcounter]->type();
				++skipcounter;
			}
		}

		vector<Element> ve2(vb.size());
		vector<SortTable*> vst2; for(unsigned int n = 0; n < vst.size(); ++n) vst2.push_back(vst[n]);
		SortTableTupleIterator stti(vst2);
		if(!stti.empty()) {
			do {
				unsigned int skip = 0;
				for(unsigned int n = 0; n < vb.size(); ++n) {
					if(vb[n]) ve2[n] = ve[n-skip];
					else {
						ve2[n] = stti.value(skip);
						++skip;
					}
				}
				funcatom(f,vet2,ve2,ct);
			} while(stti.nextvalue());
		}

		for(unsigned int n = 0; n < vst.size(); ++n) {
			delete(vst[n]);
		}
	}

	void funcatom(NSPair* nst, const vector<Element>& ve, const vector<FiniteSortTable*>& vt, const vector<ElementType>& vet, const vector<bool>& vb,bool ct) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != vb.size()) Error::incompatiblearity(nst->to_string(),pi);
			if(!nst->_func) Error::funcnameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(vb.size()-1);
		Function* f = funcInScope(nst->_name,pi);
		if(f && nst->_sortsincluded && (nst->_sorts).size() == vb.size()) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				FuncInter* ft = _currstructure->inter(f);
				if(!ft) {
					vector<ElementType> vet2(vb.size(),ELINT);
					FinitePredTable* ctt = new FinitePredTable(vet2);
					FinitePredTable* cft = new FinitePredTable(vet2);
					PredInter* pt = new PredInter(ctt,cft,true,true);
					ft = new FuncInter(0,pt);
					_currstructure->inter(f,ft);
				}
				if(vt.empty()) funcatom(f,vet,ve,ct);
				else funcatom(f,vet,ve,vt,vb,ct);
			}
			else Error::funcnotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclfunc(nst->to_string(),pi);
		delete(nst);
	}


	/** Compound domain elements **/

	compound* makecompound(NSPair* nst, const vector<TypedElement*>& vte) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != vte.size()+1) Error::incompatiblearity(nst->to_string(),pi);
			if(!(nst->_func)) Error::funcnameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(vte.size());
		Function* f = funcInScope(nst->_name,pi);
		compound* c = 0;
		if(f && nst->_sortsincluded && (nst->_sorts).size() == vte.size()+1) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				if(f->constructor()) {
					vector<TypedElement> nvte;
					for(unsigned int n = 0; n < vte.size(); ++n) {
						if(vte[n]) {
							nvte.push_back(*(vte[n]));
							delete((vte[n]));
						}
						else return 0;
					}
					c = CPPointer(f,nvte);
				}
				else Error::funcnotconstr(nst->to_string(),pi);
			}
			else Error::funcnotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclfunc(nst->to_string(),pi);
		return c;
	}

	compound* makecompound(NSPair* nst) {
		vector<TypedElement*> vte(0);
		return makecompound(nst,vte);
	}
	
	/***************
		Theories
	***************/

	PredForm* bexform(char c, bool b, int n, const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			FormulaParseInfo pi = formparseinfo(l);
			QuantSetExpr* qse = new QuantSetExpr(vv,f,pi);
			vector<Term*> vt(2);
			Element en; en._int = n;
			vt[1] = new DomainTerm(*(StdBuiltin::instance()->sort("int")->begin()),ELINT,en,pi);
			vt[0] = new AggTerm(qse,AGGCARD,pi);
			Predicate* p = StdBuiltin::instance()->pred(string(1,c) + "/2");
			return new PredForm(b,p,vt,pi);	// TODO adapt formparseinfo
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			delete(f);
			return 0;
		}
	}

	FixpDef* fixpdef(bool lfp, const vector<pair<Rule*,FixpDef*> >& vp) {
		FixpDef* d = new FixpDef(lfp);
		for(unsigned int n = 0; n < vp.size(); ++n) {
			if(vp[n].first) {
				d->add(vp[n].first);
			}
			else if(vp[n].second) {
				d->add(vp[n].second);
			}
		}
		// TODO: check if the fixpoint definition satisfies the restrictions of FO(FD)
		return d;
	}

	Variable* quantifiedvar(const string& name, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Variable* v = new Variable(name,0,pi);
		curr_vars.push_front(VarName(name,v));
		return v;
	}

	Variable* quantifiedvar(const string& name, Sort* sort, YYLTYPE l) {
		Variable* v = quantifiedvar(name,l);
		if(sort) v->sort(sort);
		return v;
	}

	FuncTerm* functerm(NSPair* nst, const vector<Term*>& vt) {
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != vt.size()+1) Error::incompatiblearity(nst->to_string(),nst->_pi);
			if(!nst->_func) Error::funcnameexpected(nst->_pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(vt.size());
		Function* f = funcInScope(nst->_name,nst->_pi);
		if(f && nst->_sortsincluded && (nst->_sorts).size() == vt.size()+1) f = f->resolve(nst->_sorts);
		FuncTerm* t = 0;
		if(f) {
			if(belongsToVoc(f)) {
				if(f->arity() == vt.size()) {
					unsigned int n = 0;
					for(; n < vt.size(); ++n) { if(!vt[n]) break; }
					if(n == vt.size()) t = new FuncTerm(f,vt,nst->_pi);
				}
				else Error::wrongfuncarity(f->name(),nst->_pi);
			}
			else Error::funcnotintheovoc(f->name(),_currtheory->name(),nst->_pi);
		}
		else Error::undeclfunc(nst->to_string(),nst->_pi);

		// Cleanup
		if(!t) {
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
		}
		delete(nst);

		return t;
	}

	Term* functerm(NSPair* nst) {
		if(nst->_sortsincluded || (nst->_name).size() != 1) {
			vector<Term*> vt = vector<Term*>(0);
			return functerm(nst,vt);
		}
		else {
			Term* t = 0;
			string name = (nst->_name)[0];
			Variable* v = getVar(name);
			nst->includeArity(0);
			Function* f = funcInScope(nst->_name,nst->_pi);
			if(v) {
				if(f) Warning::varcouldbeconst((nst->_name)[0],nst->_pi);
				t = new VarTerm(v,nst->_pi);
			}
			else if(f) {
				vector<Term*> vt(0);
				t = new FuncTerm(f,vt,nst->_pi);
			}
			else {
				YYLTYPE l; 
				l.first_line = (nst->_pi).line();
				l.first_column = (nst->_pi).column();
				v = quantifiedvar(name,l);
				t = new VarTerm(v,nst->_pi);
			}
			delete(nst);
			return t;
		}
	}

	Term* arterm(char c, Term* lt, Term* rt, YYLTYPE l) {
		if(lt && rt) {
			Function* f = _currvocabulary->func(string(1,c) + "/2");
			assert(f);
			vector<Term*> vt(2); vt[0] = lt; vt[1] = rt;
			return new FuncTerm(f,vt,parseinfo(l));
		}
		else {
			if(lt) delete(lt);
			if(rt) delete(rt);
			return 0;
		}
	}

	Term* arterm(const string& s, Term* t, YYLTYPE l) {
		if(t) {
			Function* f = _currvocabulary->func(s + "/1");
			assert(f);
			vector<Term*> vt(1,t);
			return new FuncTerm(f,vt,parseinfo(l));
		}
		else {
			delete(t);
			return 0;
		}
	}

	FTTuple* fttuple(Formula* f, Term* t) {
		if(f && t) return new FTTuple(f,t);
		else {
			if(f) delete(f);
			if(t) delete(t);
			return 0;
		}
	}

	QuantSetExpr* set(const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			ParseInfo pi = parseinfo(l);
			return new QuantSetExpr(vv,f,pi);
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			return 0;
		}
	}

	EnumSetExpr* set(const vector<FTTuple*>& vftt, YYLTYPE l) {
		vector<Formula*> vf;
		vector<Term*> vt;
		for(unsigned int n = 0; n < vftt.size(); ++n) {
			if(vftt[n]->_formula && vftt[n]->_term) {
				vf.push_back(vftt[n]->_formula);
				vt.push_back(vftt[n]->_term);
			}
			else {
				for(unsigned int m = 0; m < vftt.size(); ++m) {
					if(vftt[m]->_formula) delete(vftt[m]->_formula);
					if(vftt[m]->_term) delete(vftt[m]->_term);
				}
				return 0;
			}
		}
		ParseInfo pi = parseinfo(l);
		return new EnumSetExpr(vf,vt,pi);
	}

	EnumSetExpr* set(const vector<Formula*>& vf, YYLTYPE l) {
		vector<Term*> vt;
		for(unsigned int n = 0; n < vf.size(); ++n) {
			if(vf[n]) {
				Element one; one._int = 1;
				vt.push_back(new DomainTerm(*(StdBuiltin::instance()->sort("int")->begin()),ELINT,one,vf[n]->pi()));
			}
			else {
				for(unsigned int m = 0; m < vf.size(); ++m) {
					if(vf[m]) delete(vf[m]);
				}
				return 0;
			}
		}
		ParseInfo pi = parseinfo(l);
		return new EnumSetExpr(vf,vt,pi);
	}

	AggTerm* aggregate(AggType at, SetExpr* s, YYLTYPE l) {
		if(s) {
			ParseInfo pi = parseinfo(l);
			return new AggTerm(s,at,pi);
		}
		else return 0;
	}

	DomainTerm* domterm(int n, YYLTYPE l) {
		Element en; en._int = n;
		return new DomainTerm(*(StdBuiltin::instance()->sort("int")->begin()),ELINT,en,parseinfo(l));
	}

	DomainTerm* domterm(double d, YYLTYPE l) {
		Element ed; ed._double = d;
		return new DomainTerm(*(StdBuiltin::instance()->sort("float")->begin()),ELDOUBLE,ed,parseinfo(l));
	}

	DomainTerm* domterm(string* s, YYLTYPE l) {
		Element es; es._string = s;
		return new DomainTerm(*(StdBuiltin::instance()->sort("string")->begin()),ELSTRING,es,parseinfo(l));
	}

	DomainTerm* domterm(char c, YYLTYPE l) {
		Element es; es._string = new string(1,c);
		return new DomainTerm(*(StdBuiltin::instance()->sort("char")->begin()),ELSTRING,es,parseinfo(l));
	}

	DomainTerm* domterm(string* n, Sort* s, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		if(s) { 
			Element en; en._string = n;
			return new DomainTerm(s,ELSTRING,en,pi);
		}
		else return 0;
	}

	void help_execute() {
		// TODO?
	}
}

#endif
