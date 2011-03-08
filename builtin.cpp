/************************************
	builtin.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <typeinfo>
#include <cstdlib> //contains abs for ints
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
		PredInter*	_inter;	

	public:
		// Constructors
		BuiltInPredicate(const string& n, const vector<Sort*>& vs, PredInter* it) : 
			Predicate(n,vs) { _inter = it; }

		// Destructor
		~BuiltInPredicate() { delete(_inter);	}

		// Inspectors
		bool		builtin()							const { return true;	}
		PredInter*	inter(const AbstractStructure&)		const { return _inter;	}
	
};

class SemiBuiltInPredicate : public Predicate {
	
	private:
		mutable map<vector<SortTable*>,PredInter*>	_inters;
		PredInter*									(*_inter)(const vector<SortTable*>&);

	public:
		// Constructor
		SemiBuiltInPredicate(const string& n, const vector<Sort*>& vs, PredInter* (*inter)(const vector<SortTable*>&)) : 
			Predicate(n,vs) { _inter = inter;	}

		// Destructor
		~SemiBuiltInPredicate() { }

		// Inspector
		bool		builtin()							const { return true;	}
		PredInter*	inter(const AbstractStructure& s)	const;

};

PredInter* SemiBuiltInPredicate::inter(const AbstractStructure& s) const {
	vector<SortTable*> vs(nrSorts());
	for(unsigned int n = 0; n < nrSorts(); ++n) vs[n] = s.inter(sort(n));
	map<vector<SortTable*>,PredInter*>::const_iterator it = _inters.find(vs);
	if(it != _inters.end()) return it->second;
	else {
		PredInter* pi = _inter(vs);
		_inters[vs] = pi;
		return pi;
	}
}



/*************************************************************************
	Class to implement comparison predicates (currently '=', '<', '>')
*************************************************************************/

class ComparisonPredicate : public OverloadedPredicate {

	private:
		PredInter* (*_inter)(const vector<SortTable*>&);

	public:
		ComparisonPredicate(const string& name, unsigned int ar, PredInter* (*inter)(const vector<SortTable*>&)) : 
			OverloadedPredicate(name,ar) { _inter = inter;	}
		~ComparisonPredicate() { }

		// Inspectors
		bool		contains(Predicate* p)				const { return p->name() == _name;	}
		Predicate*	resolve(const vector<Sort*>& vs)		  { return disambiguate(vs,0);	}
		Predicate*	disambiguate(const vector<Sort*>&,Vocabulary*);

		string to_string() const { return "builtin"+_name;	}

};

Predicate* ComparisonPredicate::disambiguate(const vector<Sort*>& vs,Vocabulary* v) {
	Sort* s = 0;
	bool containszeros = false;
	for(unsigned int n = 0; n < vs.size(); ++n) {
		if(vs[n]) {
			if(s) {
				s = SortUtils::resolve(s,vs[n],v);
				if(!s) return 0;
			}
			else s = vs[n];
		}
		else containszeros = true;
	}

	Predicate* p = 0;
	if(s) {
		if(!(containszeros && s->nrParents())) {
			vector<Sort*> nvs = vector<Sort*>(vs.size(),s);
			p = OverloadedPredicate::resolve(nvs);
			if(!p) {
				p = new SemiBuiltInPredicate(_name,nvs,_inter);
				_overpreds.push_back(p);
			}
		}
	}
	return p;
}

/************************* 
	Built-in functions 
*************************/

class BuiltInFunction : public Function {
	
	private:
		FuncInter*	_inter;	

	public:
		// Constructors
		BuiltInFunction(const string& n, const vector<Sort*>& is, Sort* os, FuncInter* ft) : 
			Function(n,is,os), _inter(ft) { }
		BuiltInFunction(const string& n, const vector<Sort*>& s, FuncInter* ft) : 
			Function(n,s), _inter(ft) { }

		// Destructor
		~BuiltInFunction() { delete(_inter); }

		// Inspectors
		bool		builtin()							const { return true;	}
		FuncInter*	inter(const AbstractStructure&)		const { return _inter;	}

};

class SemiBuiltInFunction : public Function { 
	
	private:
		mutable map<vector<SortTable*>,FuncInter*>	_inters;
		FuncInter*									(*_inter)(const vector<SortTable*>&);

