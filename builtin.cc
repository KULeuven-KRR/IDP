/************************************
	builtin.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "builtin.h"
#include "structure.h"
#include <iostream>
#include <typeinfo>
#include <cmath>

extern bool isDouble(const string&);
extern bool isInt(const string&);
extern bool isInt(double);
extern bool isChar(int);
extern bool isChar(double);
extern double stod(const string&);
extern int nrOfChars();
extern string tabstring(unsigned int);

/************************* 
	Built-in sorts 
*************************/

class BuiltInSort : public Sort {
	
	private:

		SortTable*	_inter;		// The interpretation of the sort

	public:

		// Constructors
		BuiltInSort(const string& name, SortTable* t) : Sort(name,0), _inter(t) { }

		// Destructor
		~BuiltInSort() { delete(_inter);	}

		// Inspectors
		bool		builtin()	const { return true;	}
		SortTable*	inter()		const { return _inter;	}

};

/************************** 
	Built-in predicates
**************************/

class BuiltInPredicate : public Predicate {

	private:

		PredInter*	(*_inter)(const vector<SortTable*>&);

	public:

		// Constructors
		BuiltInPredicate(const string& n, const vector<Sort*>& vs, PredInter* (*it)(const vector<SortTable*>&)) : 
			Predicate(n,vs,0) { _inter = it; }

		// Destructor
		~BuiltInPredicate() { }

		// Inspectors
		bool		builtin()							const { return true;		}
		PredInter*	inter(const vector<SortTable*>& vs)	const { return _inter(vs);	}
	
};


/************************* 
	Built-in functions 
*************************/

class BuiltInFunction : public Function {
	
	private:

		FuncInter*	(*_inter)(const vector<SortTable*>&);

	public:

		// Constructors
		BuiltInFunction(const string& n, const vector<Sort*>& is, Sort* os, FuncInter* (*ft)(const vector<SortTable*>&)) : 
			Function(n,is,os,0) { _inter = ft; }
		BuiltInFunction(const string& n, const vector<Sort*>& s, FuncInter* (*ft)(const vector<SortTable*>&)) : 
			Function(n,s,0) { _inter = ft; }

		// Destructor
		~BuiltInFunction() { }

		// Inspectors
		bool		builtin()							const { return true;		}
		FuncInter*	inter(const vector<SortTable*>& s)	const { return _inter(s);	}

};


/****************************
	Overloaded predicates
****************************/

class OverloadedPredicate : public Predicate {

	private:
		vector<Sort*>							(*_deriveSort)(const vector<Sort*>&);
		PredInter*								(*_inter)(const vector<SortTable*>&);
		map<vector<Sort*>,BuiltInPredicate*>	_children;	// Maps a tuple of sorts to the overloaded predicate

	public:

		// Constructors
		OverloadedPredicate(const string& n, unsigned int ar, vector<Sort*> (*ds)(const vector<Sort*>&), PredInter* (*di)(const vector<SortTable*>&)) : 
			Predicate(n,vector<Sort*>(ar,0),0) { _deriveSort = ds; _inter = di; }

		// Destructor
		~OverloadedPredicate();

		// Inspectors
		bool		overloaded()	const { return true;	}
		bool		builtin()		const { return true;	}
		Predicate*	disambiguate(const vector<Sort*>&);

};

OverloadedPredicate::~OverloadedPredicate() {
	for(map<vector<Sort*>,BuiltInPredicate*>::iterator it = _children.begin(); it != _children.end(); ++it) 
		delete(it->second);
}

/** Sort derivation for overloaded predicate symbols **/

/** Possibilty 1: 
 *		If the given sorts are (a_1,...,a_n) and none of them is equal to 0 and they have the same base sort,
 *		return the sorts (b,...,b), where b = resolve(a_1,...,a_n) 
 *		Else, let a_i be the first sort that is not equal to 0. If a_i is a base sort and has no subsorts 
 *		and is equal to all other non-zero sorts among (a_1,...,a_n), return the sorts (a_i,...,a_i).
 *		Else return (0,...,0).
 *
 *		Used for predicates: =/2, </2, >/2, SUCC/2
 */
vector<Sort*> overloaded_predicate_deriver1(const vector<Sort*>& vs) {
	Sort* s = 0;
	bool containszeros = false;
	for(unsigned int n = 0; n < vs.size(); ++n) {
		if(vs[n]) {
			if(s) {
				s = SortUtils::resolve(s,vs[n]);
				if(!s) return vector<Sort*>(vs.size(),0);
			}
			else s = vs[n];
		}
		else containszeros = true;
	}
	if(s && containszeros) {
		if(s->parent() || s->nrChildren()) return vector<Sort*>(vs.size(),0);
	}
	return vector<Sort*>(vs.size(),s);
}

Predicate* OverloadedPredicate::disambiguate(const vector<Sort*>& vs) {
	vector<Sort*> vsd = _deriveSort(vs);
	assert(!vsd.empty());
	if(!vsd[0]) return 0;
	map<vector<Sort*>,BuiltInPredicate*>::iterator it = _children.find(vsd);
	if(it != _children.end()) return it->second;
	else {
		BuiltInPredicate* bip = new BuiltInPredicate(_name,vsd,_inter);
		_children[vsd] = bip;
		return bip;
	}
	return 0;
}

/***************************
	Overloaded functions
***************************/

class OverloadedFunction : public Function {

	private:
		vector<Sort*>						(*_deriveSort)(const vector<Sort*>&);
		FuncInter*							(*_inter)(const vector<SortTable*>&);
		map<vector<Sort*>,BuiltInFunction*>	_children;	// Maps a tuple of sorts to the overloaded function

