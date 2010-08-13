/************************************
	vocabulary.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "namespace.hpp"
#include "builtin.hpp"
#include <iostream>
#include <algorithm>

extern string itos(int);
extern string dtos(double);
extern string tabstring(unsigned int);
extern bool isInt(const string&);
extern bool isInt(double);
extern int stoi(const string&);
extern bool isDouble(const string&);
extern double stod(const string&);
extern int MAX_INT;

/*********************
	Argument types
*********************/

namespace IATUtils {
	string to_string(InfArgType t) {
		switch(t) {
			case IAT_VOID: return "void";
			case IAT_NAMESPACE: return "Namespace";
			case IAT_STRUCTURE: return "Structure";
			case IAT_THEORY: return "Theory";
			case IAT_VOCABULARY: return "Vocabulary";
			default: assert(false); return "";
		}
	}
}

/**********************
	Domain elements
**********************/

string compound::to_string() const {
	if(_function) {
		string s = _function->to_string();
		if(!_args.empty()) {
			s = s + '(' + ElementUtil::ElementToString(_args[0]);
			for(unsigned int n = 1; n < _args.size(); ++n) {
				s = s + ',' + ElementUtil::ElementToString(_args[n]);
			}
			s = s + ')';
		}
		return s;
	}
	else {
		assert(_args.size() == 1);
		return ElementUtil::ElementToString(_args[0]);
	}
}

namespace ElementUtil {

	ElementType resolve(ElementType t1, ElementType t2) {
		return (t1 < t2) ? t2 : t1;
	}

	ElementType leasttype() { return ELINT;	}

	ElementType reduce(Element e, ElementType t) {
		switch(t) {
			case ELINT: 
				break;
			case ELDOUBLE:
				if(double(int(e._double)) == e._double) return ELINT;
				break;
			case ELSTRING:
				if(isInt(*(e._string))) return ELINT;
				else if(isDouble(*(e._string))) return ELDOUBLE;
				break;
			case ELCOMPOUND:
				if(!(e._compound->_function)) return reduce((e._compound->_args)[0]._element,(e._compound->_args)[0]._type);
				break;
			default:
				assert(false);
		}
		return t;
	}

	ElementType reduce(TypedElement te) {
		return reduce(te._element,te._type);
	}

	string ElementToString(Element e, ElementType t) {
		switch(t) {
			case ELINT:
				return itos(e._int);
			case ELDOUBLE:
				return dtos(e._double);
			case ELSTRING:
				return *(e._string);
			case ELCOMPOUND:
				return e._compound->to_string();
			default:
				assert(false); return "???";
		}
	}

	string ElementToString(TypedElement e) {
		return ElementToString(e._element,e._type);
	}

	Element nonexist(ElementType t) {
		Element e;
		switch(t) {
			case ELINT:
				e._int = MAX_INT;
			case ELDOUBLE:
				e._double = MAX_DOUBLE;
			case ELSTRING:
				e._string = 0;
			case ELCOMPOUND:
				e._compound = 0;
			default:
				assert(false);
		}
		return e;
	}

	bool exists(Element e, ElementType t) {
		switch(t) {
			case ELINT:
				return e._int != MAX_INT;
			case ELDOUBLE:
				return e._double != MAX_DOUBLE;
			case ELSTRING:
				return e._string != 0;
			case ELCOMPOUND:
				return e._compound != 0;
			default:
				assert(false); return false;
		}
	}
	
	bool exists(TypedElement e) {
		return exists(e._element,e._type);
	}