	public:
		// Constructor
		SemiBuiltInFunction(const string& n, const vector<Sort*>& is, FuncInter* (*inter)(const vector<SortTable*>&)) : 
			Function(n,is) { _inter = inter;	}

		// Destructor
		~SemiBuiltInFunction() { }

		// Inspector
		bool		builtin()							const { return true;	}
		FuncInter*	inter(const AbstractStructure& s)	const;

};

FuncInter* SemiBuiltInFunction::inter(const AbstractStructure& s) const {
	vector<SortTable*> vs(nrSorts());
	for(unsigned int n = 0; n < nrSorts(); ++n) vs[n] = s.inter(sort(n));
	map<vector<SortTable*>,FuncInter*>::const_iterator it = _inters.find(vs);
	if(it != _inters.end()) return it->second;
	else {
		FuncInter* fi = _inter(vs);
		_inters[vs] = fi;
		return fi;
	}
}

/***************************************************************************
	Class to implement SUCC, PRED, MIN and MAX
***************************************************************************/

class ComparisonFunction : public OverloadedFunction {

	private:
		FuncInter* (*_inter)(const vector<SortTable*>&);

	public:
		ComparisonFunction(const string& name, unsigned int ar, FuncInter* (*inter)(const vector<SortTable*>&)) : 
			OverloadedFunction(name,ar) { _inter = inter; partial(ar == 1);	}
		~ComparisonFunction() { }

		// Inspectors
		bool		contains(Function* f)				const { return f->name() == _name;	}
		Function*	resolve(const vector<Sort*>& vs)		  { return disambiguate(vs,0);	}
		Function*	disambiguate(const vector<Sort*>&, Vocabulary* v);

		string to_string() const { return "builtin"+_name;	}

};

Function* ComparisonFunction::disambiguate(const vector<Sort*>& vs, Vocabulary* v) {
	Sort* s = 0;
	bool containszeros = false;
	for(unsigned int n = 0; n < vs.size(); ++n) {
		if(vs[n]) {
			if(s) {
				s = SortUtils::resolve(s,vs[n],v);
				if(!s) return 0;
			}
			else s = vs[n];
		}
		else containszeros = true;
	}

	Function* f = 0;
	if(s) {
		if(!(containszeros && s->nrParents())) {
			vector<Sort*> nvs = vector<Sort*>(vs.size(),s);
			f = OverloadedFunction::resolve(nvs);
			if(!f) {
				f = new SemiBuiltInFunction(_name,nvs,_inter);
				f->partial(partial());
				_overfuncs.push_back(f);
			}
		}
	}
	return f;
}

/***********************
	IntFloatFunction
	Used for +, -, *, /, abs, and -/1
***********************/

class IntFloatFunction : public OverloadedFunction {

	private:
		FuncInter*	(*_inter)(const vector<Sort*>&);

	public:
		IntFloatFunction(const string& name, unsigned int ar, FuncInter* (*inter)(const vector<Sort*>&)) : 
			OverloadedFunction(name,ar) { _inter = inter;	}
		~IntFloatFunction() { }

		// Inspectors
		bool		contains(Function* f)				const;
		Function*	disambiguate(const vector<Sort*>& vs)	  { return disambiguate(vs,0);	}
		Function*	disambiguate(const vector<Sort*>&,Vocabulary* v);

		string to_string() const { return "builtin"+_name;	}
};

bool IntFloatFunction::contains(Function* f) const {
	for(unsigned int n = 0; n < _overfuncs.size(); ++n) {
		if(f == _overfuncs[n]) return true;
	}

	Sort* ints = *((StdBuiltin::instance())->sort("int")->begin());
	Sort* floats = *((StdBuiltin::instance())->sort("float")->begin());
	for(unsigned int n = 0; n < f->nrSorts(); ++n) {
		if(f->sort(n)) {
			if(SortUtils::resolve(ints,f->sort(n),0) != ints) {
				if(SortUtils::resolve(floats,f->sort(n),0) != floats) {
					return false;
				}
			}
		}
	}
	return true;
};