	public:

		// Constructors
		OverloadedFunction(const string& n, unsigned int ar, vector<Sort*> (*ds)(const vector<Sort*>&), FuncInter* (*di)(const vector<SortTable*>&)) : 
			Function(n,vector<Sort*>(ar+1,0),0) { _deriveSort = ds; _inter = di;	}

		// Destructor
		~OverloadedFunction();

		// Inspectors
		bool		overloaded()	const { return true;	}
		bool		builtin()		const { return true;	}
		Function*	disambiguate(const vector<Sort*>&);
};

OverloadedFunction::~OverloadedFunction() {
	for(map<vector<Sort*>,BuiltInFunction*>::iterator it = _children.begin(); it != _children.end(); ++it) 
		delete(it->second);
}

/** Sort derivation for overloaded predicate symbols **/

/** Possibility 1
 *		If all but the last argument are subsorts of int, derive (int,...,int)
 *		Else if one of the arguments is a subsort of float, but not of int, derive (float,....,float)
 *		
 *		Used for functions +, -, *, /, abs 
 */
vector<Sort*> overloaded_function_deriver1(const vector<Sort*>& vs) {
	bool deriveint = true;
	for(unsigned int n = 0; n < vs.size()-1; ++n) {
		if(vs[n]) {
			if(vs[n]->base() != Builtin::floatsort()) return vector<Sort*>(vs.size(),0);
			else {
				Sort* temp = vs[n];
				while(temp) {
					if(temp == Builtin::intsort()) break;
					temp = temp->parent();
				}
				if(!temp) deriveint = false;
			}
		}
	}
	if(deriveint) return vector<Sort*>(vs.size(),Builtin::intsort());
	else return vector<Sort*>(vs.size(),Builtin::floatsort());
}

/** Possibility 2
 *		If the incoming vector contains a 0, return (0,...,0).
 *		Else return the incoming vector.
 *
 *		Used for functions MIN and MAX
 */
vector<Sort*> overloaded_function_deriver2(const vector<Sort*>& vs) {
	for(unsigned int n = 0; n < vs.size(); ++n) {
		if(!vs[n]) return vector<Sort*>(vs.size(),0);
	}
	return vs;
}

/** Possibility 3
 *		Derive that the last argument is float
 *
 *		Used for function ^
 */
vector<Sort*> overloaded_function_deriver3(const vector<Sort*>& vs) {
	vector<Sort*> vsn = vs;
	vsn.back() = Builtin::floatsort();
	for(unsigned int n = 0; n < vs.size() - 1; ++n) {
		if(vs[n] == 0) return vector<Sort*>(vs.size(),0);
	}
	return vsn;
}

Function* OverloadedFunction::disambiguate(const vector<Sort*>& vs) {
	vector<Sort*> vsd = _deriveSort(vs);
	assert(!vsd.empty());
	if(!vsd[0]) return 0;
	map<vector<Sort*>,BuiltInFunction*>::iterator it = _children.find(vsd);
	if(it != _children.end()) return it->second;
	else {
		BuiltInFunction* bif = new BuiltInFunction(_name,vsd,_inter);
		_children[vsd] = bif;
		return bif;
	}
}

/***************************
	Built-in sort tables
***************************/

/** All integers **/

class AllIntSortTable : public SortTable {

	public:
		
		bool			finite()					const { return false;				}
		unsigned int	size()						const { assert(false); return 0;	}
		bool			empty()						const { return false;				}
		bool			contains(const string& s)	const { return isInt(s);			}
		bool			contains(int)				const { return true;				}
		bool			contains(double d)			const { return isInt(d);			}
		Element			element(unsigned int n)			  { assert(false); Element e; return e;	}
		ElementType		type()						const { return ELINT;				}
		unsigned int	position(Element,ElementType)	const { assert(false); return 0;	}

		string	to_string()	const { return "all integers"; }
		
};

/** All floating point numbers **/

class AllFloatSortTable : public SortTable {
	
	public:
		
		bool			finite()					const { return false;				}
		unsigned int	size()						const { assert(false); return 0;	}
		bool			empty()						const { return false;				}
		bool			contains(const string& s)	const { return isDouble(s);			}
		bool			contains(int)				const { return true;				}
		bool			contains(double)			const { return true;				}
		Element			element(unsigned int n)			  { assert(false); Element e; return e;	}
		ElementType		type()						const { return ELDOUBLE;			}
		unsigned int	position(Element,ElementType)	const { assert(false); return 0;	}

		string	to_string()	const { return "all floats"; }
		
};

/** All strings **/

class AllStringSortTable : public SortTable {
		
	public:
		
		bool			finite()					const { return false;				}
		unsigned int	size()						const { assert(false); return 0;	}
		bool			empty()						const { return false;				}
		bool			contains(const string& s)	const { return true;				}
		bool			contains(int)				const { return true;				}
		bool			contains(double)			const { return true;				}
		Element			element(unsigned int n)			  { assert(false); Element e; return e;	}
		ElementType		type()						const { return ELSTRING;			}
		unsigned int	position(Element,ElementType)	const { assert(false); return 0;	}

		string	to_string()	const { return "all strings"; }
		
};

/** All characters **/

class AllCharSortTable : public SortTable {
		
	public:
		
