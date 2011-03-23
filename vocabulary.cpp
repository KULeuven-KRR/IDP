/************************************
	vocabulary.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "vocabulary.hpp"
#include <iostream>
#include <algorithm>
#include "data.hpp"
#include "namespace.hpp"
#include "structure.hpp"
#include "builtin.hpp"

using namespace std;

/************
	Sorts
************/

Sort::Sort(const string& name) : _name(name), _pi() { 
	_parents = vector<Sort*>(0); 
	_children = vector<Sort*>(0);
	_pred = 0;
}

Sort::Sort(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) { 
	_parents = vector<Sort*>(0);
	_children = vector<Sort*>(0);
	_pred = 0;
}

/** Mutators **/

void Sort::addParent(Sort* p) {
	unsigned int n = 0;
	for(; n < _parents.size(); ++n) {
		if(p == _parents[n]) break;
	}
	if(n == _parents.size()) {
		_parents.push_back(p);
		p->addChild(this);
	}
}

void Sort::addChild(Sort* c) {
	unsigned int n = 0;
	for(; n < _children.size(); ++n) {
		if(c == _children[n]) break;
	}
	if(n == _children.size()) {
		_children.push_back(c);
		c->addParent(this);
	}
}

set<Sort*> Sort::ancestors(Vocabulary* v) const {
	set<Sort*> ss;
	for(unsigned int n = 0; n < nrParents(); ++n) {
		if((!v) || v->contains(parent(n))) ss.insert(parent(n));
		set<Sort*> temp = parent(n)->ancestors(v);
		for(set<Sort*>::iterator it = temp.begin(); it != temp.end(); ++it)
			ss.insert(*it);
	}
	return ss;
}

set<Sort*> Sort::descendents(Vocabulary* v) const {
	set<Sort*> ss;
	for(unsigned int n = 0; n < nrChildren(); ++n) {
		if((!v) || v->contains(child(n))) ss.insert(child(n));
		set<Sort*> temp = child(n)->descendents(v);
		for(set<Sort*>::iterator it = temp.begin(); it != temp.end(); ++it)
			ss.insert(*it);
	}
	return ss;
}

bool OverloadedPredicate::contains(Predicate* p) const {
	for(unsigned int n = 0; n < _overpreds.size(); ++n) {
		if(_overpreds[n]->contains(p)) return true;
	}
	return false;
}

bool OverloadedFunction::contains(Function* f) const {
	for(unsigned int n = 0; n < _overfuncs.size(); ++n) {
		if(_overfuncs[n]->contains(f)) return true;
	}
	return false;
}

/** Utils **/

namespace SortUtils {

	// Return the smallest common ancestor of two sorts, if there is an unique one
	Sort* resolve(Sort* s1, Sort* s2, Vocabulary* v) {
		set<Sort*> ss1 = s1->ancestors(v); ss1.insert(s1);
		set<Sort*> ss2 = s2->ancestors(v); ss2.insert(s2);
		set<Sort*> ss;
		for(set<Sort*>::iterator it = ss1.begin(); it != ss1.end(); ++it) {
			if(ss2.find(*it) != ss2.end()) ss.insert(*it);
		}
		vector<Sort*> vs = vector<Sort*>(ss.begin(),ss.end());
		if(vs.empty()) return 0;
		else if(vs.size() == 1) return vs[0];
		else {
			for(unsigned int n = 0; n < vs.size(); ++n) {
				set<Sort*> ds = vs[n]->ancestors(v);
				for(set<Sort*>::const_iterator it = ds.begin(); it != ds.end(); ++it) ss.erase(*it);
			}
			vs = vector<Sort*>(ss.begin(),ss.end());
			if(vs.size() == 1) return vs[0];
			else return 0;
		}
		//	if(s1->base() != s2->base()) return 0;
		//	while(s1 != s2 && (s1->depth() || s2->depth())) {
		//		if(s1->depth() > s2->depth()) s1 = s1->parent();
		//		else if(s2->depth() > s1->depth()) s2 = s2->parent();
		//		else { s1 = s1->parent(); s2 = s2->parent(); }
		//	}
		//	assert(s1 == s2);
		//	return s1;
	}

}

