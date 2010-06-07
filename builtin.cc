/************************************
	builtin.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "builtin.h"
#include "structure.h"
#include <iostream>
#include <typeinfo>

extern bool isDouble(const string&);
extern bool isInt(const string&);
extern bool isInt(double);
extern bool isChar(int);
extern bool isChar(double);
extern int nrOfChars();

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

		PredInter*	_inter;

	public:

		// Constructors
		BuiltInPredicate(const string& n, const vector<Sort*>& vs, PredInter* pt) : 
			Predicate(n,vs,0), _inter(pt) { }

		// Destructor
		~BuiltInPredicate() { }

		// Inspectors
		bool		builtin()	const { return true;		}
		PredInter*	inter()		const { return _inter;		}
	
};


/************************* 
	Built-in functions 
*************************/

class BuiltInFunction : public Function {
	
	private:

		FuncInter*	_inter;

	public:

		// Constructors
		BuiltInFunction(const string& n, const vector<Sort*>& is, Sort* os, FuncInter* ft) : 
			Function(n,is,os,0), _inter(ft) { }
		BuiltInFunction(const string& n, const vector<Sort*>& s, FuncInter* ft) : 
			Function(n,s,0), _inter(ft) { }

		// Destructor
		~BuiltInFunction() { }

		// Inspectors
		bool		builtin()	const { return true;	}
		FuncInter*	inter()		const { return _inter;	}

};


/****************************
	Overloaded predicates
****************************/

class OverloadedPredicate : public Predicate {

	private:
		vector<Sort*>							(*_deriveSort)(const vector<Sort*>&);
		map<vector<Sort*>,BuiltInPredicate*>	_children;	// Maps a tuple of sorts to the overloaded predicate

	public:

		// Constructors
		OverloadedPredicate(const string& n, unsigned int ar, vector<Sort*> (*ds)(const vector<Sort*>&)) : 
			Predicate(n,vector<Sort*>(ar,0),0) { _deriveSort = ds; }

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
		BuiltInPredicate* bip = new BuiltInPredicate(_name,vsd,0);	// TODO: replace the 0
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
		map<vector<Sort*>,BuiltInFunction*>	_children;	// Maps a tuple of sorts to the overloaded function

	public:

		// Constructors
		OverloadedFunction(const string& n, unsigned int ar, vector<Sort*> (*ds)(const vector<Sort*>&)) : 
			Function(n,vector<Sort*>(ar+1,0),0) { _deriveSort = ds; }

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
 *		Used for functions +, -, *, /, ^, abs 
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

Function* OverloadedFunction::disambiguate(const vector<Sort*>& vs) {
	vector<Sort*> vsd = _deriveSort(vs);
	assert(!vsd.empty());
	if(!vsd[0]) return 0;
	map<vector<Sort*>,BuiltInFunction*>::iterator it = _children.find(vsd);
	if(it != _children.end()) return it->second;
	else {
		BuiltInFunction* bif = new BuiltInFunction(_name,vsd,0);	// TODO: replace the 0
		_children[vsd] = bif;
		return bif;
	}
	return 0;
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

		string	to_string()	const { return "all strings"; }
		
};

/** All characters **/

class AllCharSortTable : public SortTable {
		
	public:
		
		bool			finite()					const { return true;				}
		unsigned int	size()						const {	return nrOfChars();			}
		bool			empty()						const { return false;				}
		bool			contains(const string& s)	const { return (s.size() == 1);		}
		bool			contains(int n)				const { return isChar(n);			}
		bool			contains(double d)			const { return isChar(d);			}
		Element			element(unsigned int n)			  { Element e; e._string = new string(1,char(n)); return e;	}
		ElementType		type()						const { return ELSTRING;			}

		string	to_string()	const { return "all characters"; }
		
};

/*******************************
	Builtin predicate tables
*******************************/

class EqualPredTable : public PredTable {
};

class StrLessThanPredTable : public PredTable {
};

class StrGreaterThanPredTable : public PredTable {
};

class SuccPredTable : public PredTable {
};

/******************************
	Builtin function tables
******************************/

// TODO

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
		_equalpred			= new OverloadedPredicate("=/2",2,sd);
		_strlessthanpred	= new OverloadedPredicate("</2",2,sd);
		_strgreaterthanpred	= new OverloadedPredicate(">/2",2,sd);
		_successorpred		= new OverloadedPredicate("SUCC/2",2,sd);

		_builtinpreds["=/2"]	= _equalpred;
		_builtinpreds["</2"]	= _strlessthanpred;
		_builtinpreds[">/2"]	= _strgreaterthanpred;
		_builtinpreds["SUCC/2"] = _successorpred;

		// Built-in functions
		sd = &overloaded_function_deriver1;
		_plusfunc	= new OverloadedFunction("+/2",2,sd);
		_minusfunc	= new OverloadedFunction("-/2",2,sd);
		_timesfunc	= new OverloadedFunction("*/2",2,sd);
		_divfunc	= new OverloadedFunction("//2",2,sd);
		_expfunc	= new OverloadedFunction("^/2",2,sd);
		_absfunc	= new OverloadedFunction("abs/1",1,sd);
		_uminusfunc	= new OverloadedFunction("-/1",1,sd);
		sd = &overloaded_function_deriver2;
		_minfunc	= new OverloadedFunction("MIN/0",0,sd);
		_maxfunc	= new OverloadedFunction("MAX/0",0,sd);

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

	PredInter* inter(Predicate* p) {
		// TODO
		return 0;
	}

	FuncInter* inter(Function* f) {
		// TODO
		return 0;
	}

}