		bool			finite()						const { return true;				}
		unsigned int	size()							const {	return nrOfChars();			}
		bool			empty()							const { return false;				}
		bool			contains(const string& s)		const { return (s.size() == 1);		}
		bool			contains(int n)					const { return isChar(n);			}
		bool			contains(double d)				const { return isChar(d);			}
		Element			element(unsigned int n)			      { Element e; e._string = new string(1,char(n)); return e;	}
		ElementType		type()							const { return ELSTRING;			}
		unsigned int	position(Element,ElementType)	const { assert(false); return 0; 	} // TODO?

		string	to_string()	const { return "all characters"; }
		
};

/*******************************
	Builtin predicate tables
*******************************/

/** Equality **/

class EqualPredTable : public PredTable {
	private:
		ElementType	_type;
		SortTable*	_left;
		SortTable*	_right;
	public:
		EqualPredTable(ElementType t, SortTable* l, SortTable* r) : 
			PredTable(vector<ElementType>(2,t)), _type(t), _left(l), _right(r) { }
		bool			finite()								const { return (_left->finite() || _right->finite());	}
		unsigned int	size()									const;
		bool			empty()									const { return (_left->empty() || _right->empty());		}
		vector<Element>	tuple(unsigned int n)					const;
		Element			element(unsigned int r, unsigned int c)	const;
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;
};

unsigned int EqualPredTable::size() const {
	assert(false); // TODO?
	return 0;
}

vector<Element> EqualPredTable::tuple(unsigned int n) const {
	assert(false); // TODO?
	vector<Element> ve(0);
	return ve;
}

Element EqualPredTable::element(unsigned int r, unsigned int c) const {
	assert(false); // TODO?
	Element e;
	return e;
}

bool EqualPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			return ve[0]._int == ve[1]._int;
		case ELDOUBLE:
			return (*(ve[0]._double)) == (*(ve[1]._double));
		case ELSTRING:
			return (*(ve[0]._string)) == (*(ve[1]._string));
		default:
			assert(false); return false;
	}
}

string EqualPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "=/2";
}

PredInter*	equalinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	ElementType t = ElementUtil::resolve(vs[0]->type(),vs[1]->type());
	PredTable* pt = new EqualPredTable(t,vs[0],vs[1]);
	return new PredInter(pt,true);
}

/** Strictly less than **/

class StrLessThanPredTable : public PredTable {
	private:
		ElementType _type;
		SortTable*	_left;
		SortTable*	_right;
	public:
		StrLessThanPredTable(ElementType t, SortTable* l, SortTable* r) : 
			PredTable(vector<ElementType>(2,t)), _type(t), _left(l), _right(r) { }
		bool			finite()								const;
		unsigned int	size()									const;
		bool			empty()									const;
		vector<Element>	tuple(unsigned int n)					const;
		Element			element(unsigned int r, unsigned int c)	const;
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;

};

bool StrLessThanPredTable::finite() const {
	if(_left->finite() && _right->finite()) return true;
	else {
		assert(false); // TODO?
		return false;
	}
}

bool StrLessThanPredTable::empty() const {
	assert(false); // TODO?
	return false;
}

unsigned int StrLessThanPredTable::size() const {
	assert(false); // TODO?
	return 0;
}

vector<Element> StrLessThanPredTable::tuple(unsigned int n) const {
	assert(false); // TODO?
	vector<Element> ve(0);
	return ve;
}

Element StrLessThanPredTable::element(unsigned int r, unsigned int c) const {
	assert(false); // TODO?
	Element e;
	return e;
}

bool StrLessThanPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			return ve[0]._int < ve[1]._int;
		case ELDOUBLE:
			return (*(ve[0]._double)) < (*(ve[1]._double));
		case ELSTRING:
			if(isDouble(*(ve[0]._string))) {
				if(isDouble(*(ve[1]._string))) return (stod(*(ve[0]._string)) < stod(*(ve[1]._string)));
				else return true;
			}
			else if(isDouble(*(ve[1]._string))) return false;
			else return (*(ve[0]._string)) < (*(ve[1]._string));
		default:
			assert(false); return false;
	}
}

string StrLessThanPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "</2";
}

PredInter*	strlessinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	ElementType t = ElementUtil::resolve(vs[0]->type(),vs[1]->type());
	PredTable* pt = new StrLessThanPredTable(t,vs[0],vs[1]);
	return new PredInter(pt,true);
}


/** Strictly greater than **/

class StrGreaterThanPredTable : public PredTable {
	private:
		ElementType _type;
		SortTable*	_left;
		SortTable*	_right;
	public:
		StrGreaterThanPredTable(ElementType t, SortTable* l, SortTable* r) : 
			PredTable(vector<ElementType>(2,t)), _type(t), _left(l), _right(r) { }
		bool			finite()								const;
		unsigned int	size()									const;
		bool			empty()									const;
		vector<Element>	tuple(unsigned int n)					const;
		Element			element(unsigned int r, unsigned int c)	const;
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;

};

bool StrGreaterThanPredTable::finite() const {
	if(_left->finite() && _right->finite()) return true;
	else {
		assert(false); // TODO?
		return false;
	}
}

bool StrGreaterThanPredTable::empty() const {
	assert(false); // TODO?
	return false;
}

unsigned int StrGreaterThanPredTable::size() const {
	assert(false); // TODO?
	return 0;
}

vector<Element> StrGreaterThanPredTable::tuple(unsigned int n) const {
	assert(false); // TODO?
	vector<Element> ve(0);
	return ve;
}

Element StrGreaterThanPredTable::element(unsigned int r, unsigned int c) const {
	assert(false); // TODO?
	Element e;
	return e;
}