/****************
	Variables
****************/

int Variable::_nvnr = 0;

/** Constructor for internal variables **/ 

Variable::Variable(Sort* s) : _sort(s) {
	_name = "_var_" + s->name() + "_" + itos(Variable::_nvnr);
	++_nvnr;
}

/** Debugging **/

string Variable::to_string() const {
	return _name;
}


/** Utils **/

void VarUtils::sortunique(vector<Variable*>& vv) {
	sort(vv.begin(),vv.end());
	vector<Variable*>::iterator it = unique(vv.begin(),vv.end());
	vv.erase(it,vv.end());
}

vector<Variable*> VarUtils::makeNewVariables(const vector<Sort*>& sorts) {
	vector<Variable*> vars;
	for(vector<Sort*>::const_iterator it = sorts.begin(); it != sorts.end(); ++it)
		vars.push_back(new Variable(*it));
	return vars;
}

/*******************************
	Predicates and functions
*******************************/

/** Constructor for internal predicates **/ 

int Predicate::_npnr = 0;

Predicate::Predicate(const vector<Sort*>& sorts) : PFSymbol("",sorts,ParseInfo()) {
	_name = "_internal_predicate_" + itos(_npnr) + "/" + itos(sorts.size());
	++_npnr;
}

/** Mutators **/

void OverloadedPredicate::overpred(Predicate* p) {
	assert(p->name() == _name);
	unsigned int n = 0;
	for(; n < _overpreds.size(); ++n) {
		if(_overpreds[n] == p) break;
	}
	if(n == _overpreds.size()) _overpreds.push_back(p);
}

void OverloadedFunction::overfunc(Function* f) {
	assert(f->name() == _name);
	unsigned int n = 0;
	for(; n < _overfuncs.size(); ++n) {
		if(_overfuncs[n] == f) break;
	}
	if(n == _overfuncs.size()) _overfuncs.push_back(f);
}


/** Inspectors **/

vector<Sort*> Function::insorts() const {
	vector<Sort*> vs = _sorts;
	vs.pop_back();
	return vs;
}

Predicate* Predicate::disambiguate(const vector<Sort*>& vs,Vocabulary* v) {
	for(unsigned int n = 0; n < _sorts.size(); ++n) {
		if(!SortUtils::resolve(vs[n],_sorts[n],v)) return 0;
	}
	return this;
}

Predicate* Predicate::resolve(const vector<Sort*>& vs) {
	for(unsigned int n = 0; n < _sorts.size(); ++n) {
		if(vs[n] != _sorts[n]) return 0;
	}
	return this;
}

Function* Function::disambiguate(const vector<Sort*>& vs,Vocabulary* v) {
	for(unsigned int n = 0; n < _sorts.size(); ++n) {
		if(!SortUtils::resolve(vs[n],_sorts[n],v)) return 0;
	}
	return this;
}

Function* Function::resolve(const vector<Sort*>& vs) {
	for(unsigned int n = 0; n < _sorts.size(); ++n) {
		if(vs[n] != _sorts[n]) return 0;
	}
	return this;
}

Predicate* OverloadedPredicate::resolve(const vector<Sort*>& vs) {
	Predicate* candidate = 0;
	for(unsigned int n = 0; n < _overpreds.size(); ++n) {
		Predicate* newcandidate = 0;
		if(_overpreds[n]->overloaded()) {
			newcandidate = _overpreds[n]->resolve(vs);
		}
		else {
			unsigned int m = 0;
			for(; m < vs.size(); ++m) {
				if(vs[m]) {
					if(vs[m] != _overpreds[n]->sort(m)) break;
				}
			}
			if(m == vs.size()) {
				newcandidate = _overpreds[n];
			}
		}
		if(newcandidate) {
			if(candidate && newcandidate != candidate) {
				candidate = 0; 
				break;
			}
			else candidate = newcandidate;
		}
	}
	return candidate;
}

