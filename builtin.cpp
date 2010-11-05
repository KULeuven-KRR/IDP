/************************************
	builtin.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <typeinfo>
#include <cmath>
#include "builtin.hpp"
#include "data.hpp"

/************************* 
	Built-in sorts 
*************************/

class BuiltInSort : public Sort {
	
	private:

		SortTable*	_inter;		// The interpretation of the sort

	public:

		// Constructors
		BuiltInSort(const string& name, SortTable* t) : Sort(name), _inter(t) { }

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

		PredInter*	(*_inter)(const vector<SortTable*>&);	// The intersection of the interpretation of the predicate
															// with the cross product of the given SortTables

	public:

		// Constructors
		BuiltInPredicate(const string& n, const vector<Sort*>& vs, PredInter* (*it)(const vector<SortTable*>&)) : 
			Predicate(n,vs) { _inter = it; }

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

		FuncInter*	(*_inter)(const vector<SortTable*>&);	// The intersection of the interpretation of the function
															// with the cross product of the given SortTables

	public:

		// Constructors
		BuiltInFunction(const string& n, const vector<Sort*>& is, Sort* os, FuncInter* (*ft)(const vector<SortTable*>&)) : 
			Function(n,is,os) { _inter = ft; }
		BuiltInFunction(const string& n, const vector<Sort*>& s, FuncInter* (*ft)(const vector<SortTable*>&)) : 
			Function(n,s) { _inter = ft; }

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
		vector<Sort*>	(*_deriveSort)(const vector<Sort*>&);	// Derives the missing sorts (null-pointers) 
																// in the given vector of sorts
		PredInter*		(*_inter)(const vector<SortTable*>&);	// The intersection of the interpretation of the predicate
																// with the cross product of the given SortTables
		map<vector<Sort*>,BuiltInPredicate*>	_children;		// Maps a tuple of sorts to the overloaded predicate

	public:

		// Constructors
		OverloadedPredicate(const string& n, unsigned int ar, vector<Sort*> (*ds)(const vector<Sort*>&), PredInter* (*di)(const vector<SortTable*>&)) : 
			Predicate(n,vector<Sort*>(ar,0)) { _deriveSort = ds; _inter = di; }

		// Destructor
		~OverloadedPredicate();

		// Inspectors
		bool		overloaded()	const { return true;	}
		bool		builtin()		const { return true;	}
		Predicate*	disambiguate(const vector<Sort*>&);		// Derive the overloaded predicate that has the given sorts

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
		vector<Sort*>	(*_deriveSort)(const vector<Sort*>&);	// Derives the missing sorts (null-pointers) 
																// in the given vector of sorts
		FuncInter*	 	(*_inter)(const vector<SortTable*>&);	// The intersection of the interpretation of the predicate
																// with the cross product of the given SortTables
		map<vector<Sort*>,BuiltInFunction*>	_children;			// Maps a tuple of sorts to the overloaded function

	public:

		// Constructors
		OverloadedFunction(const string& n, unsigned int ar, vector<Sort*> (*ds)(const vector<Sort*>&), FuncInter* (*di)(const vector<SortTable*>&)) : 
			Function(n,vector<Sort*>(ar+1,0),0) { _deriveSort = ds; _inter = di;	}

		// Destructor
		~OverloadedFunction();

		// Inspectors
		bool		overloaded()	const { return true;	}
		bool		builtin()		const { return true;	}
		Function*	disambiguate(const vector<Sort*>&);		// Derive the overloaded function that has the given sorts
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
			if(vs[n]->base() != _stdbuiltin.sort("float")) return vector<Sort*>(vs.size(),0);
			else {
				Sort* temp = vs[n];
				while(temp) {
					if(temp == _stdbuiltin.sort("int")) break;
					temp = temp->parent();
				}
				if(!temp) deriveint = false;
			}
		}
	}
	if(deriveint) return vector<Sort*>(vs.size(),_stdbuiltin.sort("int"));
	else return vector<Sort*>(vs.size(),_stdbuiltin.sort("float"));
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
	vsn.back() = _stdbuiltin.sort("float");
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
		bif->partial(_partial);
		_children[vsd] = bif;
		return bif;
	}
}