bool StrGreaterThanPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			return ve[0]._int < ve[1]._int;
		case ELDOUBLE:
			return (*(ve[0]._double)) < (*(ve[1]._double));
		case ELSTRING:
			if(isDouble(*(ve[0]._string))) {
				if(isDouble(*(ve[1]._string))) return (stod(*(ve[0]._string)) < stod(*(ve[1]._string)));
				else return true;
			}
			else if(isDouble(*(ve[1]._string))) return false;
			else return (*(ve[0]._string)) < (*(ve[1]._string));
		default:
			assert(false); return false;
	}
}

string StrGreaterThanPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + ">/2";
}

PredInter*	strgreaterinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	ElementType t = ElementUtil::resolve(vs[0]->type(),vs[1]->type());
	PredTable* pt = new StrGreaterThanPredTable(t,vs[0],vs[1]);
	return new PredInter(pt,true);
}

/** Successor **/

class SuccPredTable : public PredTable {
	private:
		SortTable*	_table;
	public:
		SuccPredTable(SortTable* t) : PredTable(vector<ElementType>(2,t->type())), _table(t) { }
		bool			finite()								const { return _table->finite();	}
		unsigned int	size()									const { return _table->size() - 1;	}
		bool			empty()									const { return (size() == 0);		}
		vector<Element>	tuple(unsigned int n)					const;
		Element			element(unsigned int r, unsigned int c)	const;
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;

};

bool SuccPredTable::contains(const vector<Element>& ve) const {
	if(_table->finite()) {
		unsigned int p1 = _table->position(ve[0],_table->type());
		unsigned int p2 = _table->position(ve[1],_table->type());
		return (p2 == p1+1);
	}
	else if(typeid(*_table) == typeid(AllIntSortTable)) {
		return (ve[1]._int == ve[0]._int + 1);
	}
	else if(typeid(*_table) == typeid(AllCharSortTable)) {
		char c1 = (*(ve[0]._string))[0];
		++c1;
		return (c1 == (*(ve[1]._string))[0]);
	}
	assert(false);
	return false;
}

vector<Element> SuccPredTable::tuple(unsigned int n) const {
	vector<Element> ve(2);
	ve[0] = element(n,0);
	ve[1] = element(n,1);
	return ve;
}

Element	SuccPredTable::element(unsigned int r, unsigned int c) const {
	if(c == 0) return _table->element(r);
	else return _table->element(r+1);
}

string SuccPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "SUCC/2";
}

PredInter*	succinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	assert(vs[0] == vs[1]);
	PredTable* pt = new SuccPredTable(vs[0]);
	return new PredInter(pt,true);
}


/******************************
	Builtin function tables
******************************/

/** Addition **/

class PlusPredTable : public PredTable {
	private:
		ElementType	_type;
	public:
		PlusPredTable(ElementType t) : PredTable(vector<ElementType>(3,t)), _type(t) { }
		bool			finite()								const { return false;				}
		unsigned int	size()									const { assert(false); return 0;	}
		bool			empty()									const { return false;				}
		vector<Element>	tuple(unsigned int n)					const { assert(false); return vector<Element>(0);	}
		Element			element(unsigned int r, unsigned int c)	const { assert(false); Element e; return e;			}
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;
};

bool PlusPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			return (ve[0]._int + ve[1]._int == ve[2]._int);
		case ELDOUBLE:
			return ((*(ve[0]._double)) + (*(ve[1]._double)) == (*(ve[2]._double)));
		default:
			assert(false); return false;
	}
}

string PlusPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "+/3";
}

class PlusFuncInter : public FuncInter {
	private: 
		ElementType			_type;		// int or double
		PredInter*			_predinter;
		mutable map<double,double*>	_memory;
	public:
		PlusFuncInter(ElementType t);

		Element operator[](const vector<Element>& vi)		const;
		PredInter*	predinter()								const { return _predinter;	}
		string to_string(unsigned int spaces = 0)			const;
};

PlusFuncInter::PlusFuncInter(ElementType t) : FuncInter(vector<ElementType>(2,t),t), _type(t) {
	PlusPredTable* ppt = new PlusPredTable(t);
	_predinter = new PredInter(ppt,true);
}

Element PlusFuncInter::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = vi[0]._int + vi[1]._int;
			break;
		case ELDOUBLE:
		{
			double d = (*(vi[0]._double)) + (*(vi[1]._double));
			map<double,double*>::iterator it = _memory.find(d);
			if(it != _memory.end()) e._double = it->second;
			else {
				e._double = new double(d);
				_memory[d] = e._double;
			}
		}
		default: assert(false);
	}
	return e;
}

string PlusFuncInter::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "+/2";
}

FuncInter* plusfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 3);
	assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	return new PlusFuncInter(vs[0]->type());
}

/** Subtraction **/

class MinusPredTable : public PredTable {
	private:
		ElementType	_type;
	public:
		MinusPredTable(ElementType t) : PredTable(vector<ElementType>(3,t)), _type(t) { }
		bool			finite()								const { return false;				}
		unsigned int	size()									const { assert(false); return 0;	}
		bool			empty()									const { return false;				}
		vector<Element>	tuple(unsigned int n)					const { assert(false); return vector<Element>(0);	}
		Element			element(unsigned int r, unsigned int c)	const { assert(false); Element e; return e;			}
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;
};

bool MinusPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			return (ve[0]._int - ve[1]._int == ve[2]._int);
		case ELDOUBLE:
			return ((*(ve[0]._double)) - (*(ve[1]._double)) == (*(ve[2]._double)));
		default:
			assert(false); return false;
	}
}

string MinusPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "-/3";
}

class MinusFuncInter : public FuncInter {
	private: 
		ElementType			_type;		// int or double
		PredInter*			_predinter;
		mutable map<double,double*>	_memory;
	public:
		MinusFuncInter(ElementType t);

		Element operator[](const vector<Element>& vi)		const;
		PredInter*	predinter()								const { return _predinter;	}
		string to_string(unsigned int spaces = 0)			const;
};

MinusFuncInter::MinusFuncInter(ElementType t) : FuncInter(vector<ElementType>(2,t),t), _type(t) {
	MinusPredTable* ppt = new MinusPredTable(t);
	_predinter = new PredInter(ppt,true);
}

Element MinusFuncInter::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = vi[0]._int - vi[1]._int;
			break;
		case ELDOUBLE:
		{
			double d = (*(vi[0]._double)) - (*(vi[1]._double));
			map<double,double*>::iterator it = _memory.find(d);
			if(it != _memory.end()) e._double = it->second;
			else {
				e._double = new double(d);
				_memory[d] = e._double;
			}
		}
		default: assert(false);
	}
	return e;
}

string MinusFuncInter::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "-/2";
}

FuncInter* minusfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 3);
	assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	return new MinusFuncInter(vs[0]->type());
}

/** Multiplication **/

class TimesPredTable : public PredTable {
	private:
		ElementType	_type;
	public:
		TimesPredTable(ElementType t) : PredTable(vector<ElementType>(3,t)), _type(t) { }
		bool			finite()								const { return false;				}
		unsigned int	size()									const { assert(false); return 0;	}
		bool			empty()									const { return false;				}
		vector<Element>	tuple(unsigned int n)					const { assert(false); return vector<Element>(0);	}
		Element			element(unsigned int r, unsigned int c)	const { assert(false); Element e; return e;			}
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;
};

bool TimesPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			return (ve[0]._int * ve[1]._int == ve[2]._int);
		case ELDOUBLE:
			return ((*(ve[0]._double)) * (*(ve[1]._double)) == (*(ve[2]._double)));
		default:
			assert(false); return false;
	}
}

string TimesPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "*/3";
}

class TimesFuncInter : public FuncInter {
	private: 
		ElementType			_type;		// int or double
		PredInter*			_predinter;
		mutable map<double,double*>	_memory;
	public:
		TimesFuncInter(ElementType t);

		Element operator[](const vector<Element>& vi)		const;
		PredInter*	predinter()								const { return _predinter;	}
		string to_string(unsigned int spaces = 0)			const;
};

TimesFuncInter::TimesFuncInter(ElementType t) : FuncInter(vector<ElementType>(2,t),t), _type(t) {
	TimesPredTable* ppt = new TimesPredTable(t);
	_predinter = new PredInter(ppt,true);
}

Element TimesFuncInter::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = vi[0]._int * vi[1]._int;
			break;
		case ELDOUBLE:
		{
			double d = (*(vi[0]._double)) * (*(vi[1]._double));
			map<double,double*>::iterator it = _memory.find(d);
			if(it != _memory.end()) e._double = it->second;
			else {
				e._double = new double(d);
				_memory[d] = e._double;
			}
		}
		default: assert(false);
	}
	return e;
}

string TimesFuncInter::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "*/2";
}

FuncInter* timesfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 3);
	assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	return new TimesFuncInter(vs[0]->type());
}

/** Division **/

class DivPredTable : public PredTable {
	private:
		ElementType	_type;
	public:
		DivPredTable(ElementType t) : PredTable(vector<ElementType>(3,t)), _type(t) { }
		bool			finite()								const { return false;				}
		unsigned int	size()									const { assert(false); return 0;	}
		bool			empty()									const { return false;				}
		vector<Element>	tuple(unsigned int n)					const { assert(false); return vector<Element>(0);	}
		Element			element(unsigned int r, unsigned int c)	const { assert(false); Element e; return e;			}
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;
};

bool DivPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			if(ve[1]._int == 0) return false;
			else return (ve[0]._int / ve[1]._int == ve[2]._int);
		case ELDOUBLE:
			if(*(ve[1]._double) == 0) return false; 
			else return ((*(ve[0]._double)) / (*(ve[1]._double)) == (*(ve[2]._double)));
		default:
			assert(false); return false;
	}
}

string DivPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "//3";
}

class DivFuncInter : public FuncInter {
	private: 
		ElementType			_type;		// int or double
		PredInter*			_predinter;
		mutable map<double,double*>	_memory;
	public:
		DivFuncInter(ElementType t);

		Element operator[](const vector<Element>& vi)		const;
		PredInter*	predinter()								const { return _predinter;	}
		string to_string(unsigned int spaces = 0)			const;
};

DivFuncInter::DivFuncInter(ElementType t) : FuncInter(vector<ElementType>(2,t),t), _type(t) {
	DivPredTable* ppt = new DivPredTable(t);
	_predinter = new PredInter(ppt,true);
}

Element DivFuncInter::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			if(vi[1]._int == 0) return ElementUtil::nonexist(ELINT);
			else e._int = vi[0]._int / vi[1]._int;
			break;
		case ELDOUBLE:
		{
			if((*(vi[1]._double)) == 0) return ElementUtil::nonexist(ELDOUBLE);
			else {double d = (*(vi[0]._double)) * (*(vi[1]._double));
				map<double,double*>::iterator it = _memory.find(d);
				if(it != _memory.end()) e._double = it->second;
				else {
					e._double = new double(d);
					_memory[d] = e._double;
				}
			}
		}
		default: assert(false);
	}
	return e;
}