	Element convert(Element e, ElementType oldtype, ElementType newtype) {
		if(oldtype == newtype) return e;
		Element ne;
		switch(oldtype) {
			case ELINT:
				if(newtype == ELSTRING) {
					ne._string = IDPointer(itos(e._int));
				}
				else if(newtype == ELDOUBLE) {
					ne._double = double(e._int);
				}
				else {
					assert(newtype == ELCOMPOUND);
					TypedElement te(e,oldtype);
					ne._compound = CPPointer(te);
				}
				break;
			case ELDOUBLE:
				if(newtype == ELINT) {
					if(isInt(e._double)) {
						ne._int = int(e._double);
					}
					else return nonexist(newtype);
				}
				else if(newtype == ELSTRING) {
					ne._string = IDPointer(dtos(e._double));
				}
				else {
					assert(newtype == ELCOMPOUND);
					TypedElement te(e,oldtype);
					ne._compound = CPPointer(te);
				}
				break;
			case ELSTRING:
				if(newtype == ELINT) {
					if(isInt(*(e._string))) {
						ne._int = stoi(*(e._string));
					}
					else return nonexist(newtype);
				}
				else if(newtype == ELDOUBLE) {
					if(isDouble(*(e._string))) {
						ne._double = stod(*(e._string));
					}
					else return nonexist(newtype);
				}
				else {
					assert(newtype == ELCOMPOUND);
					TypedElement te(e,oldtype);
					ne._compound = CPPointer(te);
				}
				break;
			case ELCOMPOUND:
				if(e._compound->_function == 0) 
					return convert((e._compound->_args)[0],newtype);
				else return nonexist(newtype);
				break;
			default:
				assert(false);
		}
		return ne;
	}

	Element	convert(TypedElement te, ElementType t) {
		return convert(te._element,te._type,t);
	}