/***************************
	Built-in sort tables
***************************/

/** Infinite built-in sort tables **/

class InfiniteSortTable : public SortTable {

	public: 

		// Constructors
		InfiniteSortTable() : SortTable() { }

		// Destructor
		virtual ~InfiniteSortTable() { }

		// Mutators
		void sortunique() { }
		
		// Inspectors
				bool		finite()	const { return false;	}
				bool		empty()		const { return false;	}
		virtual	ElementType	type()		const = 0;

		// Check if the table contains a given element
		virtual bool	contains(string* s)	const = 0; 
		virtual bool	contains(int n)		const = 0;
		virtual bool	contains(double d)	const = 0;
		virtual bool	contains(compound*)	const = 0;

		// Inspectors for finite tables
		unsigned int	size()							const { assert(false); return MAX_INT;		}
		Element			element(unsigned int n)			const { assert(false); Element e; return e;	}
		unsigned int	position(Element,ElementType)	const { assert(false); return 0;			}

		// Debugging
		virtual string to_string(unsigned int n=0) const = 0;

};

/** All natural numbers **/

class AllNatSortTable : public InfiniteSortTable {

	public:
		ElementType	type()				const { return ELINT;									}
		bool		contains(string* s)	const { return isInt(*s) ? contains(stoi(*s)): false;	}
		bool		contains(int n)		const { return n >= 0;									}
		bool		contains(double d)	const { return (d >= 0 && isInt(d));					}
		bool		contains(compound*)	const;

		string		to_string(unsigned int n = 0)	const { return tabstring(n) + "all natural numbers (including 0)";		}
	
};

bool AllNatSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return SortTable::contains((c->_args)[0]);
}

/** All integers **/

class AllIntSortTable : public InfiniteSortTable {

	public:
		ElementType	type()				const { return ELINT;			}
		bool		contains(string* s)	const { return isInt(*s);		}
		bool		contains(int)		const { return true;			}
		bool		contains(double d)	const { return isInt(d);		}
		bool		contains(compound*)	const;
		string		to_string(unsigned int n = 0)	const { return tabstring(n) + "all integers";	}
		
};

bool AllIntSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return SortTable::contains((c->_args)[0]);
}

/** All floating point numbers **/

class AllFloatSortTable : public InfiniteSortTable {
	
	public:
		ElementType	type()				const { return ELDOUBLE;		}
		bool		contains(string* s)	const { return isDouble(*s);	}
		bool		contains(int)		const { return true;			}
		bool		contains(double)	const { return true;			}
		bool		contains(compound*)	const;
		string		to_string(unsigned int n = 0)	const { return tabstring(n) + "all floats";	}
		
};

bool AllFloatSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return SortTable::contains((c->_args)[0]);
}

/** All strings **/

class AllStringSortTable : public InfiniteSortTable {
		
	public:
		ElementType	type()				const { return ELSTRING;		}
		bool		contains(string* s)	const { return true;			}
		bool		contains(int)		const { return true;			}
		bool		contains(double)	const { return true;			}
		bool		contains(compound*)	const;
		string		to_string(unsigned int n  = 0)	const { return tabstring(n) + "all strings";	}
		
};

bool AllStringSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return SortTable::contains((c->_args)[0]);
}

/** All characters **/

class AllCharSortTable : public SortTable {
		
	public:
		
		// Constructors
		AllCharSortTable() : SortTable() { }

		// Destructor
		~AllCharSortTable() { }

		// Mutators
		void sortunique() { }

		// Inspectors
		bool			finite()	const { return true;				}
		bool			empty()		const { return false;				}
		ElementType		type()		const { return ELSTRING;			}

		// Check if the table contains a given element
		bool	contains(string* s)	const { return (s->size() == 1);	}
		bool	contains(int n)		const { return isChar(n);			}
		bool	contains(double d)	const { return isChar(d);			}
		bool	contains(compound*)	const;

		// Inspectors for finite tables
		unsigned int	size()							const {	return nrOfChars();			}
		Element			element(unsigned int n)			const { Element e; e._string = new string(1,char(n)); return e;	}
		unsigned int	position(Element,ElementType)	const;