string DivFuncInter::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "//2";
}

FuncInter* divfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 3);
	assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	return new DivFuncInter(vs[0]->type());
}

/** Exponentiation **/

class ExpPredTable : public PredTable {
	public:
		ExpPredTable(const vector<ElementType>& vet) : PredTable(vet) { }
		bool			finite()								const { return false;				}
		unsigned int	size()									const { assert(false); return 0;	}
		bool			empty()									const { return false;				}
		vector<Element>	tuple(unsigned int n)					const { assert(false); return vector<Element>(0);	}
		Element			element(unsigned int r, unsigned int c)	const { assert(false); Element e; return e;			}
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;
};

bool ExpPredTable::contains(const vector<Element>& ve) const {
	vector<double> vd(3);
	vd[0] = (_types[0] == ELINT) ? double(ve[0]._int) : (*(ve[0]._double));
	vd[1] = (_types[1] == ELINT) ? double(ve[1]._int) : (*(ve[1]._double));
	vd[2] = (*(ve[2]._double));
	return (pow(vd[0],vd[1]) == vd[2]);
}

string ExpPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "^/3";
}

class ExpFuncInter : public FuncInter {
	private: 
		PredInter*			_predinter;
		mutable map<double,double*>	_memory;
	public:
		ExpFuncInter(const vector<ElementType>&,ElementType);

		Element operator[](const vector<Element>& vi)		const;
		PredInter*	predinter()								const { return _predinter;	}
		string to_string(unsigned int spaces = 0)			const;
};

ExpFuncInter::ExpFuncInter(const vector<ElementType>& vet, ElementType t) : FuncInter(vet,t) {
	vector<ElementType> ve = vet; ve.push_back(t);
	ExpPredTable* ppt = new ExpPredTable(ve);
	_predinter = new PredInter(ppt,true);
}

Element ExpFuncInter::operator[](const vector<Element>& ve) const {
	double d1 = (_intypes[0] == ELINT) ? double(ve[0]._int) : (*(ve[0]._double));
	double d2 = (_intypes[1] == ELINT) ? double(ve[1]._int) : (*(ve[1]._double));
	double res = pow(d1,d2);
	Element e;
	map<double,double*>::iterator it = _memory.find(res);
	if(it != _memory.end()) e._double = it->second;
	else {
		e._double = new double(res);
		_memory[res] = e._double;
	}
	return e;
}

string ExpFuncInter::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "^/2";
}

FuncInter* expfuncinter(const vector<SortTable*>& vs) {
	vector<ElementType> vet(2);
	vet[0] = vs[0]->type();
	vet[1] = vs[1]->type();
	return new ExpFuncInter(vet,ELDOUBLE);
}

/** Absolute value **/

class AbsPredTable : public PredTable {
	private:
		ElementType	_type;
	public:
		AbsPredTable(ElementType t) : PredTable(vector<ElementType>(2,t)), _type(t) { }
		bool			finite()								const { return false;				}
		unsigned int	size()									const { assert(false); return 0;	}
		bool			empty()									const { return false;				}
		vector<Element>	tuple(unsigned int n)					const { assert(false); return vector<Element>(0);	}
		Element			element(unsigned int r, unsigned int c)	const { assert(false); Element e; return e;			}
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;
};

bool AbsPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			return (abs(ve[0]._int) ==  ve[1]._int);
		case ELDOUBLE:
			return (fabs(*(ve[0]._double)) == (*(ve[1]._double)));
		default:
			assert(false); return false;
	}
}

string AbsPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "abs/2";
}

class AbsFuncInter : public FuncInter {
	private: 
		ElementType			_type;		// int or double
		PredInter*			_predinter;
		mutable map<double,double*>	_memory;
	public:
		AbsFuncInter(ElementType t);

		Element operator[](const vector<Element>& vi)		const;
		PredInter*	predinter()								const { return _predinter;	}
		string to_string(unsigned int spaces = 0)			const;
};

AbsFuncInter::AbsFuncInter(ElementType t) : FuncInter(vector<ElementType>(1,t),t), _type(t) {
	AbsPredTable* ppt = new AbsPredTable(t);
	_predinter = new PredInter(ppt,true);
}

Element AbsFuncInter::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = abs(vi[0]._int);
			break;
		case ELDOUBLE:
		{
			double d = fabs(*(vi[0]._double));
			map<double,double*>::iterator it = _memory.find(d);
			if(it != _memory.end()) e._double = it->second;
			else {
				e._double = new double(d);
				_memory[d] = e._double;
			}
		}
		default: assert(false);
	}
	return e;
}

string AbsFuncInter::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "abs/1";
}

FuncInter* absfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	assert(vs[0] == vs[1]);
	return new TimesFuncInter(vs[0]->type());
}

/** Unary minus **/

class UminPredTable : public PredTable {
	private:
		ElementType	_type;
	public:
		UminPredTable(ElementType t) : PredTable(vector<ElementType>(2,t)), _type(t) { }
		bool			finite()								const { return false;				}
		unsigned int	size()									const { assert(false); return 0;	}
		bool			empty()									const { return false;				}
		vector<Element>	tuple(unsigned int n)					const { assert(false); return vector<Element>(0);	}
		Element			element(unsigned int r, unsigned int c)	const { assert(false); Element e; return e;			}
		bool			contains(const vector<Element>&)		const;
		string			to_string(unsigned int spaces = 0)		const;
};

