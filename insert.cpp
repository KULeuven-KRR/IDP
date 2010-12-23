/************************************
	insert.cpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "namespace.hpp"
#include "execute.hpp"
#include "error.hpp"
#include "visitor.hpp"
#include "insert.hpp"
#include "parse.h"
#include "builtin.hpp"
#include <iostream>
#include <list>
#include <set>

/********************
	Sort checking
********************/

class SortChecker : public Visitor {

	private:
		Vocabulary* _vocab;

	public:
		SortChecker(Formula* f,Vocabulary* v)		: Visitor(), _vocab(v) { f->accept(this);	}
		SortChecker(Definition* d,Vocabulary* v)	: Visitor(), _vocab(v) { d->accept(this);	}
		SortChecker(FixpDef* d,Vocabulary* v)		: Visitor(), _vocab(v) { d->accept(this);	}

		void visit(PredForm*);
		void visit(EqChainForm*);
		void visit(FuncTerm*);
		void visit(AggTerm*);

};

void SortChecker::visit(PredForm* pf) {
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

void SortChecker::visit(FuncTerm* ft) {
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

void SortChecker::visit(EqChainForm* ef) {
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

void SortChecker::visit(AggTerm* at) {
	if(at->type() != AGGCARD) {
		SetExpr* s = at->set();
		if(s->nrQvars() && s->qvar(0)->sort()) {
			if(!SortUtils::resolve(s->qvar(0)->sort(),StdBuiltin::instance()->sort("float"),_vocab)) {
				Error::wrongsort(s->qvar(0)->name(),s->qvar(0)->sort()->name(),"int or float",s->qvar(0)->pi());
			}
		}
		for(unsigned int n = 0; n < s->nrSubterms(); ++n) {
			if(s->subterm(n)->sort() && !SortUtils::resolve(s->subterm(n)->sort(),StdBuiltin::instance()->sort("float"),_vocab)) {
				Error::wrongsort(s->subterm(n)->to_string(),s->subterm(n)->sort()->name(),"int or float",s->subterm(n)->pi());
			}
		}
	}
	traverse(at);
}

/**********************
	Sort derivation
**********************/

class SortDeriver : public Visitor {

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
		SortDeriver(Formula* f,Vocabulary* v) : Visitor(), _vocab(v) { run(f); }
		SortDeriver(Rule* r,Vocabulary* v)	: Visitor(), _vocab(v) { run(r); }

		// Run sort derivation 
		void run(Formula*);
		void run(Rule*);

		// Visit 
		void visit(QuantForm*);
		void visit(PredForm*);
		void visit(EqChainForm*);

		void visit(Rule*);

		void visit(VarTerm*);
		void visit(DomainTerm*);
		void visit(FuncTerm*);

		void visit(QuantSetExpr*);

	private:

		// Auxiliary methods
		void derivesorts();		// derive the sorts of the variables, based on the sorts in _untyped
		void derivefuncs();		// disambiguate the overloaded functions
		void derivepreds();		// disambiguate the overloaded predicates

		// Check
		void check();
		
};

void SortDeriver::visit(QuantForm* qf) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
			if(!(qf->qvar(n)->sort())) _untyped[qf->qvar(n)] = set<Sort*>();
			_changed = true;
		}
	}
	traverse(qf);
}

void SortDeriver::visit(PredForm* pf) {
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
}

void SortDeriver::visit(EqChainForm* ef) {
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
}

void SortDeriver::visit(Rule* r) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < r->nrQvars(); ++n) {
			if(!(r->qvar(n)->sort())) _untyped[r->qvar(n)] = set<Sort*>();
			_changed = true;
		}
	}
	traverse(r);
}

void SortDeriver::visit(VarTerm* vt) {
	if((!(vt->sort())) && _assertsort) {
		_untyped[vt->var()].insert(_assertsort);
	}
}

void SortDeriver::visit(DomainTerm* dt) {
	if(_firstvisit && (!(dt->sort()))) {
		_domelements.insert(dt);
	}

	if((!(dt->sort())) && _assertsort) {
		dt->sort(_assertsort);
		_changed = true;
		_domelements.erase(dt);
	}
}

void SortDeriver::visit(FuncTerm* ft) {
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
}

void SortDeriver::visit(QuantSetExpr* qs) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < qs->nrQvars(); ++n) {
			if(!(qs->qvar(n)->sort())) {
				_untyped[qs->qvar(n)] = set<Sort*>();
				_changed = true;
			}
		}
	}
	traverse(qs);
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

namespace Insert {

	/***********
		Data
	***********/
	
	string*					_currfile = 0;	// The current file
	vector<string*>			_allfiles;		// All the parsed files
	Namespace*				_currspace;		// The current namespace
	Vocabulary*				_currvocab;		// The current vocabulary
	Theory*					_currtheory;	// The current theory
	Structure*				_currstructure;	// The current structure

	vector<Vocabulary*>		_usingvocab;	// The vocabularies currently used to parse
	vector<unsigned int>	_nrvocabs;		// The number of using statements in the current block

	void closeblock() {
		for(unsigned int n = 0; n < _nrvocabs.back(); ++n) _usingvocab.pop_back();
		_nrvocabs.pop_back();
		_currvocab = 0;
		_currtheory = 0;
		_currstructure = 0;
	}

	map<string,vector<Inference*> >	_inferences;	// All inference methods

	string*		currfile()					{ return _currfile;	}
	void		currfile(const string& s)	{ _allfiles.push_back(_currfile); _currfile = new string(s);	}
	void		currfile(string* s)			{ _allfiles.push_back(_currfile); if(s) _currfile = new string(*s); else _currfile = 0;	}
	ParseInfo		parseinfo(YYLTYPE l)		{ return ParseInfo(l.first_line,l.first_column,_currfile);	}
	FormParseInfo	formparseinfo(Formula* f, YYLTYPE l)	{ return FormParseInfo(l.first_line,l.first_column,_currfile,f);	}
	FormParseInfo	formparseinfo(YYLTYPE l)	{ return FormParseInfo(l.first_line,l.first_column,_currfile,0);	}

	// Empty 2-valued interpretations
	struct NameLoc {
		vector<string>	_name;
		ParseInfo		_pi;
		NameLoc(const vector<string>& vs, const ParseInfo& pi) : _name(vs), _pi(pi) { }
	};
	vector<NameLoc>	_emptyinters;

	// Three-valued interpretations
	enum UTF { UTF_UNKNOWN, UTF_CT, UTF_CF, UTF_ERROR };
	string _utf2string[4] = { "u", "ct", "cf", "error" };

	struct NameUTFLoc {
		vector<string>	_name;
		UTF				_utf;
		ParseInfo		_pi;
		NameUTFLoc(const vector<string>& n, UTF u, const ParseInfo& pi) : _name(n), _utf(u), _pi(pi) { }
	};
	map<Predicate*,FinitePredTable*>	_unknownpredtables;
	map<Function*,FinitePredTable*>		_unknownfunctables;
	vector<NameUTFLoc>					_emptythreeinters;

	
	/******************************************
		Convert vector of names to one name
	******************************************/

	string oneName(const vector<string>& vs) {
		assert(!vs.empty());
		string name = vs[0];
		for(unsigned int n = 1; n < vs.size(); ++n) name = name + "::" + vs[n];
		return name;
	}

	/*****************
		Find names
	*****************/

	bool belongsToVoc(Sort* s) {
		if(_currvocab->contains(s)) return true;
		return false;
	}

	bool belongsToVoc(Predicate* p) {
		if(_currvocab->contains(p)) return true;
		return false;
	}

	bool belongsToVoc(Function* f) {
		if(_currvocab->contains(f)) return true;
		return false;
	}

	Namespace* namespaceInScope(const string& name) {
		Namespace* ns = _currspace;
		while(ns) {
			if(name == ns->name()) {
				return ns;
			}
			else if(ns->isSubspace(name)) {
				return ns->subspace(name); 
			}
			else ns = ns->super();
		}
		return 0;
	}

	AbstractTheory* theoInScope(const string& name) {
		Namespace* ns = _currspace;
		while(ns) {
			if(ns->isTheory(name)) {
				return ns->theory(name);
			}
			else ns = ns->super();
		}
		return 0;
	}

	Vocabulary* vocabInScope(const string& name) {
		Namespace* ns = _currspace;
		while(ns) {
			if(ns->isVocab(name)) {
				return ns->vocabulary(name);
			}
			else ns = ns->super();
		}
		return 0;
	}