		// Debugging
		string	to_string(unsigned int n = 0)	const { return tabstring(n) + "all characters"; }
		
};

bool AllCharSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return SortTable::contains((c->_args)[0]);
}

unsigned int AllCharSortTable::position(Element e,ElementType t) const {
	string* s = (ElementUtil::convert(e,t,ELSTRING))._string;
	assert(s->size() == 1);
	char c = (*s)[0];
	return c - char(0);
}

/*******************************
	Built-in predicate tables
*******************************/

/** Infinite tables **/

class InfinitePredTable : public PredTable {

	public:
		
		// Constructor
		InfinitePredTable() : PredTable() { }

		// Destructor
		virtual ~InfinitePredTable() { }

		// Mutators
		void sortunique() { }

		// Inspectors
				bool			finite()			const { return true;	}
				bool			empty()				const { return false;	}
		virtual unsigned int	arity()				const = 0;
		virtual	ElementType		type(unsigned int)	const = 0;

		// Check if the table contains a given tuple
		virtual bool	contains(const vector<Element>&)	const = 0;

		// Inspectors for finite tables	
		unsigned int	size()									const { assert(false); return MAX_INT;					}
		vector<Element>	tuple(unsigned int n)					const { assert(false); vector<Element> ve; return ve;	}
		Element			element(unsigned int r, unsigned int c)	const { assert(false); Element e; return e;				}

		// Debugging
		virtual string	to_string(unsigned int spaces = 0)		const = 0;


};

/** Comparisons **/

class ComparisonPredTable : public InfinitePredTable {

	protected:
		ElementType	_lefttype;	// The type of the left-hand side elements in the table
		ElementType _righttype; // The type of the right-hand side elements in the table

	public:
		
		// Constructor
		ComparisonPredTable(ElementType tl, ElementType tr) : InfinitePredTable(), _lefttype(tl), _righttype(tr) { }

		// Destructor
		virtual ~ComparisonPredTable() { }

		// Inspectors
		unsigned int	arity()					const { return 2;							}
		ElementType		type(unsigned int n)	const { return n ? _righttype : _lefttype;	}

		// Check if the table contains a given tuple
		virtual bool	contains(const vector<Element>&)	const = 0;

		// Debugging
		virtual string	to_string(unsigned int spaces = 0)	const = 0;

};

/** Equality **/

class EqualPredTable : public ComparisonPredTable {
	public:
		EqualPredTable(ElementType tl, ElementType tr) : ComparisonPredTable(tl,tr) { }
		~EqualPredTable() { }
		bool	contains(const vector<Element>&)	const;
		string	to_string(unsigned int spaces = 0)	const { return tabstring(spaces) + "=/2";	}
};

bool EqualPredTable::contains(const vector<Element>& ve) const {
	return ElementUtil::equal(ve[0],_lefttype,ve[1],_righttype);
}

PredInter*	equalinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	PredTable* pt = new EqualPredTable(vs[0]->type(),vs[1]->type());
	return new PredInter(pt,true);
}

/** Strictly less than **/

class StrLessThanPredTable : public ComparisonPredTable {
	public:
		StrLessThanPredTable(ElementType tl, ElementType tr) : ComparisonPredTable(tl,tr) { }
		~StrLessThanPredTable() { }
		bool	contains(const vector<Element>&)		const;
		string	to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "</2";	}
};

bool StrLessThanPredTable::contains(const vector<Element>& ve) const {
	return ElementUtil::strlessthan(ve[0],_lefttype,ve[1],_righttype);
}

PredInter*	strlessinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	PredTable* pt = new StrLessThanPredTable(vs[0]->type(),vs[1]->type());
	return new PredInter(pt,true);
}

/** Strictly greater than **/

class StrGreaterThanPredTable : public ComparisonPredTable {
	public:
		StrGreaterThanPredTable(ElementType tl, ElementType tr) : ComparisonPredTable(tl,tr) { }
		~StrGreaterThanPredTable() { }
		bool	contains(const vector<Element>&)		const;
		string	to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + ">/2";	}
};

bool StrGreaterThanPredTable::contains(const vector<Element>& ve) const {
	return ElementUtil::strlessthan(ve[1],_righttype,ve[0],_lefttype);
}