	bool equal(Element e1, ElementType t1, Element e2, ElementType t2) {
		switch(t1) {
			case ELINT:
				switch(t2) {
					case ELINT: return e1._int == e2._int;
					case ELDOUBLE: return double(e1._int) == e2._double;
					case ELSTRING: return (isInt(*(e2._string)) && e1._int == stoi(*(e2._string)));
					case ELCOMPOUND: return ((e2._compound)->_function == 0 && 
											  equal(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELDOUBLE:
				switch(t2) {
					case ELINT: return e1._double == double(e2._int);
					case ELDOUBLE: return e1._double == e2._double;
					case ELSTRING: return (isDouble(*(e2._string)) && e1._double == stod(*(e2._string)));
					case ELCOMPOUND: return ((e2._compound)->_function == 0 && 
											  equal(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELSTRING:
				switch(t2) {
					case ELINT: return (isInt(*(e1._string)) && e2._int == stoi(*(e1._string)));
					case ELDOUBLE: return (isDouble(*(e1._string)) && e2._double == stod(*(e1._string)));
					case ELSTRING: return e1._string == e2._string;
					case ELCOMPOUND: return ((e2._compound)->_function == 0 && 
											  equal(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELCOMPOUND:
				switch(t2) {
					case ELINT: return ((e1._compound)->_function == 0 && 
											  equal(e2,t2,((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type));
					case ELDOUBLE: return ((e1._compound)->_function == 0 && 
											  equal(e2,t2,((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type));
					case ELSTRING: return ((e1._compound)->_function == 0 && 
											  equal(e2,t2,((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type));
					case ELCOMPOUND: 
						if((e1._compound)->_function != (e2._compound)->_function) return false;
						else {
							for(unsigned int n = 0; n < (e1._compound)->_function->arity(); ++n) {
								if(!(((e1._compound)->_args)[n] ==  ((e2._compound)->_args)[n])) return false;
							}
							return true;
						}
					default: assert(false); return false;
				}
			default:
				assert(false); return false;
		}
	}

	bool strlessthan(Element e1, ElementType t1, Element e2, ElementType t2) {
		switch(t1) {
			case ELINT:
				switch(t2) {
					case ELINT: return e1._int < e2._int;
					case ELDOUBLE: return double(e1._int) < e2._double;
					case ELSTRING: return ((!isDouble(*(e2._string))) || double(e1._int) < stod(*(e2._string)));
					case ELCOMPOUND: return ((e2._compound)->_function != 0 || 
											  strlessthan(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELDOUBLE:
				switch(t2) {
					case ELINT: return e1._double < double(e2._int);
					case ELDOUBLE: return e1._double < e2._double;
					case ELSTRING: return ((!isDouble(*(e2._string))) || e1._double < stod(*(e2._string)));
					case ELCOMPOUND: return ((e2._compound)->_function != 0 || 
											  strlessthan(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELSTRING:
				switch(t2) {
					case ELINT: return (isDouble(*(e1._string)) && stod(*(e1._string)) < double(e2._int));
					case ELDOUBLE: return (isDouble(*(e1._string)) && stod(*(e1._string)) < e2._double);
					case ELSTRING: {
						if(isDouble(*(e1._string))) {
							if(isDouble(*(e2._string))) return stod(*(e1._string)) < stod(*(e2._string));
							else return true;
						}
						else if(isDouble(*(e2._string))) return false;
						else return e1._string < e2._string;
					}
					case ELCOMPOUND: return ((e2._compound)->_function != 0 || 
											  strlessthan(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELCOMPOUND:
				switch(t2) {
					case ELINT: return ((e1._compound)->_function == 0 && 
											  strlessthan(((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type,e2,t2));
					case ELDOUBLE: return ((e1._compound)->_function == 0 && 
											  strlessthan(((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type,e2,t2));
					case ELSTRING: return ((e1._compound)->_function == 0 && 
											  strlessthan(((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type,e2,t2));
					case ELCOMPOUND: 
						if((e1._compound)->_function == 0) {
							if((e2._compound)->_function == 0) return ((e1._compound)->_args)[0] < ((e2._compound)->_args)[0];
							else return true;
						}
						else if((e2._compound)->_function == 0) return false;
						else if((e1._compound)->_function < (e2._compound)->_function) return true;
						else if((e1._compound)->_function > (e2._compound)->_function) return false;
						else {
							for(unsigned int n = 0; n < (e1._compound)->_function->arity(); ++n) {
								if((((e1._compound)->_args)[n] <  ((e2._compound)->_args)[n])) return true;
								if((((e2._compound)->_args)[n] <  ((e1._compound)->_args)[n])) return false;
							}
							return false;
						}
					default: assert(false); return false;
				}
			default:
				assert(false); return false;
		}
	}


	bool lessthanorequal(Element e1, ElementType t1, Element e2, ElementType t2) {
		return (strlessthan(e1,t1,e2,t2) || equal(e1,t1,e2,t2));
	}

}

bool operator==(TypedElement e1, TypedElement e2)	{ return ElementUtil::equal(e1._element,e1._type,e2._element,e2._type);				}
bool operator<=(TypedElement e1, TypedElement e2)	{ return ElementUtil::lessthanorequal(e1._element,e1._type,e2._element,e2._type);	}
bool operator<(TypedElement e1, TypedElement e2)	{ return ElementUtil::strlessthan(e1._element,e1._type,e2._element,e2._type);		}

/************
	Sorts
************/

Sort::Sort(const string& name) : _name(name), _pi() { 
	_parent = 0;
	_base = this;
	_depth = 0;
	_children = vector<Sort*>(0);
	_pred = 0;
}

Sort::Sort(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) { 
	_parent = 0;
	_base = this;
	_depth = 0;
	_children = vector<Sort*>(0);
	_pred = 0;
}

/** Mutators **/

void Sort::parent(Sort* p) {
	if(_parent != p) {
		_parent = p;
		_base = p->base();
		_depth = p->depth() + 1;
		p->child(this);
	}
}

void Sort::child(Sort* c) {
	unsigned int n = 0;
	for(; n < _children.size(); ++n) {
		if(c == _children[n]) break;
	}
	if(n == _children.size()) {
		_children.push_back(c);
		c->parent(this);
	}
}

/** Utils **/

namespace SortUtils {

	// Return the smallest common ancestor of two sorts
	Sort* resolve(Sort* s1, Sort* s2) {
		if(s1->base() != s2->base()) return 0;
		while(s1 != s2 && (s1->depth() || s2->depth())) {
			if(s1->depth() > s2->depth()) s1 = s1->parent();
			else if(s2->depth() > s1->depth()) s2 = s2->parent();
			else { s1 = s1->parent(); s2 = s2->parent(); }
		}
		assert(s1 == s2);
		return s1;
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

/*******************************
	Predicates and functions
*******************************/

/** Constructor for internal predicates **/ 

int Predicate::_npnr = 0;

Predicate::Predicate(const vector<Sort*>& sorts) : PFSymbol("",sorts,ParseInfo()) {
	_name = "_internal_predicate_" + itos(_npnr) + "/" + itos(sorts.size());
	++_npnr;
}

/** Inspectors **/

vector<Sort*> Function::insorts() const {
	vector<Sort*> vs = _sorts;
	vs.pop_back();
	return vs;
}


/*****************
	Vocabulary
*****************/

/** Mutators **/

void Vocabulary::addSort(Sort* s) {
	if(!contains(s)) {
		_sorts[s] = _vsorts.size();
		_vsorts.push_back(s);
		if(s->parent()) addSort(s->parent());
	}
}

void Vocabulary::addPred(Predicate* p) {
	if(!contains(p)) {
		_predicates[p] = _vpredicates.size();
		_vpredicates.push_back(p);
		for(unsigned int n = 0; n < p->arity(); ++n) addSort(p->sort(n));
	}
}

void Vocabulary::addFunc(Function* f) {
	if(!contains(f)) {
		_functions[f] = _vfunctions.size();
		_vfunctions.push_back(f);
		for(unsigned int n = 0; n < f->arity(); ++n) addSort(f->insort(n));
		addSort(f->outsort());
	}
}

void Vocabulary::addSort(const string& name, Sort* s) {
	if(!s->builtin()) {
		addSort(s);
		_sortnames[name] = s;
	}
}

void Vocabulary::addPred(const string& name, Predicate* p) {
	if(!p->builtin()) {
		addPred(p);
		_prednames[name] = p;
	}
}

void Vocabulary::addFunc(const string& name, Function* f) {
	if(!f->builtin()) {
		addFunc(f);
		_funcnames[name] = f;
	}
}


/** Inspectors **/

bool Vocabulary::contains(Sort* s) const {
	if(s->builtin()) return true;
	else return (_sorts.find(s) != _sorts.end());
}

bool Vocabulary::contains(Predicate* p) const {
	if(p->builtin()) return true;
	else return (_predicates.find(p) != _predicates.end());
}

bool Vocabulary::contains(Function* f) const {
	if(f->builtin()) return true;
	else return (_functions.find(f) != _functions.end());
}

unsigned int Vocabulary::index(Sort* s) const {
	assert(contains(s));
	assert(!s->builtin());
	return _sorts.find(s)->second;
}

unsigned int Vocabulary::index(Predicate* p) const {
	assert(contains(p));
	assert(!p->builtin());
	return _predicates.find(p)->second;
}

unsigned int Vocabulary::index(Function* f) const {
	assert(contains(f));
	assert(!f->builtin());
	return _functions.find(f)->second;
}

Sort* Vocabulary::sort(const string& name) const {
	Sort* s = stdbuiltin()->sort(name);
	if(s) return s;
	else {
		map<string,Sort*>::const_iterator it = _sortnames.find(name);
		if(it != _sortnames.end()) return it->second;
		else return 0;
	}
}

Predicate* Vocabulary::pred(const string& name) const {
	Predicate* p = stdbuiltin()->pred(name);
	if(p) return p;
	else {
		map<string,Predicate*>::const_iterator it = _prednames.find(name);
		if(it != _prednames.end()) return it->second;
		else return 0;
	}
}

Function* Vocabulary::func(const string& name) const {
	Function* f = stdbuiltin()->func(name);
	if(f) return f;
	else {
		map<string,Function*>::const_iterator it = _funcnames.find(name);
		if(it != _funcnames.end()) return it->second;
		else return 0;
	}
}

vector<Predicate*> Vocabulary::pred_no_arity(const string& name) const {
	vector<Predicate*> vp = stdbuiltin()->pred_no_arity(name);
	for(unsigned int n = 0; n < _vpredicates.size(); ++n) {
		string pn = _vpredicates[n]->name();
		if(pn.substr(0,pn.find('/')) == name) vp.push_back(_vpredicates[n]);
	}
	return vp;
}

vector<Function*> Vocabulary::func_no_arity(const string& name) const {
	vector<Function*> vf = stdbuiltin()->func_no_arity(name);
	for(unsigned int n = 0; n < _vfunctions.size(); ++n) {
		string fn = _vfunctions[n]->name();
		if(fn.substr(0,fn.find('/')) == name) vf.push_back(_vfunctions[n]);
	}
	return vf;
}

/** Debugging **/

string Vocabulary::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "Vocabulary " + _name + ":\n";
	s = s + tab + "  Sorts:\n";
	for(unsigned int n = 0; n < nrSorts(); ++n) {
		s = s + tab + "    " + sort(n)->name() + '\n';
	}
	s = s + tab + "  Predicates:\n";
	for(unsigned int n = 0; n < nrPreds(); ++n) {
		s = s + tab + "    " + pred(n)->name() + '\n';
	}
	s = s + tab + "  Functions:\n";
	for(unsigned int n = 0; n < nrFuncs(); ++n) {
		s = s + tab + "    " + func(n)->name() + '\n';
	}
	return s;
}