Predicate* OverloadedPredicate::disambiguate(const vector<Sort*>& vs,Vocabulary* v) {
	Predicate* candidate = 0;
	for(unsigned int n = 0; n < _overpreds.size(); ++n) {
		Predicate* newcandidate = 0;
		if(_overpreds[n]->overloaded()) {
			newcandidate = _overpreds[n]->disambiguate(vs,v);
		}
		else {
			unsigned int m = 0;
			for(; m < vs.size(); ++m) {
				if(vs[m]) {
					if(!(SortUtils::resolve(vs[m],_overpreds[n]->sort(m),v))) break;
				}
			}
			if(m == vs.size()) {
				newcandidate = _overpreds[n];
			}
		}
		if(newcandidate) {
			if(candidate && newcandidate != candidate) {
				candidate = 0; 
				break;
			}
			else candidate = newcandidate;
		}
	}
	return candidate;
}

Function* OverloadedFunction::resolve(const vector<Sort*>& vs) {
	Function* candidate = 0;
	for(unsigned int n = 0; n < _overfuncs.size(); ++n) {
		Function* newcandidate = 0;
		if(_overfuncs[n]->overloaded()) {
			newcandidate = _overfuncs[n]->resolve(vs);
		}
		else {
			unsigned int m = 0;
			for(; m < vs.size(); ++m) {
				if(vs[m]) {
					if(vs[m] != _overfuncs[n]->sort(m)) break;
				}
			}
			if(m == vs.size()) {
				newcandidate = _overfuncs[n];
			}
		}
		if(newcandidate) {
			if(candidate && newcandidate != candidate) {
				candidate = 0; 
				break;
			}
			else candidate = newcandidate;
		}
	}
	return candidate;
}

Function* OverloadedFunction::disambiguate(const vector<Sort*>& vs,Vocabulary* v) {
	Function* candidate = 0;
	for(unsigned int n = 0; n < _overfuncs.size(); ++n) {
		Function* newcandidate = 0;
		if(_overfuncs[n]->overloaded()) {
			newcandidate = _overfuncs[n]->disambiguate(vs,v);
		}
		else {
			unsigned int m = 0;
			for(; m < vs.size(); ++m) {
				if(vs[m]) {
					if(!(SortUtils::resolve(vs[m],_overfuncs[n]->sort(m),v))) break;
				}
			}
			if(m == vs.size()) {
				newcandidate = _overfuncs[n];
			}
		}
		if(newcandidate) {
			if(candidate && newcandidate != candidate) {
				candidate = 0; 
				break;
			}
			else candidate = newcandidate;
		}
	}
	return candidate;
}

PredInter* Function::predinter(const AbstractStructure& s) const {
	FuncInter* fi = inter(s);
	if(fi) return fi->predinter();
	else return 0;
}

vector<Predicate*> Predicate::nonbuiltins() {
	vector<Predicate*> vp;
	if(!builtin()) vp.push_back(this);
	return vp;
}

vector<Predicate*> OverloadedPredicate::nonbuiltins() {
	vector<Predicate*> vp;
	for(unsigned int n = 0; n < _overpreds.size(); ++n) {
		vector<Predicate*> temp = _overpreds[n]->nonbuiltins();
		for(unsigned int m = 0; m < temp.size(); ++m) {
			unsigned int k = 0;
			for(; k < vp.size(); ++k) {
				if(vp[k] == temp[m]) break;
			}
			if(k == vp.size()) vp.push_back(temp[m]);
		}
	}
	return vp;
}

vector<Function*> Function::nonbuiltins() {
	vector<Function*> vf;
	if(!builtin()) vf.push_back(this);
	return vf;
}

vector<Function*> OverloadedFunction::nonbuiltins() {
	vector<Function*> vf;
	for(unsigned int n = 0; n < _overfuncs.size(); ++n) {
		vector<Function*> temp = _overfuncs[n]->nonbuiltins();
		for(unsigned int m = 0; m < temp.size(); ++m) {
			unsigned int k = 0;
			for(; k < vf.size(); ++k) {
				if(vf[k] == temp[m]) break;
			}
			if(k == vf.size()) vf.push_back(temp[m]);
		}
	}
	return vf;
}