PredInter*	strgreaterinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	PredTable* pt = new StrGreaterThanPredTable(vs[0]->type(),vs[1]->type());
	assert(vs.size() == 2);
}

/** Successor **/

class SuccPredTable : public PredTable {

	private:
		SortTable*	_table;		// The table of the sort of the successor predicate

	public:

		// Constructors
		SuccPredTable(SortTable* t) : PredTable(), _table(t) { }

		// Destructor
		~SuccPredTable() { }

		// Mutators
		void sortunique() { }

		// Inspectors
		bool			finite()				const { return _table->finite();						}
		bool			empty()					const { return (_table->finite() && size() == 0);		}
		unsigned int	arity()					const { return 2;										}
		ElementType		type(unsigned int n)	const { return _table->type();							}

		// Check if the table contains a given tuple
		bool			contains(const vector<Element>&)		const;

		// Inspectors for finite tables
		unsigned int	size()									const { return _table->size() - 1;	}
		vector<Element>	tuple(unsigned int n)					const;
		Element			element(unsigned int r, unsigned int c)	const;

		// Debugging
		string	to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "SUCC/2";	} 

};

bool SuccPredTable::contains(const vector<Element>& ve) const {
	if(_table->finite()) {
		unsigned int p1 = _table->position(ve[0],_table->type());
		unsigned int p2 = _table->position(ve[1],_table->type());
		return (p2 == p1+1);
	}
	else if(typeid(*_table) == typeid(AllNatSortTable)) {
		return (ve[1]._int == ve[0]._int + 1);
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

PredInter*	succinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	assert(vs[0] == vs[1]);
	PredTable* pt = new SuccPredTable(vs[0]);
	return new PredInter(pt,true);
}

/******************************
	Builtin function tables
******************************/

class InfiniteFuncTable : public FuncTable {

	public:

		// Constructors
		InfiniteFuncTable() : FuncTable() { }

		// Destructor
		virtual ~InfiniteFuncTable() { }

		// Inspectors
				bool			finite()								const { return false;								}
				bool			empty()									const { return false;								}
				unsigned int	size()									const { assert(false); return MAX_INT;				}
				vector<Element>	tuple(unsigned int n)					const { assert(false); return vector<Element>(0);	}
				Element			element(unsigned int r,unsigned int c)	const { assert(false); Element e; return e;			}
		virtual unsigned int	arity()									const = 0;
		virtual ElementType		type(unsigned int)						const = 0;

		virtual	Element	operator[](const vector<Element>& vi)		const = 0;

		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};

class AritFuncTable : public InfiniteFuncTable {

	protected:
		ElementType _type;	// int or double

	public:

		// Constructors
		AritFuncTable(ElementType t) : InfiniteFuncTable(), _type(t) { }

		// Destructor
		virtual ~AritFuncTable() { }

		// Inspectors
		virtual unsigned int	arity()		const = 0;
		virtual	Element	operator[](const vector<Element>& vi)		const = 0;
				ElementType	type(unsigned int)	const { return _type;	}

		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};

/** Addition **/

class PlusFuncTable : public AritFuncTable { 
	public:
		PlusFuncTable(ElementType t) : AritFuncTable(t) { }
		~PlusFuncTable() { }
		unsigned int arity()							const { return 2;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "+/2";	}
};

Element PlusFuncTable::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = vi[0]._int + vi[1]._int;
			break;
		case ELDOUBLE:
			e._double = (vi[0]._double + vi[1]._double);
			break;
		default: assert(false);
	}
	return e;
}

FuncInter* plusfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 3); assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	FuncTable* ft = new PlusFuncTable(vs[0]->type());
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Subtraction **/

class MinusFuncTable : public AritFuncTable {
	public:
		MinusFuncTable(ElementType t) : AritFuncTable(t) { }
		~MinusFuncTable() { }
		unsigned int arity()							const { return 2;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "-/2";	}
};

Element MinusFuncTable::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = vi[0]._int - vi[1]._int;
			break;
		case ELDOUBLE:
			e._double = (vi[0]._double - vi[1]._double);
			break;
		default: assert(false);
	}
	return e;
}