Function* IntFloatFunction::disambiguate(const vector<Sort*>& vs, Vocabulary* v) {
	unsigned int intcount = 0;
	bool hasfloat = false;
	Sort* ints = *((StdBuiltin::instance())->sort("int")->begin());
	Sort* floats = *((StdBuiltin::instance())->sort("float")->begin());
	for(unsigned int n = 0; n < vs.size(); ++n) {
		if(vs[n]) {
			if(SortUtils::resolve(ints,vs[n],v) == ints) ++intcount;
			else if(SortUtils::resolve(floats,vs[n],v) == floats) hasfloat = true;
			else return 0;
		}
	}

	Sort* s = 0;
	if(intcount >= vs.size()-1) s = ints;
	if(hasfloat) s = floats;
	
	Function* f = 0;
	if(s) {
		vector<Sort*> nvs = vector<Sort*>(vs.size(),s);
		f = OverloadedFunction::resolve(nvs);
		if(!f) {
			f = new BuiltInFunction(_name,nvs,_inter(nvs));
			f->partial(partial());
			_overfuncs.push_back(f);
		}
	}
	return f;
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
		SortTable*	add(const vector<TypedElement>& tuple);
		SortTable*	remove(const vector<TypedElement>& tuple);
		
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
		Element			element(unsigned int)			const { assert(false); Element e; return e;	}
		unsigned int	position(Element,ElementType)	const { assert(false); return 0;			}

		// Debugging
		virtual string to_string(unsigned int n=0) const = 0;

};

SortTable* InfiniteSortTable::add(const vector<TypedElement>& tuple) {
	if(!PredTable::contains(tuple)) {
		UnionSortTable* ust = new UnionSortTable();
		ust->add(this);
		ust->add(tuple);
		return ust;
	}
	else return this;
}

SortTable* InfiniteSortTable::remove(const vector<TypedElement>& tuple) {
	if(PredTable::contains(tuple)) {
		UnionSortTable* ust = new UnionSortTable();
		ust->add(this);
		ust->remove(tuple);
		return ust;
	}
	else return this;
}

/** All natural numbers **/

class AllNatSortTable : public InfiniteSortTable {

	public:
		ElementType	type()				const { return ELINT;									}
		bool		contains(string* s)	const { return isInt(*s) ? contains(stoi(*s)): false;	}
		bool		contains(int n)		const { return n >= 0;									}
		bool		contains(double d)	const { return (d >= 0 && isInt(d));					}
		bool		contains(compound*)	const;

		AllNatSortTable*	clone()	const	{ return new AllNatSortTable();	}

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

		AllIntSortTable*	clone()	const	{ return new AllIntSortTable();	}

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

		AllFloatSortTable*	clone()	const	{ return new AllFloatSortTable();	}

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
		bool		contains(string*)	const { return true;			}
		bool		contains(int)		const { return true;			}
		bool		contains(double)	const { return true;			}
		bool		contains(compound*)	const;

		AllStringSortTable*	clone()	const	{ return new AllStringSortTable();	}

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
		AllCharSortTable* clone() const { return new AllCharSortTable();	}

		// Destructor
		~AllCharSortTable() { }

		// Mutators
		void sortunique() { }
		SortTable*	add(const vector<TypedElement>& tuple);
		SortTable*	remove(const vector<TypedElement>& tuple);


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

SortTable* AllCharSortTable::add(const vector<TypedElement>& tuple) {
	if(!PredTable::contains(tuple)) {
		StrSortTable* sst = new StrSortTable();
		for(unsigned int n = 0; n < size(); ++n) {
			sst->add(element(n)._string);
		}
		sst->FiniteSortTable::add(tuple);
		return sst;
	}
	else return this;
}

SortTable* AllCharSortTable::remove(const vector<TypedElement>& tuple) {
	if(PredTable::contains(tuple)) {
		Element e = ElementUtil::convert(tuple[0],ELSTRING);
		StrSortTable* sst = new StrSortTable();
		for(unsigned int n = 0; n < size(); ++n) {
			Element e2 = element(n);
			if(e._string != e2._string) sst->add(e2._string);
		}
		return sst;
	}
	else return this;
}

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

/** Comparisons **/

class ComparisonPredTable : public PredTable {

	protected:
		SortTable*	_leftsort;
		SortTable*	_rightsort;