vector<Sort*> Predicate::allsorts() const {
	return _sorts;
}

vector<Sort*> OverloadedPredicate::allsorts() const {
	vector<Sort*> vs;
	for(unsigned int n = 0; n < _overpreds.size(); ++n) {
		vector<Sort*> temp = _overpreds[n]->allsorts();
		for(unsigned int m = 0; m < temp.size(); ++m) {
			unsigned int k = 0;
			for(; k < vs.size(); ++k) {
				if(vs[k] == temp[m]) break;
			}
			if(k == vs.size()) vs.push_back(temp[m]);
		}
	}
	return vs;
}

vector<Sort*> Function::allsorts() const {
	return _sorts;
}

vector<Sort*> OverloadedFunction::allsorts() const {
	vector<Sort*> vs;
	for(unsigned int n = 0; n < _overfuncs.size(); ++n) {
		vector<Sort*> temp = _overfuncs[n]->allsorts();
		for(unsigned int m = 0; m < temp.size(); ++m) {
			unsigned int k = 0;
			for(; k < vs.size(); ++k) {
				if(vs[k] == temp[m]) break;
			}
			if(k == vs.size()) vs.push_back(temp[m]);
		}
	}
	return vs;
}


namespace PredUtils {

	// Overload two predicates
	Predicate* overload(Predicate* p1, Predicate* p2) {
		assert(p1->name() == p2->name());
		if(p1 == p2) return p1;
		OverloadedPredicate* ovp = new OverloadedPredicate(p1->name(),p1->arity());
		ovp->overpred(p1);
		ovp->overpred(p2);
		return ovp;
	}

	// Overload multiple predicates
	Predicate* overload(const vector<Predicate*>& vp) {
		if(vp.empty()) return 0;
		else if(vp.size() == 1) return vp[0];
		else {
			OverloadedPredicate* ovp = new OverloadedPredicate(vp[0]->name(),vp[0]->arity());
			for(unsigned int n = 0; n < vp.size(); ++n) {
				ovp->overpred(vp[n]);
			}
			return ovp;
		}
	}


}

namespace FuncUtils {

	// Overload two functions
	Function* overload(Function* f1, Function* f2) {
		assert(f1->name() == f2->name());
		if(f1 == f2) return f1;
		OverloadedFunction* ovf = new OverloadedFunction(f1->name(),f1->arity());
		ovf->overfunc(f1);
		ovf->overfunc(f2);
		return ovf;
	}

	// Overload multiple functions
	Function* overload(const vector<Function*>& vf) {
		if(vf.empty()) return 0;
		else if(vf.size() == 1) return vf[0];
		else {
			OverloadedFunction* ovf = new OverloadedFunction(vf[0]->name(),vf[0]->arity());
			for(unsigned int n = 0; n < vf.size(); ++n) {
				ovf->overfunc(vf[n]);
			}
			return ovf;
		}
	}

}


/*****************
	Vocabulary
*****************/

/** Constructors **/

Vocabulary::Vocabulary(const string& name) : _name(name) {
	if(_name != "std") addVocabulary(StdBuiltin::instance());
}

Vocabulary::Vocabulary(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) {
	if(_name != "std") addVocabulary(StdBuiltin::instance());
}

void Vocabulary::addVocabulary(Vocabulary* v) {
	for(map<string,set<Sort*> >::iterator it = v->firstsort(); it != v->lastsort(); ++it) {
		for(set<Sort*>::iterator jt = (it->second).begin(); jt != (it->second).end(); ++jt) addSort(*jt);
	}
	for(map<string,Predicate*>::iterator it = v->firstpred(); it != v->lastpred(); ++it) {
		addPred(it->second);
	}
	for(map<string,Function*>::iterator it = v->firstfunc(); it != v->lastfunc(); ++it) {
		addFunc(it->second);
	}
}