FuncInter* minusfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 3); assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	FuncTable* ft = new MinusFuncTable(vs[0]->type());
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Multiplication **/

class TimesFuncTable : public AritFuncTable {
	public:
		TimesFuncTable(ElementType t) : AritFuncTable(t) { }
		~TimesFuncTable() { }
		unsigned int arity()							const { return 2;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "*/2";	}
};

Element TimesFuncTable::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = vi[0]._int * vi[1]._int;
			break;
		case ELDOUBLE:
			e._double = vi[0]._double * vi[1]._double;
			break;
		default: assert(false);
	}
	return e;
}

FuncInter* timesfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 3); assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	FuncTable* ft = new TimesFuncTable(vs[0]->type());
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Division **/

class DivFuncTable : public AritFuncTable {
	public:
		DivFuncTable(ElementType t) : AritFuncTable(t) { }
		~DivFuncTable() { }
		unsigned int arity()							const { return 2;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "//2";	}
};

Element DivFuncTable::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			if(vi[1]._int == 0) return ElementUtil::nonexist(ELINT);
			else e._int = vi[0]._int / vi[1]._int;
			break;
		case ELDOUBLE:
			if(vi[1]._double == 0) return ElementUtil::nonexist(ELDOUBLE);
			else e._double = (vi[0]._double / vi[1]._double);
			break;
		default: assert(false);
	}
	return e;
}

FuncInter* divfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 3); assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	FuncTable* ft = new DivFuncTable(vs[0]->type());
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Modulo **/

class ModFuncTable : public InfiniteFuncTable {
	public:
		ModFuncTable() : InfiniteFuncTable() { }
		~ModFuncTable() { }
		unsigned int arity()							const { return 2;	}
		ElementType type(unsigned int n)				const { return ELINT;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "%/2";	}
};

Element ModFuncTable::operator[](const vector<Element>& vi) const {
	if(vi[1]._int == 0) return ElementUtil::nonexist(ELINT);
	else {
		Element e;
		e._int = vi[0]._int % vi[1]._int;
		return e;
	}
}

FuncInter* modfuncinter(const vector<SortTable*>& vs) {
	FuncTable* ft = new ModFuncTable();
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Exponentiation **/

class ExpFuncTable : public AritFuncTable {
	public:
		ExpFuncTable(ElementType t) : AritFuncTable(t) { }
		~ExpFuncTable() { }
		unsigned int arity()							const { return 2;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "^/2";	}
};

Element ExpFuncTable::operator[](const vector<Element>& ve) const {
	Element e;
	switch(_type) {
		case ELINT:
			assert(ve[1]._int >= 0);
			e._int = int(pow(double(ve[0]._int),double(ve[1]._int)));
			break;
		case ELDOUBLE:
			e._double = pow(ve[0]._double,ve[1]._double);
			break;
		default: assert(false);
	}
	return e;
}

FuncInter* expfuncinter(const vector<SortTable*>& vs) {
	FuncTable* ft = new ExpFuncTable(vs[1]->type());
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Absolute value **/

class AbsFuncTable : public AritFuncTable {
	public:
		AbsFuncTable(ElementType t) : AritFuncTable(t) { }
		~AbsFuncTable() { }
		unsigned int arity()							const { return 1;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "abs/1";	}
};

Element AbsFuncTable::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = abs(vi[0]._int);
			break;
		case ELDOUBLE:
			e._double = fabs(vi[0]._double);
			break;
		default: assert(false);
	}
	return e;
}

FuncInter* absfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2); assert(vs[0] == vs[1]);
	FuncTable* ft = new AbsFuncTable(vs[0]->type());
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Unary minus **/

class UMinFuncTable : public AritFuncTable {
	public:
		UMinFuncTable(ElementType t) : AritFuncTable(t) { }
		~UMinFuncTable() { }
		unsigned int arity()							const { return 1;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "-/1";	}
};

Element UMinFuncTable::operator[](const vector<Element>& vi) const {
	Element e;
	switch(_type) {
		case ELINT:
			e._int = -(vi[0]._int);
			break;
		case ELDOUBLE:
			e._double = -(vi[0]._double);
			break;
		default: assert(false);
	}
	return e;
}