	public:
		// Constructor
		ComparisonPredTable(SortTable* tl, SortTable* tr) : PredTable(), _leftsort(tl), _rightsort(tr) { }
		virtual ComparisonPredTable* clone() const = 0;

		// Destructor
		virtual ~ComparisonPredTable() { }

		// Mutators
		void sortunique() { }
		PredTable*	add(const vector<TypedElement>& tuple);
		PredTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		virtual	bool	finite()						const = 0;
		virtual	bool	empty()							const = 0;
				unsigned int	arity()					const { return 2;							}
				ElementType		type(unsigned int n)	const { return n ? _rightsort->type() : _leftsort->type();	}
		virtual unsigned int	size()					const = 0;
		virtual vector<Element>	tuple(unsigned int n)	const = 0;
		virtual Element			element(unsigned int r, unsigned int c)	const = 0;

		// Check if the table contains a given tuple
		virtual bool	contains(const vector<Element>&)	const = 0;

		// Debugging
		virtual string	to_string(unsigned int spaces = 0)	const = 0;

};

PredTable* ComparisonPredTable::add(const vector<TypedElement>& tuple) {
	if(!PredTable::contains(tuple)) {
		UnionPredTable* upt = new UnionPredTable();
		upt->add(this);
		upt->add(tuple);
		return upt;
	}
	else return this;
}

PredTable* ComparisonPredTable::remove(const vector<TypedElement>& tuple) {
	if(PredTable::contains(tuple)) {
		UnionPredTable* upt = new UnionPredTable();
		upt->add(this);
		upt->remove(tuple);
		return upt;
	}
	else return this;
}

/** Equality **/

class EqualPredTable : public ComparisonPredTable {
	public:
		EqualPredTable(SortTable* tl, SortTable* tr) : ComparisonPredTable(tl,tr) { }
		~EqualPredTable() { }
		EqualPredTable* clone() const { return new EqualPredTable(_leftsort,_rightsort);	}

		bool	contains(const vector<Element>&)	const;
		bool	finite()							const { return (_leftsort->finite() || _rightsort->finite());	}
		bool	empty()								const { return (_leftsort->empty() || _rightsort->empty());		}

		unsigned int	size()						const { assert(false); return 0; /* TODO */ }
		vector<Element>	tuple(unsigned int)			const { assert(false); return vector<Element>(0); /* TODO */ }
		Element			element(unsigned int, unsigned int)		const { assert(false); Element e; return e; /* TODO */ }

		string	to_string(unsigned int spaces = 0)	const { return tabstring(spaces) + "=/2";	}
};

bool EqualPredTable::contains(const vector<Element>& ve) const {
	return ElementUtil::equal(ve[0],_leftsort->type(),ve[1],_rightsort->type());
}

PredInter*	equalinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	PredTable* pt = new EqualPredTable(vs[0],vs[1]);
	return new PredInter(pt,true);
}

/** Strictly less than **/

class StrLessThanPredTable : public ComparisonPredTable {
	public:
		StrLessThanPredTable(SortTable* tl, SortTable* tr) : ComparisonPredTable(tl,tr) { }
		~StrLessThanPredTable() { }
		StrLessThanPredTable* clone() const { return new StrLessThanPredTable(_leftsort,_rightsort);	}
		bool	contains(const vector<Element>&)		const;
		bool	finite()	const { return (_leftsort->finite() && _rightsort->finite());assert(false); return false; }
		bool	empty()		const { return (_leftsort->empty() && _rightsort->empty());		}

		unsigned int	size()					const { assert(false); return 0; /* TODO */ }
		vector<Element>	tuple(unsigned int)	const { assert(false); return vector<Element>(0); /* TODO */ }
		Element			element(unsigned int, unsigned int)	const { assert(false); Element e; return e; /* TODO */ }
		string	to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "</2";	}
};

bool StrLessThanPredTable::contains(const vector<Element>& ve) const {
	return ElementUtil::strlessthan(ve[0],_leftsort->type(),ve[1],_rightsort->type());
}

PredInter*	strlessinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	PredTable* pt = new StrLessThanPredTable(vs[0],vs[1]);
	return new PredInter(pt,true);
}

/** Strictly greater than **/