/** Mutators **/

void Vocabulary::addSort(Sort* s) {
	if(!contains(s)) {
		_name2sort[s->name()].insert(s);
//		if(_name2sort.find(s->name()) == _name2sort.end()) {
//			_name2sort[s->name()] = s;
//		}
//		else {
//			Sort* ovs = SortUtils::overload(s,_name2sort[s->name()]);
//			assert(ovs->overloaded());
//			_name2sort[s->name()] = ovs;
//		}

//		if(s->overloaded()) {
//			vector<Sort*> vs = s->nonbuiltins();
//			for(unsigned int n = 0; n < vs.size(); ++n) {
//				if(_sort2index.find(vs[n]) == _sort2index.end()) {
//					_sort2index[vs[n]] = _index2sort.size();
//					_index2sort.push_back(vs[n]);
//				}
//			}
//		}
//		else {
			if(!s->builtin()) {
				_sort2index[s] = _index2sort.size();
				_index2sort.push_back(s);
			}
//		}

		if(s->pred()) addPred(s->pred());
	}
}

void Vocabulary::addPred(Predicate* p) {
	if(!contains(p)) {
		if(_name2pred.find(p->name()) == _name2pred.end()) {
			_name2pred[p->name()] = p;
		}
		else {
			Predicate* ovp = PredUtils::overload(p,_name2pred[p->name()]);
			_name2pred[p->name()] = ovp;
		}

		if(p->overloaded()) {
			vector<Predicate*> vp = p->nonbuiltins();
			for(unsigned int n = 0; n < vp.size(); ++n) {
				if(_predicate2index.find(vp[n]) == _predicate2index.end()) {
					_predicate2index[vp[n]] = _index2predicate.size();
					_index2predicate.push_back(vp[n]);
				}
			}
			vector<Sort*> vs = p->allsorts();
			for(unsigned int n = 0; n < vs.size(); ++n) {
				addSort(vs[n]);
			}
		}
		else {
			if(!p->builtin()) {
				_predicate2index[p] = _index2predicate.size();
				_index2predicate.push_back(p);
			}
			for(unsigned int n = 0; n < p->arity(); ++n) addSort(p->sort(n));
		}
	}
}

void Vocabulary::addFunc(Function* f) {
	if(!contains(f)) {
		if(_name2func.find(f->name()) == _name2func.end()) {
			_name2func[f->name()] = f;
		}
		else {
			Function* ovf = FuncUtils::overload(f,_name2func[f->name()]);
			_name2func[f->name()] = ovf;
		}

		if(f->overloaded()) {
			vector<Function*> vf = f->nonbuiltins();
			for(unsigned int n = 0; n < vf.size(); ++n) {
				if(_function2index.find(vf[n]) == _function2index.end()) {
					_function2index[vf[n]] = _index2function.size();
					_index2function.push_back(vf[n]);
				}
			}
			vector<Sort*> vs = f->allsorts();
			for(unsigned int n = 0; n < vs.size(); ++n) {
				addSort(vs[n]);
			}
		}
		else {
			if(!f->builtin()) {
				_function2index[f] = _index2function.size();
				_index2function.push_back(f);
			}
			for(unsigned int n = 0; n < f->arity(); ++n) addSort(f->insort(n));
			addSort(f->outsort());
		}
	}
}

/** Inspectors **/

bool Vocabulary::contains(Sort* s) const {
	map<string,set<Sort*> >::const_iterator it = _name2sort.find(s->name());
	if(it != _name2sort.end()) {
		if((it->second).find(s) != (it->second).end()) return true; 
	}
	return false;
}

bool Vocabulary::contains(Predicate* p) const {
	map<string,Predicate*>::const_iterator it = _name2pred.find(p->name());
	if(it != _name2pred.end()) return it->second->contains(p);
	else return false;
}

bool Vocabulary::contains(Function* f) const {
	map<string,Function*>::const_iterator it = _name2func.find(f->name());
	if(it != _name2func.end()) return it->second->contains(f);
	else return false;
}