bool UminPredTable::contains(const vector<Element>& ve) const {
	switch(_type) {
		case ELINT:
			return (-(ve[0]._int) ==  ve[1]._int);
		case ELDOUBLE:
			return (-(*(ve[0]._double)) == (*(ve[1]._double)));
		default:
			assert(false); return false;
	}
}

string UminPredTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "-/2";
}

class UMinFuncInter : public FuncInter {
	private: 
		ElementType			_type;		// int or double
		PredInter*			_predinter;
		mutable map<double,double*>	_memory;
	public:
		UMinFuncInter(ElementType t);

		Element operator[](const vector<Element>& vi)		const;
		PredInter*	predinter()								const { return _predinter;	}
		string to_string(unsigned int spaces = 0)			const;
};

UMinFuncInter::UMinFuncInter(ElementType t) : FuncInter(vector<ElementType>(1,t),t), _type(t) {
	UminPredTable* ppt = new UminPredTable(t);
	_predinter = new PredInter(ppt,true);
}

Element UMinFuncInter::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = -(vi[0]._int);
			break;
		case ELDOUBLE:
		{
			double d = -(*(vi[0]._double));
			map<double,double*>::iterator it = _memory.find(d);
			if(it != _memory.end()) e._double = it->second;
			else {
				e._double = new double(d);
				_memory[d] = e._double;
			}
		}
		default: assert(false);
	}
	return e;
}

string UMinFuncInter::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	return s + "-/1";
}

FuncInter* uminfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	assert(vs[0] == vs[1]);
	return new TimesFuncInter(vs[0]->type());
}

/** Minimum and maximum **/

FuncInter* minimumfuncinter(const vector<SortTable*>& vs) {
	assert(vs[0]->finite());
	UserPredTable* upt = new UserPredTable(vector<ElementType>(1,vs[0]->type()));
	upt->addRow();
	if(vs[0]->empty()) (*upt)[0][0] = ElementUtil::nonexist(vs[0]->type());
	else (*upt)[0][0] = ElementUtil::clone(vs[0]->element(0),vs[0]->type());
	PredInter* pt = new PredInter(upt,true);
	return new UserFuncInter(vector<ElementType>(0),vs[0]->type(),pt,upt);
}

FuncInter* maximumfuncinter(const vector<SortTable*>& vs) {
	assert(vs[0]->finite());
	assert(vs[0]->finite());
	UserPredTable* upt = new UserPredTable(vector<ElementType>(1,vs[0]->type()));
	upt->addRow();
	if(vs[0]->empty()) (*upt)[0][0] = ElementUtil::nonexist(vs[0]->type());
	else (*upt)[0][0] = ElementUtil::clone(vs[0]->element(vs[0]->size() - 1),vs[0]->type());
	PredInter* pt = new PredInter(upt,true);
	return new UserFuncInter(vector<ElementType>(0),vs[0]->type(),pt,upt);
}



/**********************
	Built-in symbols
**********************/

namespace Builtin {

	/** Built-in sorts **/
	BuiltInSort*	_intsort;	
	BuiltInSort*	_floatsort;	
	BuiltInSort*	_stringsort;
	BuiltInSort*	_charsort;	

	Sort*	intsort()		{ return _intsort;		}
	Sort*	floatsort()		{ return _floatsort;	}
	Sort*	stringsort()	{ return _stringsort;	}
	Sort*	charsort()		{ return _charsort;		}

	/** Built-in predicates **/
	OverloadedPredicate* _equalpred;			// =/2
	OverloadedPredicate* _strlessthanpred;		// </2
	OverloadedPredicate* _strgreaterthanpred;	// >/2
	OverloadedPredicate* _successorpred;		// SUCC/2
	
	/** Built-in functions **/
	OverloadedFunction*	_plusfunc;			// +/2
	OverloadedFunction* _minusfunc;			// -/2
	OverloadedFunction* _timesfunc;			// */2
	OverloadedFunction*	_divfunc;			// //2
	OverloadedFunction* _expfunc;			// ^/2
	OverloadedFunction*	_absfunc;			// abs/1
	OverloadedFunction* _uminusfunc;		// -/1
	OverloadedFunction* _minfunc;			// MIN/0
	OverloadedFunction* _maxfunc;			// MAX/0

	/** Built-in symbol lists **/
	map<string,Sort*>		_builtinsorts;
	map<string,Predicate*>	_builtinpreds;
	map<string,Function*>	_builtinfuncs;

	/** Built-in interpretations **/
	map<Predicate*,map<vector<SortTable*>,PredInter*> >	_predinters;
	map<Function*,map<vector<SortTable*>,FuncInter*> >	_funcinters;

	/** Return symbol with given name **/
	Sort* sort(const string& n) {
		map<string,Sort*>::iterator it = _builtinsorts.find(n);
		if(it == _builtinsorts.end()) return 0;
		else return it->second;
	}

	Predicate* pred(const string& n) {
		map<string,Predicate*>::iterator it = _builtinpreds.find(n);
		if(it == _builtinpreds.end()) return 0;
		else return it->second;
	}

	Function* func(const string& n) {
		map<string,Function*>::iterator it = _builtinfuncs.find(n);
		if(it == _builtinfuncs.end())  return 0;
		else return it->second;
	}

	vector<Predicate*> pred_no_arity(const string& name) {
		vector<Predicate*> vp;
		for(map<string,Predicate*>::iterator it = _builtinpreds.begin(); it != _builtinpreds.end(); ++it) {
			string pn = it->first;
			if(pn.substr(0,pn.find('/')) == name) vp.push_back(it->second);
		}
		return vp;
	}