class StrGreaterThanPredTable : public ComparisonPredTable {
	public:
		StrGreaterThanPredTable(SortTable* tl, SortTable* tr) : ComparisonPredTable(tl,tr) { }
		~StrGreaterThanPredTable() { }
		StrGreaterThanPredTable* clone() const { return new StrGreaterThanPredTable(_leftsort,_rightsort);	}
		bool	contains(const vector<Element>&)		const;
		bool	finite()	const { return (_leftsort->finite() && _rightsort->finite());assert(false); return false; }
		bool	empty()		const { return (_leftsort->empty() && _rightsort->empty());		}

		unsigned int	size()					const { assert(false); return 0; /* TODO */ }
		vector<Element>	tuple(unsigned int)	const { assert(false); return vector<Element>(0); /* TODO */ }
		Element			element(unsigned int, unsigned int)	const { assert(false); Element e; return e; /* TODO */ }
		string	to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + ">/2";	}
};

bool StrGreaterThanPredTable::contains(const vector<Element>& ve) const {
	return ElementUtil::strlessthan(ve[1],_rightsort->type(),ve[0],_leftsort->type());
}

PredInter*	strgreaterinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	PredTable* pt = new StrGreaterThanPredTable(vs[0],vs[1]);
	return new PredInter(pt,true);
}

/******************************
	Builtin function tables
******************************/

class InfiniteFuncTable : public FuncTable {

	public:
		// Constructors
		InfiniteFuncTable() : FuncTable() { }
		virtual InfiniteFuncTable* clone() const = 0;

		// Destructor
		virtual ~InfiniteFuncTable() { }