FuncInter* uminfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2); assert(vs[0] == vs[1]);
	FuncTable* ft = new UMinFuncTable(vs[0]->type());
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Minimum and maximum **/

FuncInter* minimumfuncinter(const vector<SortTable*>& vs) {
	assert(vs[0]->finite());
	FinitePredTable* upt = new FinitePredTable(vector<ElementType>(1,vs[0]->type()));
	upt->addRow();
	if(vs[0]->empty()) (*upt)[0][0] = ElementUtil::nonexist(vs[0]->type());
	else (*upt)[0][0] = vs[0]->element(0);
	PredInter* pt = new PredInter(upt,true);
	FiniteFuncTable* fft = new FiniteFuncTable(upt);
	return new FuncInter(fft,pt);
}

FuncInter* maximumfuncinter(const vector<SortTable*>& vs) {
	assert(vs[0]->finite());
	assert(vs[0]->finite());
	FinitePredTable* upt = new FinitePredTable(vector<ElementType>(1,vs[0]->type()));
	upt->addRow();
	if(vs[0]->empty()) (*upt)[0][0] = ElementUtil::nonexist(vs[0]->type());
	else (*upt)[0][0] = vs[0]->element(vs[0]->size() - 1);
	PredInter* pt = new PredInter(upt,true);
	FiniteFuncTable* fft = new FiniteFuncTable(upt);
	return new FuncInter(fft,pt);
}

/**********************************
	Standard built-in vocabulary
**********************************/

StdBuiltin::StdBuiltin() : Vocabulary("std") {

	// Create sorts
	Sort* natsort		= new BuiltInSort("nat",new AllNatSortTable());
	Sort* intsort		= new BuiltInSort("int",new AllIntSortTable()); 
	Sort* floatsort		= new BuiltInSort("float",new AllFloatSortTable());
	Sort* charsort		= new BuiltInSort("char",new AllCharSortTable());
	Sort* stringsort	= new BuiltInSort("string",new AllStringSortTable());

	// Add the sorts
	addSort("nat",natsort);
	addSort("int",intsort);
	addSort("float",floatsort);
	addSort("char",charsort);
	addSort("string",stringsort);

	// Set sort hierarchy 
	intsort->parent(floatsort);
	natsort->parent(intsort);
	charsort->parent(stringsort);

	// Built-in predicates
	vector<Sort*> (*sd)(const vector<Sort*>&);
	sd = &overloaded_predicate_deriver1;
	addPred("=/2",new OverloadedPredicate("=/2",2,sd,&equalinter));
	addPred("</2",new OverloadedPredicate("</2",2,sd,&strlessinter));
	addPred(">/2",new OverloadedPredicate(">/2",2,sd,&strgreaterinter));
	addPred("SUCC/2",new OverloadedPredicate("SUCC/2",2,sd,&succinter));

	// Built-in functions
	sd = &overloaded_function_deriver1;
	addFunc("+/2",new OverloadedFunction("+/2",2,sd,&plusfuncinter));
	addFunc("-/2",new OverloadedFunction("-/2",2,sd,&minusfuncinter));
	addFunc("*/2",new OverloadedFunction("*/2",2,sd,&timesfuncinter));
	Function* divfunc = new OverloadedFunction("//2",2,sd,&divfuncinter); divfunc->partial(true);
	addFunc("//2",divfunc);
	addFunc("abs/1",new OverloadedFunction("abs/1",1,sd,&absfuncinter));
	addFunc("-/1",new OverloadedFunction("-/1",1,sd,&uminfuncinter));

	sd = &overloaded_function_deriver2;
	addFunc("MIN/0",new OverloadedFunction("MIN/0",0,sd,&minimumfuncinter));
	addFunc("MAX/0",new OverloadedFunction("MAX/0",0,sd,&maximumfuncinter));

	sd = &overloaded_function_deriver3;
	addFunc("^/2",new OverloadedFunction("^/2",2,sd,&expfuncinter));

	vector<Sort*> modsorts(3,intsort);
	Function* modfunc = new BuiltInFunction("%/2",modsorts,&modfuncinter); modfunc->partial(true);
	addFunc("%/2",modfunc);

}