bool Vocabulary::contains(PFSymbol* s) const {
	if(s->ispred()) {
		return contains(dynamic_cast<Predicate*>(s));
	}
	else {
		return contains(dynamic_cast<Function*>(s));
	}
}

unsigned int Vocabulary::index(Sort* s) const {
	assert(_sort2index.find(s) != _sort2index.end());
	return _sort2index.find(s)->second;
}

unsigned int Vocabulary::index(Predicate* p) const {
	assert(_predicate2index.find(p) != _predicate2index.end());
	return _predicate2index.find(p)->second;
}

unsigned int Vocabulary::index(Function* f) const {
	assert(_function2index.find(f) != _function2index.end());
	return _function2index.find(f)->second;
}

const set<Sort*>* Vocabulary::sort(const string& name) const {
	map<string,set<Sort*> >::const_iterator it = _name2sort.find(name);
	if(it != _name2sort.end()) return &(it->second);
	else return 0;
}

Predicate* Vocabulary::pred(const string& name) const {
	map<string,Predicate*>::const_iterator it = _name2pred.find(name);
	if(it != _name2pred.end()) return it->second;
	else return 0;
}

Function* Vocabulary::func(const string& name) const {
	map<string,Function*>::const_iterator it = _name2func.find(name);
	if(it != _name2func.end())  return it->second;
	else  return 0;
}

vector<Predicate*> Vocabulary::pred_no_arity(const string& name) const {
	vector<Predicate*> vp;
	for(map<string,Predicate*>::const_iterator it = _name2pred.begin(); it != _name2pred.end(); ++it) {
		string nm = it->second->name();
		if(nm.substr(0,nm.find('/')) == name) vp.push_back(it->second);
	}
	return vp;
}

vector<Function*> Vocabulary::func_no_arity(const string& name) const {
	vector<Function*> vf;
	for(map<string,Function*>::const_iterator it = _name2func.begin(); it != _name2func.end(); ++it) {
		string nm = it->second->name();
		if(nm.substr(0,nm.find('/')) == name) vf.push_back(it->second);
	}
	return vf;
}

/** Debugging **/

string PFSymbol::to_string() const {
	string s  = _name.substr(0,_name.find('/'));	
	if(nrSorts()) {
		if(sort(0)) s += '[' + sort(0)->to_string();
		else s += '[';
		for(unsigned int n = 1; n < nrSorts(); ++n) {
			if(sort(n)) s += ',' + sort(n)->to_string();
			else s += ',';
		}
		s += ']';
	}
	return s;
}

string OverloadedPredicate::to_string() const {
	string s = " ";
	for(unsigned int n = 0; n < _overpreds.size(); ++n) {
		s += _overpreds[n]->to_string();
		s += ' ';
	}
	return s;
}

string OverloadedFunction::to_string() const {
	string s = " ";
	for(unsigned int n = 0; n < _overfuncs.size(); ++n) {
		s += _overfuncs[n]->to_string();
		s += ' ';
	}
	return s;
}

string Vocabulary::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "Vocabulary " + _name + ":\n";
	s = s + tab + "  Sorts:\n";
	for(map<string,set<Sort*> >::const_iterator it = _name2sort.begin(); it != _name2sort.end(); ++it) {
		for(set<Sort*>::const_iterator jt = (it->second).begin(); jt != (it->second).end(); ++jt) {
			s = s + tab + "    " + (*jt)->to_string() + '\n';
		}
	}
	s = s + tab + "  Predicates:\n";
	for(map<string,Predicate*>::const_iterator it = _name2pred.begin(); it != _name2pred.end(); ++it) {
		s = s + tab + "    " + it->second->to_string() + '\n';
	}
	s = s + tab + "  Functions:\n";
	for(map<string,Function*>::const_iterator it = _name2func.begin(); it != _name2func.end(); ++it) {
		s = s + tab + "    " + it->second->to_string() + '\n';
	}
	return s;
}