	AbstractStructure* structInScope(const string& name) {
		Namespace* ns = _currspace;
		while(ns) {
			if(ns->isStructure(name)) {
				return ns->structure(name);
			}
			else ns = ns->super();
		}
		return 0;
	}

	Vocabulary* vocabInScope(const vector<string>& vs) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return vocabInScope(vs[0]);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0]);
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
	}

	AbstractStructure* structInScope(const vector<string>& vs) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return structInScope(vs[0]);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0]);
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

	Sort* sortInScope(const string& name) {
		vector<Sort*> vs;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			Sort* s = _usingvocab[n]->sort(name);
			if(s) vs.push_back(s);
		}
		if(vs.empty()) return 0;
		else return(SortUtils::overload(vs));
	}

	Sort* sortInScope(const vector<string>& vs) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return sortInScope(vs[0]);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv);
			if(v) return v->sort(vs.back());
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

	Predicate* predInScope(const vector<string>& vs) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return predInScope(vs[0]);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv);
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

	vector<Predicate*> noArPredInScope(const vector<string>& vs) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return noArPredInScope(vs[0]);
		}
		else {
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv);
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

	Function* funcInScope(const vector<string>& vs) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return funcInScope(vs[0]);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv);
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


	vector<Function*> noArFuncInScope(const vector<string>& vs) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return noArFuncInScope(vs[0]);
		}
		else {
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv);
			if(v) return v->func_no_arity(vs.back());
			else return vector<Function*>(0);
		}
	}


	/******************************************************************* 
		Data structures and methods to link variables during parsing 
	*******************************************************************/

	struct VarName {
		string		_name;
		Variable*	_var;
		VarName(const string& n, Variable* v) : _name(n), _var(v) { }
	};

	list<VarName>	curr_vars;

	Variable* getVar(const string& name) {
		for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
			if(name == i->_name) return i->_var;
		}
		return 0;
	}

	void remove_vars(const vector<Variable*>& v) {
		for(unsigned int n = 0; n < v.size(); ++n) {
			if(v[n]) {
				for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
					if(i->_name == v[n]->name()) {
						curr_vars.erase(i);
						break;
					}
				}
			}
		}
	}

	vector<Variable*> freevars(const ParseInfo& pi) {
		vector<Variable*> vv;
		string vs;
		for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
			vv.push_back(i->_var);
			vs = vs + ' ' + i->_name;
		}
		if(!vv.empty()) Warning::freevars(vs,pi);
		curr_vars.clear();
		return vv;
	}

	/***********************
		Global structure
	***********************/

	void initialize() {
		_currspace = Namespace::global();
		_inferences["print"].push_back(new PrintTheory());
		_inferences["print"].push_back(new PrintVocabulary());
		_inferences["print"].push_back(new PrintStructure());
		_inferences["print"].push_back(new PrintNamespace());
		_inferences["push_negations"].push_back(new PushNegations());
		_inferences["remove_equivalences"].push_back(new RemoveEquivalences());
		_inferences["remove_eqchains"].push_back(new RemoveEqchains());
		_inferences["flatten"].push_back(new FlattenFormulas());
		_inferences["ground"].push_back(new GroundingInference());
		_inferences["ground"].push_back(new GroundingWithResult());
		_inferences["convert_to_theory"].push_back(new StructToTheory());
		_inferences["move_quantifiers"].push_back(new MoveQuantifiers());
		_inferences["tseitin"].push_back(new ApplyTseitin());
		_inferences["reduce"].push_back(new GroundReduce());
		_inferences["move_functions"].push_back(new MoveFunctions());
		_inferences["model_expand"].push_back(new ModelExpansionInference());
	}

	void cleanup() {
		for(unsigned int n = 0; n < _allfiles.size(); ++n) {
			if(_allfiles[n]) delete(_allfiles[n]);
		}
		if(_currfile) delete(_currfile);
		for(map<string,vector<Inference*> >::iterator it = _inferences.begin(); it != _inferences.end(); ++it) {
			for(unsigned int n = 0; n < (it->second).size(); ++n) {
				delete((it->second)[n]);
			}
		}
	}

	void closespace() {
		closeblock();
		_currspace = _currspace->super();
		assert(_currspace);
	}

	void openspace(const string& sname, YYLTYPE l) {
		Info::print("Parsing namespace " + sname);
		ParseInfo pi = parseinfo(l);
		Namespace* ns = namespaceInScope(sname);
		if(ns) Error::multdeclns(sname,pi,ns->pi());
		_currspace = new Namespace(sname,_currspace,pi);
		_nrvocabs.push_back(0);
	}


	/*******************
		Vocabularies
	*******************/

	/** Open and close vocabularies **/

	void openvocab(const string& vname, YYLTYPE l) {
		Info::print("Parsing vocabulary " + vname);
		ParseInfo pi = parseinfo(l);
		Vocabulary* v = vocabInScope(vname);
		if(v) {
			Error::multdeclvoc(vname,pi,v->pi());
		}
		else {
			v = new Vocabulary(vname,pi);
			_currspace->add(v);
		}
		_currvocab = v;	
		_usingvocab.push_back(v);
		_nrvocabs.push_back(1);
	}

	void closevocab() {
		closeblock();
	}

	void usingvocab(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Vocabulary* v = vocabInScope(vs);
		if(v) {
			_usingvocab.push_back(v);
			_nrvocabs.back() = _nrvocabs.back()+1;
		}
		else Error::undeclvoc(oneName(vs),pi);
	}

	void setvocab(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Vocabulary* v = vocabInScope(vs);
		if(v) {
			_usingvocab.push_back(v);
			_nrvocabs.back() = _nrvocabs.back()+1;
			if(_currstructure) _currstructure->vocabulary(v);
			else if(_currtheory) _currtheory->vocabulary(v);
			_currvocab = v;
		}
		else {
			Error::undeclvoc(oneName(vs),pi);
		}
	}

	/** Pointers to symbols **/

	Predicate* predpointer(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Predicate* p = predInScope(vs);
		if(!p) Error::undeclpred(oneName(vs),pi);
		return p;
	}

	Predicate* predpointer(const vector<string>& vs, const vector<Sort*>& va, YYLTYPE l) {
		Predicate* p = predpointer(vs,l);
		if(p) p = p->disambiguate(va,0);
		return p;
	}

	Function* funcpointer(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Function* f = funcInScope(vs);
		if(!f) Error::undeclfunc(oneName(vs),pi);
		return f;
	}

	Function* funcpointer(const vector<string>& vs, const vector<Sort*>& va, YYLTYPE l) {
		Function* f = funcpointer(vs,l);
		if(f) f->disambiguate(va,0);
		return f;
	}

	Sort* sortpointer(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Sort* s = sortInScope(vs);
		if(!s) Error::undeclsort(oneName(vs),pi);
		return s;
	}

	Sort* theosortpointer(const vector<string>& vs, YYLTYPE l) {
		Sort* s = sortpointer(vs,l);
		if(s) {
			if(belongsToVoc(s)) {
				return s;
			}
			else {
				ParseInfo pi = parseinfo(l);
				string uname = oneName(vs);
				Error::sortnotintheovoc(uname,_currtheory->name(),pi);
				return 0;
			}
		}
		else return 0;
	}

	/** Add symbols to the current vocabulary **/

	Sort* sort(Sort* s) {
		if(s) _currvocab->addSort(s);
		return s;
	}

	Sort* sort(const string& name, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		ParseInfo pip = parseinfo(l);
		Sort* s = new Sort(name,pi);
		_currvocab->addSort(s);
		Predicate* p = new Predicate(name + "/1",vector<Sort*>(1,s),pip);
		_currvocab->addPred(p);
		s->pred(p);
		return s;
	}

	Sort* sort(const string& name, const vector<Sort*> supbs, bool p, YYLTYPE l) {
		vector<Sort*> vs(0);
		if(p) return sort(name,supbs,vs,l);
		else return sort(name,vs,supbs,l);
	}

	Sort* sort(const string& name, const vector<Sort*> sups, const vector<Sort*> subs, YYLTYPE l) {
		Sort* s = sort(name,l);
		if(s) {
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
		}
		return s;
	}

	Predicate* predicate(Predicate* p) {
		if(p) _currvocab->addPred(p);
		return p;
	}

	Predicate* predicate(const string& name, const vector<Sort*>& sorts, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		string nar = string(name) + '/' + itos(sorts.size());
		for(unsigned int n = 0; n < sorts.size(); ++n) {
			if(!sorts[n]) return 0;
		}
		Predicate* p = new Predicate(nar,sorts,pi);
		_currvocab->addPred(p);
		return p;
	}

	Predicate* predicate(const string& name, YYLTYPE l) {
		vector<Sort*> vs(0);
		return predicate(name,vs,l);
	}

	Function* function(Function* f) {
		if(f) _currvocab->addFunc(f);
		return f;
	}

	Function* function(const string& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		string nar = string(name) + '/' + itos(insorts.size());
		for(unsigned int n = 0; n < insorts.size(); ++n) {
			if(!insorts[n]) return 0;
		}
		if(!outsort) return 0;
		Function* f = new Function(nar,insorts,outsort,pi);
		_currvocab->addFunc(f);
		return f;
	}

	Function* function(const string& name, const vector<Sort*>& sorts, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		string nar = string(name) + '/' + itos(sorts.size());
		for(unsigned int n = 0; n < sorts.size(); ++n) {
			if(!sorts[n]) return 0;
		}
		Function* f = new Function(nar,sorts,pi);
		_currvocab->addFunc(f);
		return f;
	}

	Function* function(const string& name, Sort* outsort, YYLTYPE l) {
		vector<Sort*> vs(0);
		return function(name,vs,outsort,l);
	}


	/*****************
		Structures
	*****************/

	void openstructure(const string& sname, YYLTYPE l) {
		Info::print("Parsing structure " + sname);
		ParseInfo pi = parseinfo(l);
		AbstractStructure* s = structInScope(sname);
		if(s) {
			Error::multdeclstruct(sname,pi,s->pi());
		}
		_currstructure = new Structure(sname,pi);
		_currspace->add(_currstructure);
		_nrvocabs.push_back(0);
	}

	void structinclusioncheck() {
		/*Vocabulary* v = _currstructure->vocabulary();
		for(unsigned int n = 0; n < v->nrSorts(); ++n) {
			Sort* s = v->sort(n);
			if(s->parent()) {
				SortTable* st = _currstructure->inter(s);
				SortTable* pt = _currstructure->inter(s->parent());
				if(st && pt) {
					for(unsigned int r = 0; r < st->size(); ++r) {
						if(!(pt->contains(st->element(r),st->type()))) {
							string str = ElementUtil::ElementToString(st->element(r),st->type());
							Error::sortelnotinsort(str,s->name(),s->parent()->name(),_currstructure->name());
						}
					}
				}
			}
		}
		for(unsigned int n = 0; n < v->nrPreds(); ++n) {
			Predicate* p = v->pred(n);
			PredInter* pt = _currstructure->inter(p);
			if(pt) {
				PredTable* ct = pt->ctpf();
				PredTable* cf = pt->cfpt();
				for(unsigned int c = 0; c < p->arity(); ++c) {
					SortTable* st = _currstructure->inter(p->sort(c));
					if(st) {
						if(ct) {
							for(unsigned int r = 0; r < ct->size(); ++r) {
								if(!(st->contains(ct->element(r,c),ct->type(c)))) {
									string str = ElementUtil::ElementToString(ct->element(r,c),ct->type(c));
									Error::predelnotinsort(str,p->name(),p->sort(c)->name(),_currstructure->name());
								}
							}
						}
						if(cf && ct != cf) {
							for(unsigned int r = 0; r < cf->size(); ++r) {
								if(!(st->contains(cf->element(r,c),cf->type(c)))) {
									string str = ElementUtil::ElementToString(cf->element(r,c),cf->type(c));
									Error::predelnotinsort(str,p->name(),p->sort(c)->name(),_currstructure->name());
								}
							}
						}
					}
				}
			}
		}
		for(unsigned int n = 0; n < v->nrFuncs(); ++n) {
			Function* f = v->func(n);
			PredInter* pt = 0;
			if(_currstructure->inter(f)) pt = _currstructure->inter(f)->predinter();
			if(pt) {
				PredTable* ct = pt->ctpf();
				PredTable* cf = pt->cfpt();
				for(unsigned int c = 0; c < f->nrSorts(); ++c) {
					SortTable* st = _currstructure->inter(f->sort(c));
					if(st) {
						if(ct) {
							for(unsigned int r = 0; r < ct->size(); ++r) {
								if(!(st->contains(ct->element(r,c),ct->type(c)))) {
									string str = ElementUtil::ElementToString(ct->element(r,c),ct->type(c));
									Error::predelnotinsort(str,f->name(),f->sort(c)->name(),_currstructure->name());
								}
							}
						}
						if(cf && ct != cf) {
							for(unsigned int r = 0; r < cf->size(); ++r) {
								if(!(st->contains(cf->element(r,c),cf->type(c)))) {
									string str = ElementUtil::ElementToString(cf->element(r,c),cf->type(c));
									Error::predelnotinsort(str,f->name(),f->sort(c)->name(),_currstructure->name());
								}
							}
						}
					}
				}
			}
		}
		*/
	}

	void functioncheck() {
		/*Vocabulary* v = _currstructure->vocabulary();
		for(unsigned int n = 0; n < v->nrFuncs(); ++n) {
			Function* f = v->func(n);
			FuncInter* ft = _currstructure->inter(f);
			if(ft) {
				PredInter* pt = ft->predinter();
				PredTable* ct = pt->ctpf();
				PredTable* cf = pt->cfpt();
				// Check if the interpretation is indeed a function
				bool isfunc = true;
				if(ct) {
					vector<ElementType> vet = ct->types(); vet.pop_back();
					ElementEquality eq(vet);
					for(unsigned int r = 1; r < ct->size(); ) {
						if(eq(ct->tuple(r-1),ct->tuple(r))) {
							vector<Element> vel = ct->tuple(r);
							vector<string> vstr(vel.size()-1);
							for(unsigned int c = 0; c < vel.size()-1; ++c) 
								vstr[c] = ElementUtil::ElementToString(vel[c],ct->type(c));
							Error::notfunction(f->name(),_currstructure->name(),vstr);
							while(eq(ct->tuple(r-1),ct->tuple(r))) ++r;
							isfunc = false;
						}
						else ++r;
					}
				}
				// Check if the interpretation is total
				if(isfunc && !(f->partial()) && ct && (ct == cf || (!(pt->cf()) && cf->size() == 0))) {
					unsigned int c = 0;
					unsigned int s = 1;
					for(; c < f->arity(); ++c) {
						if(_currstructure->inter(f->insort(c))) {
							s = s * _currstructure->inter(f->insort(c))->size();
						}
						else break;
					}
					if(c == f->arity()) {
						assert(ct->size() <= s);
						if(ct->size() < s) {
							Error::nottotal(f->name(),_currstructure->name());
						}
					}
				}
			}
		}
		*/
	}

	void closestructure() {

		// Complete the structure
		bool change = true;
		while(change) {
			change = false;

			// Assign the unknown predicate interpretations
			for(map<Predicate*,FinitePredTable*>::iterator it = _unknownpredtables.begin(); it != _unknownpredtables.end(); ) {
				map<Predicate*,FinitePredTable*>::iterator jt = it; ++it;
				PredInter* pt = _currstructure->inter(jt->first);
				if(pt) {
					if(pt->ctpf()) {
						if(pt->cfpt()) {
							Error::threethreepred(jt->first->name(),_currstructure->name());
							delete(jt->second);
						}
						else {
							assert(pt->ct()); 
							assert(pt->ctpf()->finite());
							FinitePredTable* fpt = jt->second;
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
						FinitePredTable* fpt = jt->second;
						for(unsigned int n = 0; n < pt->cfpt()->size(); ++n) {
							fpt->addRow(pt->cfpt()->tuple(n),pt->cfpt()->types());
						}
						fpt->sortunique();
						pt->replace(fpt,true,false);
					}
					_unknownpredtables.erase(jt);
					change = true;
				}
			}
			// Assign the unknown function interpretations
			for(map<Function*,FinitePredTable*>::iterator it = _unknownfunctables.begin(); it != _unknownfunctables.end(); ) {
				map<Function*,FinitePredTable*>::iterator jt = it; ++it;
				FuncInter* ft = _currstructure->inter(jt->first);
				PredInter* pt = 0;
				if(ft) pt = ft->predinter();
				if(pt) {
					if(pt->ctpf()) {
						if(pt->cfpt()) {
							Error::threethreefunc(jt->first->name(),_currstructure->name());
							delete(jt->second);
						}
						else {
							assert(pt->ct()); 
							assert(pt->ctpf()->finite());
							FinitePredTable* fpt = jt->second;
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
						FinitePredTable* fpt = jt->second;
						for(unsigned int n = 0; n < pt->cfpt()->size(); ++n) {
							fpt->addRow(pt->cfpt()->tuple(n),pt->cfpt()->types());
						}
						fpt->sortunique();
						pt->replace(fpt,true,false);
					}
					_unknownfunctables.erase(jt);
					change = true;
				}
			}
	
			// Assign the empty 2-valued interpretations
			vector<NameLoc> temp;
			for(unsigned int n = 0; n < _emptyinters.size(); ++n) {
				vector<Predicate*> vp = noArPredInScope(_emptyinters[n]._name);
				vector<Function*> vf = noArFuncInScope(_emptyinters[n]._name);
				if(vp.empty() && vf.empty()) {
					Error::symbnotinstructvoc(oneName(_emptyinters[n]._name),_currstructure->name(),_emptyinters[n]._pi);
				}
				else {	// There is at least one symbol with the parsed name
					vector<Predicate*> evp;
					vector<Function*> evf;
					for(unsigned int m = 0; m < vp.size(); ++m) {
						if(!(_currstructure->inter(vp[m]))) evp.push_back(vp[m]);
					}
					for(unsigned int m = 0; m < vf.size(); ++m) {
						if(!(_currstructure->inter(vf[m]))) evf.push_back(vf[m]);
					}
					if(evp.empty() && evf.empty()) {
						Error::emptyassign(oneName(_emptyinters[n]._name),_emptyinters[n]._pi);
					}
					else if(evp.size() + evf.size() == 1) {
						if(!evp.empty()) {
							vector<ElementType> vet(evp[0]->arity(),ELINT);
							FinitePredTable* upt = new FinitePredTable(vet);
							_currstructure->inter(evp[0],new PredInter(upt,true));
						}
						else {
							vector<ElementType> vet(evf[0]->nrSorts(),ELINT);
							FinitePredTable* upt = new FinitePredTable(vet);
							FiniteFuncTable* fft = new FiniteFuncTable(upt);
							PredInter* pt = new PredInter(upt,true);
							vet.pop_back();
							_currstructure->inter(evf[0],new FuncInter(fft,pt));
						}
					}
					else {
						temp.push_back(_emptyinters[n]);
					}
				}
			}
			if(temp.size() < _emptyinters.size()) {
				change = true; _emptyinters = temp;
			}

			// Assign the empty 3-valued interpretations
			vector<NameUTFLoc> tempthree;
			for(unsigned int n = 0; n < _emptythreeinters.size(); ++n) {
				vector<Predicate*> vp = noArPredInScope(_emptythreeinters[n]._name);
				vector<Function*> vf = noArFuncInScope(_emptythreeinters[n]._name);
				if(vp.empty() && vf.empty()) {
					Error::symbnotinstructvoc(oneName(_emptythreeinters[n]._name),_currstructure->name(),_emptythreeinters[n]._pi);
				}
				else {
					vector<Predicate*> evp;
					vector<Function*> evf;
					for(unsigned int m = 0; m < vp.size(); ++m) {
						PredInter* pt = _currstructure->inter(vp[m]);
						if(pt) {
							switch(_emptythreeinters[n]._utf) {
								case UTF_CT:
									if(!(pt->ctpf())) evp.push_back(vp[m]);
									break;
								case UTF_CF:
									if(!(pt->cfpt())) evp.push_back(vp[m]);
									break;
								case UTF_UNKNOWN:
									evp.push_back(vp[m]);
									break;
								default:
									assert(false);
							}
						}
					}
					for(unsigned int m = 0; m < vf.size(); ++m) {
						FuncInter* pf = _currstructure->inter(vf[m]);
						if(pf) {
							PredInter* pt = pf->predinter();
							switch(_emptythreeinters[n]._utf) {
								case UTF_CT:
									if(!(pt->ctpf())) evf.push_back(vf[m]);
									break;
								case UTF_CF:
									if(!(pt->cfpt())) evf.push_back(vf[m]);
									break;
								case UTF_UNKNOWN:
									evf.push_back(vf[m]);
									break;
								default:
									assert(false);
							}
						}
					}				
					if(evp.empty() && evf.empty()) {
						Error::emptyassign(oneName(_emptythreeinters[n]._name),_emptythreeinters[n]._pi);
					}
					else if(evp.size() + evf.size() == 1) {
						if(!evp.empty()) {
							vector<ElementType> vet(evp[0]->arity(),ELINT);
							FinitePredTable* upt = new FinitePredTable(vet);
							switch(_emptythreeinters[n]._utf) {
								case UTF_CT:
									_currstructure->inter(evp[0])->replace(upt,true,true);
									break;
								case UTF_CF:
									_currstructure->inter(evp[0])->replace(upt,false,true);
									break;
								case UTF_UNKNOWN:
									if(_unknownpredtables.find(evp[0]) == _unknownpredtables.end()) 
										_unknownpredtables[evp[0]] = upt;
									else {
										Error::multunknpredinter(evp[0]->name(),_emptythreeinters[n]._pi);
									}
									break;
								default:
									assert(false);
							}
						}
						else {
							vector<ElementType> vet(evf[0]->nrSorts(),ELINT);
							FinitePredTable* upt = new FinitePredTable(vet);
							switch(_emptythreeinters[n]._utf) {
								case UTF_CT:
									_currstructure->inter(evf[0])->predinter()->replace(upt,true,true);
									break;
								case UTF_CF:
									_currstructure->inter(evf[0])->predinter()->replace(upt,false,true);
									break;
								case UTF_UNKNOWN:
									if(_unknownfunctables.find(evf[0]) == _unknownfunctables.end()) 
										_unknownfunctables[evf[0]] = upt;
									else {
										Error::multunknfuncinter(evf[0]->name(),_emptythreeinters[n]._pi);
									}
									break;
								default:
									assert(false);
							}
						}
					}
					else {
						tempthree.push_back(_emptythreeinters[n]);
					}
				}
			}
			if(tempthree.size() < _emptythreeinters.size()) {
				change = true; _emptythreeinters = tempthree;
			}
		}

		// Errors for non-assignable tables
		for(unsigned int n = 0; n < _emptyinters.size(); ++n) {
			Error::emptyambig(oneName(_emptyinters[n]._name),_emptyinters[n]._pi);
		}
		for(unsigned int n = 0; n < _emptythreeinters.size(); ++n) {
			Error::emptyambig(oneName(_emptythreeinters[n]._name),_emptythreeinters[n]._pi);
		}
		for(map<Predicate*,FinitePredTable*>::iterator it = _unknownpredtables.begin(); it != _unknownpredtables.end(); ++it) {
			Error::onethreepred(it->first->name(),_currstructure->name());
			delete(it->second);
		}
		for(map<Function*,FinitePredTable*>::iterator it = _unknownfunctables.begin(); it != _unknownfunctables.end(); ++it) {
			Error::onethreefunc(it->first->name(),_currstructure->name());
			delete(it->second);
		}
	
		// inclusion checks
		structinclusioncheck();

		// function check
		functioncheck();

		// close the structure
		_currstructure->close();

		// reset the auxiliary data structures
		_emptyinters.clear();
		_emptythreeinters.clear();
		_unknownpredtables.clear();
		_unknownfunctables.clear();

		// close block
		closeblock();
	}

	void closeaspstructure() {
		for(unsigned int n = 0; n < _currstructure->nrSortInters(); ++n) {
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
		}
		closestructure();
	}

	/** Two-valued interpretations **/

	void sortinter(const vector<string>& sname, FiniteSortTable* t, YYLTYPE l) {
		Sort* s = sortInScope(sname);
		vector<string> pname(sname); pname.back() = pname.back() + "/1";
		Predicate* p = predInScope(pname);
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
		else {
			ParseInfo pi = parseinfo(l);
			Error::prednotinstructvoc(oneName(pname),_currstructure->name(),pi);
		}
	}

	void predinter(const vector<string>& pname, FinitePredTable* t, YYLTYPE l) {
		Predicate* p = predInScope(pname);
		if(p) {
			if(_currstructure->inter(p)) {
				ParseInfo pi = parseinfo(l);
				Error::multpredinter(p->name(),pi);
			}
			else {
				t->sortunique();
				PredInter* pt = new PredInter(t,true);
				_currstructure->inter(p,pt);
			}
		}
		else {
			ParseInfo pi = parseinfo(l);
			Error::prednotinstructvoc(oneName(pname),_currstructure->name(),pi);
		}
	}

	void funcinter(const vector<string>& fname, FinitePredTable* t, YYLTYPE l) {
		Function* f = funcInScope(fname);
		if(f) {
			if(_currstructure->inter(f)) {
				ParseInfo pi = parseinfo(l);
				Error::multfuncinter(f->name(),pi);
			}
			else {
				t->sortunique();
				vector<ElementType> in = t->types(); ElementType out = in.back(); in.pop_back();
				PredInter* pt = new PredInter(t,true);
				FiniteFuncTable* fft = new FiniteFuncTable(t);
				FuncInter* ft = new FuncInter(fft,pt);
				_currstructure->inter(f,ft);
			}
		}
		else {
			ParseInfo pi = parseinfo(l);
			Error::funcnotinstructvoc(oneName(fname),_currstructure->name(),pi);
		}
	}

	void truepredinter(const vector<string>& name, YYLTYPE l) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		upt->addRow();
		predinter(name,upt,l);
	}

	void falsepredinter(const vector<string>& name, YYLTYPE l) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		predinter(name,upt,l);
	}

	void emptyinter(const vector<string>& name, YYLTYPE l) {
		_emptyinters.push_back(NameLoc(name,parseinfo(l)));
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

	void threepredinter(const vector<string>& pname, const string& utf, FinitePredTable* t, YYLTYPE l) {
		Predicate* p = predInScope(pname);
		ParseInfo pi = parseinfo(l);
		if(p) {
			t->sortunique();
			switch(getUTF(utf,pi)) {
				case UTF_UNKNOWN:
					if(_unknownpredtables.find(p) == _unknownpredtables.end() && !(p->builtin()))  _unknownpredtables[p] = t;
					else Error::multunknpredinter(p->name(),pi);
					break;
				case UTF_CT:
				{	
					PredInter* pt = _currstructure->inter(p);
					if(pt) {
						if(pt->ctpf()) Error::multctpredinter(p->name(),pi);
						else pt->replace(t,true,true);
					}
					else {
						pt = new PredInter(t,0,true,true);
						_currstructure->inter(p,pt);
					}
					break;
				}
				case UTF_CF:
				{
					PredInter* pt = _currstructure->inter(p);
					if(pt) {
						if(pt->cfpt()) Error::multcfpredinter(p->name(),pi);
						else pt->replace(t,false,true);
					}
					else {
						pt = new PredInter(0,t,true,true);
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
		else Error::prednotinstructvoc(oneName(pname),_currstructure->name(),pi);
	}

	void truethreepredinter(const vector<string>& pname, const string& utf, YYLTYPE l) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		upt->addRow();
		threepredinter(pname,utf,upt,l);
	}

	void falsethreepredinter(const vector<string>& pname, const string& utf, YYLTYPE l) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		threepredinter(pname,utf,upt,l);
	}

	void threefuncinter(const vector<string>& fname, const string& utf, FinitePredTable* t, YYLTYPE l) {
		Function* f = funcInScope(fname);
		ParseInfo pi = parseinfo(l);
		if(f) {
			t->sortunique();
			switch(getUTF(utf,pi)) {
				case UTF_UNKNOWN:
					if(_unknownfunctables.find(f) == _unknownfunctables.end() && !(f->builtin()))  _unknownfunctables[f] = t;
					else Error::multunknfuncinter(f->name(),pi);
					break;
				case UTF_CT:
				{	
					FuncInter* ft = _currstructure->inter(f);
					if(ft) {
						if(ft->predinter()->ctpf()) Error::multctfuncinter(f->name(),pi);
						else ft->predinter()->replace(t,true,true);
					}
					else {
						PredInter* pt = new PredInter(t,0,true,true);
						vector<ElementType> in = t->types(); ElementType out = in.back(); in.pop_back();
						ft = new FuncInter(0,pt);
						_currstructure->inter(f,ft);
					}
					break;
				}
				case UTF_CF:
				{
					FuncInter* ft = _currstructure->inter(f);
					if(ft) {
						if(ft->predinter()->cfpt()) Error::multcffuncinter(f->name(),pi);
						else ft->predinter()->replace(t,false,true);
					}
					else {
						PredInter* pt = new PredInter(0,t,true,true);
						vector<ElementType> in = t->types(); ElementType out = in.back(); in.pop_back();
						ft = new FuncInter(0,pt);
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
		else Error::funcnotinstructvoc(oneName(fname),_currstructure->name(),pi);
	}

	void threeinter(const vector<string>& name, const string& utf, FiniteSortTable* t, YYLTYPE l) {
		vector<string> pname = name; pname.back() = pname.back() + "/1";
		vector<string> fname = name; fname.back() = fname.back() + "/0";
		Predicate* p = predInScope(pname);
		Function* f = funcInScope(fname);
		ParseInfo pi = parseinfo(l);
		if(p) {
			if(f) Error::predorfuncsymbol(oneName(name),pi);
			else {
				FinitePredTable* spt = new FinitePredTable(*t);
				threepredinter(pname,utf,spt,l);
			}
		}
		else if(f) {
			FinitePredTable* spt = new FinitePredTable(*t);
			threefuncinter(fname,utf,spt,l);
		}
		else Error::symbnotinstructvoc(oneName(name),_currstructure->name(),pi);
	}

	void emptythreeinter(const vector<string>& name, const string& utf, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		UTF u = getUTF(utf,pi);
		if(u != UTF_ERROR) {
			NameUTFLoc nul(name,u,pi);
			_emptythreeinters.push_back(nul);
		}
	}

	/** ASP atoms **/

	void predatom(Predicate* p, const vector<ElementType>& vet, const vector<Element>& ve) {
		FinitePredTable* pt = dynamic_cast<FinitePredTable*>(_currstructure->inter(p)->ctpf());
		pt->addRow(ve,vet);
	}

	void predatom(Predicate* p, ElementType t, Element e) {
		PredInter* pt = _currstructure->inter(p);
		if(pt) {
			FiniteSortTable* spt = dynamic_cast<FiniteSortTable*>(pt->ctpf());
			FiniteSortTable* ust = 0;
			switch(t) {
				case ELINT:
					ust = spt->add(e._int); break;
				case ELDOUBLE:
					ust = spt->add(e._double); break;
				case ELSTRING:
					ust = spt->add(e._string); break;
				default:
					assert(false);
			}
			if(ust != spt) {
				delete(spt);
				pt->replace(ust,true,true);
				if(p->sort(0)->pred() == p) _currstructure->inter(p->sort(0),ust);
			}
		}
		else {
			FiniteSortTable* ust = 0;
			switch(t) {
				case ELINT: ust = new IntSortTable(); ust->add(e._int); break;
				case ELDOUBLE: ust = new FloatSortTable(); ust->add(e._double); break;
				case ELSTRING: ust = new StrSortTable(); ust->add(e._string); break;
				default: assert(false);
			}
			pt = new PredInter(ust,true);
			_currstructure->inter(p,pt);
			if(p->sort(0)->pred() == p) _currstructure->inter(p->sort(0),ust);
		}
	}

	void predatom(Predicate* p, FiniteSortTable* st) {
		PredInter* pt = _currstructure->inter(p);
		if(!pt) {
			pt = new PredInter(st,true);
			_currstructure->inter(p,pt);
			if(p->sort(0)->pred() == p) _currstructure->inter(p->sort(0),st);
		}
		else {
			switch(st->type()) {
				case ELINT: {
					RanSortTable* rst = dynamic_cast<RanSortTable*>(st);
					for(unsigned int n = 0; n < st->size(); ++n) {
						Element e; e._int = (*rst)[n];
						predatom(p,ELINT,e);
					}
					break;
				}
				case ELSTRING: {
					StrSortTable* sst = dynamic_cast<StrSortTable*>(st);
					for(unsigned int n = 0; n < st->size(); ++n) {
						Element e; e._string = (*sst)[n];
						predatom(p,ELSTRING,e);
					}
					break;
				}
				default: assert(false);
			}
			delete(st);
		}
	}

	void predatom(Predicate* p, const vector<ElementType>& vet, const vector<Element>& ve, const vector<FiniteSortTable*>& vst) {
		vector<ElementType> vet2(vet.size());
		for(unsigned int c = 0; c < vet.size(); ++c) {
			switch(vet[c]) {
				case ELDOUBLE: vet2[c] = ve[c]._double ? ELDOUBLE : ELINT; break;
				default: vet2[c] = vet[c];
			}
		}
		vector<Element> ve2(ve.size());
		vector<unsigned int> vi(vst.size(),0); 
		unsigned int posctr = 0;
		while(posctr < vi.size()) {
			// Add the current tuple
			unsigned int vstctr = 0;
			for(unsigned int c = 0; c < ve2.size(); ++c) {
				switch(vet[c]) {
					case ELINT: ve2[c]._int = ve[c]._int; break;
					case ELDOUBLE:
						if(ve[c]._double) ve2[c]._double = ve[c]._double;
						else {
							ve2[c]._int = (vst[vstctr]->element(vi[vstctr]))._int;
							++vstctr;
						}
						break;
					case ELSTRING:
						if(ve[c]._string) ve2[c]._string = new string(*(ve[c]._string));
						else {
							ve2[c]._string = (vst[vstctr]->element(vi[vstctr]))._string;
							++vstctr;
						}
						break;
					default:
						assert(false);
				}
			}
			predatom(p,vet2,ve2);
			// Advance in the tables
			bool advanced = false;
			while(!advanced && posctr < vi.size()) {
				++vi[posctr];
				if(vi[posctr] == vst[posctr]->size()) {
					vi[posctr] = 0;
					++posctr;
				}
				else {
					advanced = true;
					posctr = 0;
				}
			}
		}
		for(unsigned int c = 0; c < ve.size(); ++c) {
			switch(vet[c]) {
				case ELINT: break;
				case ELDOUBLE: break;
				case ELSTRING: if(ve[c]._string) delete(ve[c]._string); break;
				default: assert(false);
			}
		}
		for(unsigned int n = 0; n < vst.size(); ++n) {
			delete(vst[n]);
		}
	}

	void predatom(const vector<string>& name,YYLTYPE l) {
		vector<ElementType> vet(0);
		vector<Element> ve(0);
		vector<FiniteSortTable*> vst(0);
		return predatom(name,vet,ve,vst,l);
	}

	void predatom(const vector<string>& name, const vector<ElementType>& vet, const vector<Element>& ve, const vector<FiniteSortTable*>& vst, YYLTYPE l) {
		Predicate* p = predInScope(name);
		ParseInfo pi = parseinfo(l);
		if(p) {
			if(p->arity() == 1) {
				if(vst.empty()) predatom(p,vet[0],ve[0]);
				else predatom(p,vst[0]);
			}
			else {
				PredInter* pt = _currstructure->inter(p);
				if(!pt) {
					vector<ElementType> vet2(vet.size(),ELINT);
					FinitePredTable* upt = new FinitePredTable(vet2);
					pt = new PredInter(upt,true);
					_currstructure->inter(p,pt);
				}
				if(vst.empty()) predatom(p,vet,ve);
				else predatom(p,vet,ve,vst);
			}
		}
		else Error::prednotinstructvoc(oneName(name),_currstructure->name(),pi);
	}

	void funcatom(Function* f, const vector<ElementType>& vet, const vector<Element>& ve) {
		FuncInter* ft = _currstructure->inter(f);
		FiniteFuncTable* fft = dynamic_cast<FiniteFuncTable*>(ft->functable());
		fft->ftable()->addRow(ve,vet);
	}

	void funcatom(Function* f, const vector<ElementType>& vet, const vector<Element>& ve, const vector<FiniteSortTable*>& vst) {
		vector<ElementType> vet2(vet.size());
		for(unsigned int c = 0; c < vet.size(); ++c) {
			switch(vet[c]) {
				case ELDOUBLE: vet2[c] = ve[c]._double ? ELDOUBLE : ELINT; break;
				default: vet2[c] = vet[c];
			}
		}
		vector<Element> ve2(ve.size());
		vector<unsigned int> vi(vst.size(),0); 
		unsigned int posctr = 0;
		while(posctr < vi.size()) {
			// Add the current tuple
			unsigned int vstctr = 0;
			for(unsigned int c = 0; c < ve2.size(); ++c) {
				switch(vet[c]) {
					case ELINT: ve2[c]._int = ve[c]._int; break;
					case ELDOUBLE:
						if(ve[c]._double) ve2[c]._double = ve[c]._double;
						else {
							ve2[c]._int = (vst[vstctr]->element(vi[vstctr]))._int;
							++vstctr;
						}
						break;
					case ELSTRING:
						if(ve[c]._string) ve2[c]._string = new string(*(ve[c]._string));
						else {
							ve2[c]._string = (vst[vstctr]->element(vi[vstctr]))._string;
							++vstctr;
						}
						break;
					default:
						assert(false);
				}
			}
			funcatom(f,vet2,ve2);
			// Advance in the tables
			bool advanced = false;
			while(!advanced && posctr < vi.size()) {
				++vi[posctr];
				if(vi[posctr] == vst[posctr]->size()) {
					vi[posctr] = 0;
					++posctr;
				}
				else {
					advanced = true;
					posctr = 0;
				}
			}
		}
		for(unsigned int c = 0; c < ve.size(); ++c) {
			switch(vet[c]) {
				case ELINT: break;
				case ELDOUBLE: break;
				case ELSTRING: if(ve[c]._string) delete(ve[c]._string); break;
				default: assert(false);
			}
		}
		for(unsigned int n = 0; n < vst.size(); ++n) {
			delete(vst[n]);
		}
	}

	void funcatom(const vector<string>& name, const vector<ElementType>& vet, const vector<Element>& ve, const vector<FiniteSortTable*>& vst, YYLTYPE l) {
		Function* f = funcInScope(name);
		ParseInfo pi = parseinfo(l);
		if(f) {
			FuncInter* ft = _currstructure->inter(f);
			if(!ft) {
				vector<ElementType> vet2(vet.size(),ELINT);
				FinitePredTable* upt = new FinitePredTable(vet2);
				FiniteFuncTable* fft = new FiniteFuncTable(upt);
				PredInter* pt = new PredInter(upt,true);
				ft = new FuncInter(fft,pt);
				_currstructure->inter(f,ft);
			}
			if(vst.empty()) funcatom(f,vet,ve);
			else funcatom(f,vet,ve,vst);
		}
		else Error::funcnotinstructvoc(oneName(name),_currstructure->name(),pi);
	}

	/***************
		Theories
	***************/

	void opentheory(const string& tname, YYLTYPE l) {
		Info::print("Parsing theory "  + tname);
		ParseInfo pi = parseinfo(l);
		AbstractTheory* t = theoInScope(tname);
		if(t) {
			Error::multdecltheo(tname,pi,t->pi());
		}
		_currtheory = new Theory(tname,pi);
		_currspace->add(_currtheory);
		_nrvocabs.push_back(0);
	}

	void closetheory() {
		closeblock();
	}

	void definition(Definition* d) {
		if(d) _currtheory->add(d);
	}

	void sentence(Formula* f) {
		if(f) {
			// 1. Quantify the free variables universally
			vector<Variable*> vv = freevars(f->pi());
			if(!vv.empty()) f =  new QuantForm(true,true,vv,f,f->pi());
			// 2. Sort derivation & checking
			SortDeriver sd(f,_currvocab); 
			SortChecker sc(f,_currvocab);
			// Add the formula to the current theory
			_currtheory->add(f);
		}
		else curr_vars.clear();
	}

	void fixpdef(FixpDef* d) {
		if(d) _currtheory->add(d);
	}

	BoolForm* trueform() {
		vector<Formula*> vf(0);
		return new BoolForm(true,true,vf,FormParseInfo());
	}

	BoolForm* trueform(YYLTYPE l) {
		vector<Formula*> vf(0);
		FormParseInfo pi = formparseinfo(new BoolForm(true,true,vf,FormParseInfo()),l);
		return new BoolForm(true,true,vf,pi);
	}

	BoolForm* falseform() {
		vector<Formula*> vf(0);
		return new BoolForm(true,false,vf,FormParseInfo());
	}

	BoolForm* falseform(YYLTYPE l) {
		vector<Formula*> vf(0);
		FormParseInfo pi = formparseinfo(new BoolForm(true,false,vf,FormParseInfo()),l);
		return new BoolForm(true,false,vf,pi);
	}

	PredForm* predform(const vector<string>& vs, const vector<Term*>& vt, YYLTYPE l) {
		FormParseInfo pi = formparseinfo(l);
		Predicate* p = predInScope(vs);
		string uname = oneName(vs);
		if(p) {
			if(belongsToVoc(p)) {
				for(unsigned int n = 0; n < vt.size(); ++n) {
					if(!vt[n]) {
						// delete the correctly parsed terms
						for(unsigned int m = 0; m < n; ++m) delete(vt[m]);
						for(unsigned int m = n+1; m < vt.size(); ++m) { if(vt[m]) delete(vt[m]); }
						return 0;
					}
				}
				return new PredForm(true,p,vt,pi);	
			}
			else {
				Error::prednotintheovoc(uname,_currtheory->name(),pi);
				for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
				return 0;
			}
		}
		else {
			Error::undeclpred(uname,pi);
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
			return 0;
		}
	}

	PredForm* predform(NSTuple* nst, const vector<Term*>& vt, YYLTYPE l) {
		FormParseInfo pi = formparseinfo(l);
		(nst->_name).back() = (nst->_name).back() + '/' + itos((nst->_sorts).size());
		Predicate* p = predInScope(nst->_name);
		if(p) p = p->disambiguate(nst->_sorts,0);
		if(p) {
			if(belongsToVoc(p)) {
				if(p->arity() == vt.size()) {
					for(unsigned int n = 0; n < vt.size(); ++n) {
						if(!vt[n]) {
							// delete the correctly parsed terms
							for(unsigned int m = 0; m < n; ++m) delete(vt[m]);
							for(unsigned int m = n+1; m < vt.size(); ++m) { if(vt[m]) delete(vt[m]); }
							delete(nst);
							return 0;
						}
					}
					delete(nst);
					return new PredForm(true,p,vt,pi);	// TODO change the formparseinfo
				}
				else {
					Error::wrongpredarity(p->name(),pi);
					for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
					delete(nst);
					return 0;
				}
			}
			else {
				Error::prednotintheovoc(p->name(),_currtheory->name(),pi);
				for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
				delete(nst);
				return 0;
			}
		}
		else {
			Error::undeclpred(oneName(nst->_name),pi);
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
			delete(nst);
			return 0;
		}
	}

	PredForm* predform(const vector<string>& vs, YYLTYPE l) {
		vector<Term*> vt(0);
		return predform(vs,vt,l);
	}

	PredForm* predform(NSTuple* t, YYLTYPE l) {
		vector<Term*> vt(0);
		return predform(t,vt,l);
	}

	PredForm* funcgraphform(const vector<string>& vs, const vector<Term*>& vt, Term* t, YYLTYPE l) {
		FormParseInfo pi = formparseinfo(l);
		Function* f = funcInScope(vs);
		string uname = oneName(vs);
		if(f) {
			if(belongsToVoc(f)) {
				for(unsigned int n = 0; n < vt.size(); ++n) {
					if(!vt[n]) {
						for(unsigned int m = 0; m < n; ++m) delete(vt[m]);
						for(unsigned int m = n+1; m < vt.size(); ++m) { if(vt[m]) delete(vt[m]); }
						if(t) delete(t);
						return 0;
					}
				}
				if(!t) {
					for(unsigned int n = 0; n < vt.size(); ++n) delete(vt[n]);
					return 0;
				}
				vector<Term*> vt2(vt); vt2.push_back(t);
				return new PredForm(true,f,vt2,pi);	// TODO: change the formparseinfo
			}
			else {
				Error::funcnotintheovoc(uname,_currtheory->name(),pi);
				for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
				return 0;
			}
		}
		else {
			Error::undeclfunc(uname,pi);
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
			return 0;
		}
	}

	PredForm* funcgraphform(const vector<string>& vs, Term* t, YYLTYPE l) {
		vector<Term*> vt;
		return funcgraphform(vs,vt,t,l);
	}

	EquivForm* equivform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			//Formula* fpf = new EquivForm(true,lf->pi().original(),rf->pi().original(),FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new EquivForm(true,lf,rf,pi);
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	BoolForm* disjform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			vector<Formula*> vf(2);
			vector<Formula*> pivf(2);
			vf[0] = lf; vf[1] = rf;
			//pivf[0] = lf->pi().original(); pivf[1] = rf->pi().original();
			//BoolForm* pibf = new BoolForm(true,false,pivf,FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new BoolForm(true,false,vf,pi);
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	BoolForm* conjform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			vector<Formula*> vf(2);
			vector<Formula*> pivf(2);
			vf[0] = lf; vf[1] = rf;
			//pivf[0] = lf->pi().original(); pivf[1] = rf->pi().original();
			//BoolForm* pibf = new BoolForm(true,true,pivf,FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new BoolForm(true,true,vf,pi);
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	BoolForm* implform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			lf->swapsign();
			BoolForm* bf = disjform(lf,rf,l);
			return bf;
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	BoolForm* revimplform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			rf->swapsign();
			BoolForm* bf = disjform(lf,rf,l);
			return bf;
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	QuantForm* univform(const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			//QuantForm* piqf = new QuantForm(true,true,vv,f->pi().original(),FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new QuantForm(true,true,vv,f,pi);
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			delete(f);
			return 0;
		}
	}

	QuantForm* existform(const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			//QuantForm* piqf = new QuantForm(true,false,vv,f->pi().original(),FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new QuantForm(true,false,vv,f,pi);
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			delete(f);
			return 0;
		}
	}

	PredForm* bexform(char c, bool b, int n, const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			FormParseInfo pi = formparseinfo(l);
			QuantSetExpr* qse = new QuantSetExpr(vv,f,pi);
			vector<Term*> vt(2);
			Element en; en._int = n;
			vt[0] = new DomainTerm(StdBuiltin::instance()->sort("int"),ELINT,en,pi);
			vt[1] = new AggTerm(qse,AGGCARD,pi);
			Predicate* p = StdBuiltin::instance()->pred(string(1,c) + "/2");
			return new PredForm(b,p,vt,pi);	// TODO adapt formparseinfo
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			delete(f);
			return 0;
		}
	}

	EqChainForm* eqchain(char c, bool b, Term* lt, Term* rt, YYLTYPE l) {
		if(lt && rt) {
			FormParseInfo pi = formparseinfo(l);	// TODO adapt formparseinfo
			EqChainForm* ecf = new EqChainForm(true,true,lt,pi);
			ecf->add(c,b,rt);
			return ecf;
		}
		else {
			if(lt) delete(lt);
			if(rt) delete(rt);
			return 0;
		}
	}

	EqChainForm* eqchain(char c, bool b, EqChainForm* ecf, Term* rt, YYLTYPE l) {
		if(ecf && rt) {
			ecf->add(c,b,rt);
			return ecf;
		}
		else {
			if(ecf) delete(ecf);
			if(rt) delete(rt);
			return 0;
		}
	}

	Rule* rule(const vector<Variable*>& qv, PredForm* head, Formula* body,YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		remove_vars(qv);
		if(head && body) {
			// Quantify the free variables
			vector<Variable*> vv = freevars(head->pi());
			remove_vars(vv);
			// Split quantified variables in head and body variables
			vector<Variable*> hv;
			vector<Variable*> bv;
			for(unsigned int n = 0; n < qv.size(); ++n) {
				if(head->contains(qv[n])) hv.push_back(qv[n]);
				else bv.push_back(qv[n]);
			}
			for(unsigned int n = 0; n < vv.size(); ++n) {
				if(head->contains(vv[n])) hv.push_back(vv[n]);
				else bv.push_back(vv[n]);
			}
			// Create a new rule
			if(!(bv.empty())) body = new QuantForm(true,false,bv,body,FormParseInfo((body->pi())));
			Rule* r = new Rule(hv,head,body,pi);
			// Sort derivation
			SortDeriver sd(r,_currvocab);
			// Return the rule
			return r;
		}
		else {
			curr_vars.clear();
			if(head) delete(head);
			if(body) delete(body);
			for(unsigned int n = 0; n < qv.size(); ++n) delete(qv[n]);
			return 0;
		}
	}

	Rule* rule(const vector<Variable*>& qv, PredForm* head, YYLTYPE l) {
		Formula* body = trueform();
		return rule(qv,head,body,l);
	}

	Rule* rule(PredForm* head, Formula* body, YYLTYPE l) {
		vector<Variable*> vv(0);
		return rule(vv,head,body,l);
	}

	Rule* rule(PredForm* head,YYLTYPE l) {
		Formula* body = trueform();
		return rule(head,body,l);
	}

	Definition* definition(const vector<Rule*>& rules) {
		Definition* d = new Definition();
		for(unsigned int n = 0; n < rules.size(); ++n) {
			if(rules[n]) d->add(rules[n]);
		}
		return d;
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

	FuncTerm* functerm(const vector<string>& vs, const vector<Term*>& vt, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Function* f = funcInScope(vs);
		string uname = oneName(vs);
		if(f) {
			if(belongsToVoc(f)) {
				for(unsigned int n = 0; n < vt.size(); ++n) {
					if(!vt[n]) {
						for(unsigned int m = 0; m < vt.size(); ++m) { if(vt[m]) delete(vt[m]);	}
						return 0;
					}
				}
				return new FuncTerm(f,vt,pi);
			}
			else {
				Error::funcnotintheovoc(uname,_currtheory->name(),pi);
				for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
				return 0;
			}
		}
		else {
			Error::undeclfunc(uname,pi);
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
			return 0;
		}
	}

	FuncTerm* functerm(NSTuple* nst, const vector<Term*>& vt, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		(nst->_name).back() = (nst->_name).back() + '/' + itos((nst->_sorts).size()-1);
		Function* f = funcInScope(nst->_name);
		if(f) f = f->disambiguate(nst->_sorts,0);
		if(f) {
			if(belongsToVoc(f)) {
				if(f->arity() == vt.size()) {
					for(unsigned int n = 0; n < vt.size(); ++n) {
						if(!vt[n]) {
							for(unsigned int m = 0; m < vt.size(); ++m) { if(vt[m]) delete(vt[m]);	}
							delete(nst);
							return 0;
						}
					}
					delete(nst);
					return new FuncTerm(f,vt,pi);
				}
				else {
					Error::wrongfuncarity(f->name(),pi);
					for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
					delete(nst);
					return 0;
				}
			}
			else {
				Error::funcnotintheovoc(f->name(),_currtheory->name(),pi);
				for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
				delete(nst);
				return 0;
			}
		}
		else {
			Error::undeclfunc(oneName(nst->_name),pi);
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
			delete(nst);
			return 0;
		}
	}

	FuncTerm* functerm(const vector<string>& vs, YYLTYPE l) {
		vector<Term*> vt = vector<Term*>(0);
		return functerm(vs,vt,l);
	}

	FuncTerm* functerm(NSTuple* nst, YYLTYPE l) {
		vector<Term*> vt = vector<Term*>(0);
		return functerm(nst,vt,l);
	}

	Term* funcvar(const vector<string>& vs, YYLTYPE l) {
		if(vs.size() == 1) {
			Variable* v = getVar(vs[0]);
			string c = vs[0] + "/0";
			Function* f = funcInScope(c);
			ParseInfo pi = parseinfo(l);
			if(v) {
				if(f) Warning::varcouldbeconst(vs[0],pi);
				return new VarTerm(v,pi);
			}
			else if(f) {
				vector<Term*> vt(0);
				return new FuncTerm(f,vt,pi);
			}
			else {
				v = quantifiedvar(vs[0],l);
				return new VarTerm(v,pi);
			}
		}
		else {
			vector<string> vs2(vs);
			vs2.back() = vs2.back() + "/0";
			return functerm(vs2,l); 
		}
	}

	Term* arterm(char c, Term* lt, Term* rt, YYLTYPE l) {
		if(lt && rt) {
			Function* f = _currvocab->func(string(1,c) + "/2");
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
			Function* f = _currvocab->func(s + "/1");
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
				vt.push_back(new DomainTerm(StdBuiltin::instance()->sort("int"),ELINT,one,vf[n]->pi()));
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
		return new DomainTerm(StdBuiltin::instance()->sort("int"),ELINT,en,parseinfo(l));
	}

	DomainTerm* domterm(double d, YYLTYPE l) {
		Element ed; ed._double = d;
		return new DomainTerm(StdBuiltin::instance()->sort("float"),ELDOUBLE,ed,parseinfo(l));
	}

	DomainTerm* domterm(string* s, YYLTYPE l) {
		Element es; es._string = s;
		return new DomainTerm(StdBuiltin::instance()->sort("string"),ELSTRING,es,parseinfo(l));
	}

	DomainTerm* domterm(char c, YYLTYPE l) {
		Element es; es._string = new string(1,c);
		return new DomainTerm(StdBuiltin::instance()->sort("char"),ELSTRING,es,parseinfo(l));
	}

	DomainTerm* domterm(string* n, Sort* s, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		if(s) { 
			Element en; en._string = n;
			return new DomainTerm(s,ELSTRING,en,pi);
		}
	}

	/*****************
		Statements
	*****************/

	bool checkarg(const string& arg, InfArgType t) {
		switch(t) {
			case IAT_THEORY:
				if(theoInScope(arg)) return true;
				break;
			case IAT_VOCABULARY:
				if(vocabInScope(arg)) return true;
				break;
			case IAT_STRUCTURE:
				if(structInScope(arg)) return true;
				break;
			case IAT_NAMESPACE:
				if(namespaceInScope(arg)) return true;
				break;
			case IAT_VOID:
				if(arg.size() == 0) return true;
			default: assert(false); 
		}
		return false;
	}

	InfArg convertarg(const string& arg, InfArgType t) {
		InfArg a;
		switch(t) {
			case IAT_THEORY:
				a._theory = theoInScope(arg);
				break;
			case IAT_VOCABULARY:
				a._vocabulary = vocabInScope(arg);
				break;
			case IAT_STRUCTURE:
				a._structure = structInScope(arg);
				break;
			case IAT_NAMESPACE:
				a._namespace = namespaceInScope(arg);
				break;
			case IAT_VOID:
				break;
			default:
				assert(false);
		}
		return a;
	}

	void command(InfArgType type, const string& cname, const vector<string>& args, const string& res, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		map<string,vector<Inference*> >::iterator it = _inferences.find(cname);
		if(it != _inferences.end()) {
			vector<Inference*> vi;
			for(unsigned int n = 0; n < (it->second).size(); ++n) {
				if(args.size() == (it->second)[n]->arity()) vi.push_back((it->second)[n]);
			}
			if(vi.empty()) {
				Error::unkncommand(cname + '/' + itos(args.size()),pi);
			}
			else {
				vector<Inference*> vi2;
				for(unsigned int n = 0; n < vi.size(); ++n) {
					bool ok = true;
					for(unsigned int m = 0; m < args.size(); ++m) {
						ok = checkarg(args[m],(vi[n]->intypes())[m]);
						if(!ok) break;
					}
					if(ok) {
						if(type == vi[n]->outtype()) vi2.push_back(vi[n]);
					}
				}
				if(vi2.empty()) Error::wrongcommandargs(cname + '/' + itos(args.size()),pi);
				else if(vi2.size() == 1) {
					vector<InfArg> via;
					for(unsigned int m = 0; m < args.size(); ++m)
						via.push_back(convertarg(args[m],(vi2[0]->intypes())[m]));
					vi2[0]->execute(via,res,_currspace);
				}
				else Error::ambigcommand(cname + '/' + itos(args.size()),pi);
			}
		}
		else {
			Error::unkncommand(cname + '/' + itos(args.size()),pi);
		}
	}
	
	void command(const string& cname, const vector<string>& args, YYLTYPE l) {
		string res;
		command(IAT_VOID,cname,args,res,l);
	}

	void command(InfArgType t, const string& cname, const string& res, YYLTYPE l) {
		vector<string> args;
		command(t,cname,args,res,l);
	}

	void command(const string& cname, YYLTYPE l) {
		vector<string> args;
		string res;
		command(IAT_VOID,cname,args,res,l);
	}

}

void help_execute() {
	cout << "The available methods in the execute block are:\n";
	for(map<string,vector<Inference*> >::iterator it = Insert::_inferences.begin(); it != Insert::_inferences.end(); ++it) {
		for(unsigned int n = 0; n < (it->second).size(); ++n) {
			cout << "   " << IATUtils::to_string(((it->second)[n])->outtype()) << ' ' << it->first << '(';
			for(unsigned int m = 0; m < ((it->second)[n])->arity(); ++m) {
				cout << IATUtils::to_string((((it->second)[n])->intypes())[m]);
				if(m != ((it->second)[n])->arity()-1) cout << ',';
			}
			cout << ")\n";
			cout << "      " << ((it->second)[n])->description() << '\n';
		}
	}
}