		// Inspectors
				bool			finite()								const { return false;								}
				bool			empty()									const { return false;								}
				unsigned int	size()									const { assert(false); return MAX_INT;				}
				vector<Element>	tuple(unsigned int)						const { assert(false); return vector<Element>(0);	}
				Element			element(unsigned int,unsigned int)		const { assert(false); Element e; return e;			}
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
		virtual AritFuncTable* clone() const = 0; 

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
		PlusFuncTable* clone() const { return new PlusFuncTable(_type);	}
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

FuncInter* plusfuncinter(const vector<Sort*>& vs) {
	assert(vs.size() == 3); assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	ElementType t = ELINT;
	if(vs[0] == *((StdBuiltin::instance())->sort("float")->begin())) t = ELDOUBLE;
	FuncTable* ft = new PlusFuncTable(t);
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Subtraction **/

class MinusFuncTable : public AritFuncTable {
	public:
		MinusFuncTable(ElementType t) : AritFuncTable(t) { }
		MinusFuncTable* clone() const { return new MinusFuncTable(_type);	}
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

FuncInter* minusfuncinter(const vector<Sort*>& vs) {
	assert(vs.size() == 3); assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	ElementType t = ELINT;
	if(vs[0] == *((StdBuiltin::instance())->sort("float")->begin())) t = ELDOUBLE;
	FuncTable* ft = new MinusFuncTable(t);
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Multiplication **/

class TimesFuncTable : public AritFuncTable {
	public:
		TimesFuncTable(ElementType t) : AritFuncTable(t) { }
		TimesFuncTable* clone() const { return new TimesFuncTable(_type);	}
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

FuncInter* timesfuncinter(const vector<Sort*>& vs) {
	assert(vs.size() == 3); assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	ElementType t = ELINT;
	if(vs[0] == *((StdBuiltin::instance())->sort("float")->begin())) t = ELDOUBLE;
	FuncTable* ft = new TimesFuncTable(t);
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Division **/

class DivFuncTable : public AritFuncTable {
	public:
		DivFuncTable(ElementType t) : AritFuncTable(t) { }
		DivFuncTable* clone() const { return new DivFuncTable(_type);	}
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

FuncInter* divfuncinter(const vector<Sort*>& vs) {
	assert(vs.size() == 3); assert(vs[0] == vs[1]); assert(vs[1] == vs[2]);
	ElementType t = ELINT;
	if(vs[0] == *((StdBuiltin::instance())->sort("float")->begin())) t = ELDOUBLE;
	FuncTable* ft = new DivFuncTable(t);
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Modulo **/

class ModFuncTable : public InfiniteFuncTable {
	public:
		ModFuncTable() : InfiniteFuncTable() { }
		ModFuncTable* clone() const { return new ModFuncTable();	}
		~ModFuncTable() { }
		unsigned int arity()							const { return 2;	}

		ElementType type(unsigned int)					const { return ELINT;	}
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

FuncInter* modfuncinter() {
	FuncTable* ft = new ModFuncTable();
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Exponentiation **/

class ExpFuncTable : public InfiniteFuncTable {
	public:
		ExpFuncTable() : InfiniteFuncTable() { }
		ExpFuncTable* clone() const { return new ExpFuncTable();	}
		~ExpFuncTable() { }
		unsigned int arity()							const { return 2;			}

		ElementType type(unsigned int)					const { return ELDOUBLE;	}
		Element operator[](const vector<Element>& vi)	const;
		string to_string(unsigned int spaces = 0)		const { return tabstring(spaces) + "^/2";	}
};

Element ExpFuncTable::operator[](const vector<Element>& ve) const {
	Element e;
	e._double = pow(ve[0]._double,ve[1]._double);
	return e;
}

FuncInter* expfuncinter() {
	FuncTable* ft = new ExpFuncTable();
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Absolute value **/

class AbsFuncTable : public AritFuncTable {
	public:
		AbsFuncTable(ElementType t) : AritFuncTable(t) { }
		AbsFuncTable* clone() const { return new AbsFuncTable(_type);	}
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

FuncInter* absfuncinter(const vector<Sort*>& vs) {
	assert(vs.size() == 2); assert(vs[0] == vs[1]);
	ElementType t = ELINT;
	if(vs[0] == *((StdBuiltin::instance())->sort("float")->begin())) t = ELDOUBLE;
	FuncTable* ft = new AbsFuncTable(t);
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/** Unary minus **/

class UMinFuncTable : public AritFuncTable {
	public:
		UMinFuncTable(ElementType t) : AritFuncTable(t) { }
		UMinFuncTable* clone() const { return new UMinFuncTable(_type);	}
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

FuncInter* uminfuncinter(const vector<Sort*>& vs) {
	assert(vs.size() == 2); assert(vs[0] == vs[1]);
	ElementType t = ELINT;
	if(vs[0] == *((StdBuiltin::instance())->sort("float")->begin())) t = ELDOUBLE;
	FuncTable* ft = new UMinFuncTable(t);
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

/** Successor and predecessor **/

class SuccFuncTable : public FuncTable {

	private:
		SortTable*	_table;
		bool		_succ;

	public:

		// Constructors
		SuccFuncTable(SortTable* t, bool s) : FuncTable(), _table(t), _succ(s) { }
		SuccFuncTable* clone() const { return new SuccFuncTable(_table,_succ);	}

		// Destructor
		~SuccFuncTable() { }

		// Mutators
		FuncTable*	add(const vector<TypedElement>& tuple);
		FuncTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		bool			finite()								const { return _table->finite();	}
		bool			empty()									const { return (_table->finite()) && _table->size() < 2;	}
		unsigned int	size()									const { assert(_table->finite()); return _table->size()-1;	}
		vector<Element>	tuple(unsigned int n)					const;
		Element			element(unsigned int r,unsigned int c)	const;
		unsigned int	arity()									const { return 1;				}
		ElementType		type(unsigned int)						const { return _table->type();	}
		Element			operator[](const vector<Element>& vi)	const;

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

string SuccFuncTable::to_string(unsigned int spaces) const {
	if(_succ) return tabstring(spaces) + "SUCC/1";
	else return tabstring(spaces) + "PRED/1";
}

Element SuccFuncTable::operator[](const vector<Element>& ve) const {
	if(_table->finite()) {
		unsigned int p1 = _table->position(ve[0],_table->type());
		if(_succ) {
			if(p1 < _table->size()-1) return _table->element(p1+1); 
			else return ElementUtil::nonexist(_table->type());
		}
		else {
			if(p1 > 0) return _table->element(p1-1);
			else return ElementUtil::nonexist(_table->type());
		}
	}
	else if(typeid(*_table) == typeid(AllNatSortTable)) {
		if(_succ) {
			Element e; e._int = ve[0]._int + 1; return e;
		}
		else {
			if(ve[0]._int == 0) return ElementUtil::nonexist(ELINT);
			else {
				Element e; e._int = ve[0]._int - 1; return e;
			}
		}
	}
	else if(typeid(*_table) == typeid(AllIntSortTable)) {
		if(_succ) {
			Element e; e._int = ve[0]._int + 1; return e;
		}
		else {
			Element e; e._int = ve[0]._int - 1; return e;
		}
	}
	else if(typeid(*_table) == typeid(AllCharSortTable)) {
		char c1 = (*(ve[0]._string))[0];
		if(_succ) ++c1;
		else {
			if(c1 == 0) return ElementUtil::nonexist(ELSTRING);
			else --c1;
		}
		Element e; 
		e._string = IDPointer(string(1,c1));
		return e;
	}
	assert(false);
	Element e;
	return e;
}

vector<Element> SuccFuncTable::tuple(unsigned int n) const {
	vector<Element> ve(2);
	ve[0] = element(n,0);
	ve[1] = element(n,1);
	return ve;
}

Element	SuccFuncTable::element(unsigned int r, unsigned int c) const {
	if(_succ) {
		if(c == 0) return _table->element(r);
		else return _table->element(r+1);
	}
	else {
		if(c == 0) return _table->element(r+1);
		else return _table->element(r);
	}
}

FuncInter* succfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	assert(vs[0] == vs[1]);
	FuncTable* ft = new SuccFuncTable(vs[0],true);
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

FuncInter* predfuncinter(const vector<SortTable*>& vs) {
	assert(vs.size() == 2);
	assert(vs[0] == vs[1]);
	FuncTable* ft = new SuccFuncTable(vs[0],false);
	PredTable* pt = new FuncPredTable(ft);
	PredInter* pi = new PredInter(pt,true);
	return new FuncInter(ft,pi);
}

/**********************************
	Standard built-in vocabulary
**********************************/

StdBuiltin* StdBuiltin::_instance = 0;

StdBuiltin* StdBuiltin::instance() {
	if(!_instance) _instance = new StdBuiltin();
	return _instance;
}

StdBuiltin::StdBuiltin() : Vocabulary("std") {

	// Create sorts
	Sort* natsort		= new BuiltInSort("nat",new AllNatSortTable());
	Sort* intsort		= new BuiltInSort("int",new AllIntSortTable()); 
	Sort* floatsort		= new BuiltInSort("float",new AllFloatSortTable());
	Sort* charsort		= new BuiltInSort("char",new AllCharSortTable());
	Sort* stringsort	= new BuiltInSort("string",new AllStringSortTable());

	// Add the sorts
	addSort(natsort);
	addSort(intsort);
	addSort(floatsort);
	addSort(charsort);
	addSort(stringsort);

	// Set sort hierarchy 
	intsort->addParent(floatsort);
	natsort->addParent(intsort);
	charsort->addParent(stringsort);

	// Built-in predicates
	addPred(new ComparisonPredicate("=/2",2,&equalinter));
	addPred(new ComparisonPredicate("</2",2,&strlessinter));
	addPred(new ComparisonPredicate(">/2",2,&strgreaterinter));

	// Built-in functions
	addFunc(new IntFloatFunction("+/2",2,&plusfuncinter));
	addFunc(new IntFloatFunction("-/2",2,&minusfuncinter));
	addFunc(new IntFloatFunction("*/2",2,&timesfuncinter));
	Function* divfunc = new IntFloatFunction("//2",2,&divfuncinter); divfunc->partial(true);
	addFunc(divfunc);
	addFunc(new IntFloatFunction("abs/1",1,&absfuncinter));
	addFunc(new IntFloatFunction("-/1",1,&uminfuncinter));

	vector<Sort*> modsorts(3,intsort);
	Function* modfunc = new BuiltInFunction("%/2",modsorts,modfuncinter()); modfunc->partial(true);
	addFunc(modfunc);

	vector<Sort*> expsorts(3,floatsort);
	addFunc(new BuiltInFunction("^/2",expsorts,expfuncinter()));

	addFunc(new ComparisonFunction("MIN/0",0,&minimumfuncinter));
	addFunc(new ComparisonFunction("MAX/0",0,&maximumfuncinter));
	addFunc(new ComparisonFunction("SUCC/1",1,&succfuncinter));
	addFunc(new ComparisonFunction("PRED/1",1,&predfuncinter));

}