	vector<Function*> func_no_arity(const string& name) {
		vector<Function*> vf;
		for(map<string,Function*>::iterator it = _builtinfuncs.begin(); it != _builtinfuncs.end(); ++it) {
			string fn = it->first;
			if(fn.substr(0,fn.find('/')) == name) vf.push_back(it->second);
		}
		return vf;
	}

	/** Initialization **/
	void initialize() {

		vector<Sort*> (*sd)(const vector<Sort*>&);

		// Built-in sorts
		_floatsort	= new BuiltInSort("float", new AllFloatSortTable());
		_intsort	= new BuiltInSort("int", new AllIntSortTable()); _intsort->parent(_floatsort);
		_stringsort	= new BuiltInSort("string", new AllStringSortTable());
		_charsort	= new BuiltInSort("char", new AllCharSortTable()); _charsort->parent(_stringsort);

		_builtinsorts["int"]	= _intsort;
		_builtinsorts["float"]	= _floatsort;
		_builtinsorts["char"]	= _charsort;
		_builtinsorts["string"]	= _stringsort;

		// Built-in predicates
		sd = &overloaded_predicate_deriver1;
		_equalpred			= new OverloadedPredicate("=/2",2,sd,&equalinter);
		_strlessthanpred	= new OverloadedPredicate("</2",2,sd,&strlessinter);
		_strgreaterthanpred	= new OverloadedPredicate(">/2",2,sd,&strgreaterinter);
		_successorpred		= new OverloadedPredicate("SUCC/2",2,sd,&succinter);

		_builtinpreds["=/2"]	= _equalpred;
		_builtinpreds["</2"]	= _strlessthanpred;
		_builtinpreds[">/2"]	= _strgreaterthanpred;
		_builtinpreds["SUCC/2"] = _successorpred;

		// Built-in functions
		sd = &overloaded_function_deriver1;
		_plusfunc	= new OverloadedFunction("+/2",2,sd,&plusfuncinter);
		_minusfunc	= new OverloadedFunction("-/2",2,sd,&minusfuncinter);
		_timesfunc	= new OverloadedFunction("*/2",2,sd,&timesfuncinter);
		_divfunc	= new OverloadedFunction("//2",2,sd,&divfuncinter);
		_absfunc	= new OverloadedFunction("abs/1",1,sd,&absfuncinter);
		_uminusfunc	= new OverloadedFunction("-/1",1,sd,&uminfuncinter);
		sd = &overloaded_function_deriver2;
		_minfunc	= new OverloadedFunction("MIN/0",0,sd,&minimumfuncinter);
		_maxfunc	= new OverloadedFunction("MAX/0",0,sd,&maximumfuncinter);
		sd = &overloaded_function_deriver3;
		_expfunc	= new OverloadedFunction("^/2",2,sd,&expfuncinter);

		_builtinfuncs["+/2"]	= _plusfunc;
		_builtinfuncs["-/2"]	= _minusfunc;
		_builtinfuncs["*/2"]	= _timesfunc;
		_builtinfuncs["//2"]	= _divfunc;
		_builtinfuncs["^/2"]	= _expfunc;
		_builtinfuncs["abs/1"]	= _absfunc;
		_builtinfuncs["-/1"]	= _uminusfunc;
		_builtinfuncs["MIN/0"]	= _minfunc;
		_builtinfuncs["MAX/0"]	= _maxfunc;

	}

	// Destruction
	void deleteAll() {
		for(map<string,Sort*>::iterator it = _builtinsorts.begin(); it != _builtinsorts.end(); ++it) 
			delete(it->second);
		for(map<string,Predicate*>::iterator it = _builtinpreds.begin(); it != _builtinpreds.end(); ++it) 
			delete(it->second);
		for(map<string,Function*>::iterator it = _builtinfuncs.begin(); it != _builtinfuncs.end(); ++it) 
			delete(it->second);
	}

	// Structure
	SortTable* inter(Sort* s) {
		assert(typeid(*s) == typeid(BuiltInSort));
		BuiltInSort* bs = dynamic_cast<BuiltInSort*>(s);
		return bs->inter();
	}

	PredInter* inter(Predicate* p, const vector<SortTable*>& t) {
		map<Predicate*,map<vector<SortTable*>,PredInter*> >::iterator it = _predinters.find(p);
		if(it != _predinters.end()) {
			map<vector<SortTable*>,PredInter*>::iterator jt = (it->second).find(t);
			if(jt != (it->second.end())) return jt->second;
		}
		assert(typeid(*p) == typeid(BuiltInPredicate));
		BuiltInPredicate* bp = dynamic_cast<BuiltInPredicate*>(p);
		PredInter* pt = bp->inter(t);
		_predinters[bp][t] = pt;
		return pt;
	}

	FuncInter* inter(Function* f, const vector<SortTable*>& t) {
		map<Function*,map<vector<SortTable*>,FuncInter*> >::iterator it = _funcinters.find(f);
		if(it != _funcinters.end()) {
			map<vector<SortTable*>,FuncInter*>::iterator jt = (it->second).find(t);
			if(jt != (it->second.end())) return jt->second;
		}
		assert(typeid(*f) == typeid(BuiltInFunction));
		BuiltInFunction* bf = dynamic_cast<BuiltInFunction*>(f);
		FuncInter* ft = bf->inter(t);
		_funcinters[bf][t] = ft;
		return ft;
	}

}


