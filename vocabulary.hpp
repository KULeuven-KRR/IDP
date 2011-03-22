/************************************
	vocabulary.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VOCABULARY_HPP
#define VOCABULARY_HPP

#include <vector>
#include <set>
#include <ostream>
#include "parseinfo.hpp"

/**
 * \file vocabulary.hpp
 * DESCRIPTION
 *		This file contains the classes concerning vocabularies:
 *		- sorts, variables, predicate, and function symbols
 *		- class to represent a vocabulary.
 */

/*************
	Sorts
*************/

class Predicate;
class Vocabulary;
class SortTable;

/**
 * DESCRIPTION
 *		Class to represent sorts
 */
class Sort {

	private:
		std::string				_name;			//!< name of the sort
		std::set<Vocabulary*>	_vocabularies;	//!< all vocabularies the sort belongs to 

		std::set<Sort*>			_parents;		//!< the parent sorts of the sort in the sort hierarchy
		std::set<Sort*>			_children;		//!< the children of the sort in the sort hierarchy 
	
		Predicate*				_pred;			//!< the predicate that corresponds to the sort
		ParseInfo				_pi;			//!< the place where the sort was declared 

		SortTable*				_interpretation;	//!< the interpretation of the sort if it is built-in. 
													//!< a null-pointer otherwise.
		~Sort();
		void removeParent(Sort* p);	//!< Removes parent p
		void removeChild(Sort* c);	//!< Removes child c

	public:
		// Constructors
		Sort(const std::string& name);						//!< create an internal sort 
		Sort(const std::string& name, const ParseInfo& pi);  //!< create a user-declared sort

		// Mutators
		void	addParent(Sort* p);	//!< Adds p as a parent. Also adds this as a child of p.
		void	addChild(Sort* c);	//!< Adds c as a child. Also add this as a parent of c.
		
		// Inspectors
		const std::string&	name()							const	{ return _name;		} 
		const ParseInfo&	pi()							const	{ return _pi;		}
		Predicate*			pred()							const	{ return _pred;		}
		std::set<Sort*>		ancestors(Vocabulary* v = 0)	const;	//!< Returns the ancestor of the sort
		std::set<Sort*>		descendents(Vocabulary* v = 0)	const;	//!< Returns the descendents of the sort
		SortTable*			interpretation()				const	{ return _interpretation;		}	
			//!< returns the interpretation for built-in sorts
		bool				builtin()						const	{ return _interpretation != 0;	}

		// Output
		std::ostream&	put(std::ostream&)	const;
		std::string		to_string()		const;
};

std::ostream& operator<< (std::ostream&,const Sort&);

namespace SortUtils {
	Sort* resolve(Sort* s1, Sort* s2, Vocabulary* v);	//!< Return the unique nearest common ancestor of two sorts
}

/****************
	Variables	
****************/

/**
 * DESCRIPTION
 *		Class to represent variables.
 */
class Variable {

	private:
		string			_name;	//!< name of the variable
		const Sort*		_sort;	//!< sort of the variable (0 if the sort is not derived)
		static int		_nvnr;	//!< used to create unique new names for internal variables
		ParseInfo		_pi;	//!< the place where the variable was quantified 

		// Destructor
		~Variable() { }	// NOTE: deleting variables creates dangling pointers
						// Only delete a variable when deleting its quantifier!
	public:

		// Constructors
		Variable(const string& name, const Sort* sort, const ParseInfo& pi) : _name(name), _sort(sort), _pi(pi) { }
		Variable(const Sort* s);	// constructor for an internal variable 

		// Mutators
		void	sort(const Sort* s)	{ _sort = s; }

		// Inspectors
		const std::string&	name()	const { return _name;	}
		const Sort*			sort()	const { return _sort;	}
		const ParseInfo&	pi()	const { return _pi;		}

		// Output
		std::ostream&	put(std::ostream&)	const;
		std::string		to_string()			const;

};

std::ostream& operator<< (std::ostream&,const Variable&);


/*************************************
	Predicate and function symbols
*************************************/

/** 
 * DESCRIPTION
 *		Abstract base class to represent predicate and function symbols
 */
class PFSymbol {
	
	protected:
		string				_name;	//!< Name of the symbol (ending with the /arity)
		std::vector<Sort*>	_sorts;	//!< Sorts of the arguments. For a function symbol, the last sort is the output sort.
		ParseInfo			_pi;	//!< the place where the symbol was declared 

		// Destructor
		virtual ~PFSymbol() { }		// NOTE: deleting a PFSymbol creates dangling pointers
									// Only delete a PFSymbol if the global namespace is deleted

	public:

		// Constructors
		PFSymbol(const string& name, const vector<Sort*>& sorts) : 
			_name(name), _sorts(sorts) { }
		PFSymbol(const string& name, const vector<Sort*>& sorts, const ParseInfo& pi) : 
			_name(name), _sorts(sorts), _pi(pi) { }

		// Inspectors
		const string&			name()					const { return _name;			}
		const ParseInfo&		pi()					const { return _pi;				}
		unsigned int			nrSorts()				const { return _sorts.size();	}
		Sort*					sort(unsigned int n)	const { return _sorts[n];		}
		const vector<Sort*>&	sorts()					const { return _sorts;			}

		// Built-in and overloaded symbols 
		virtual bool		builtin()										const = 0;
		virtual PredInter*	predinter(const AbstractStructure&)				const = 0;
		virtual bool		overloaded()									const = 0;
		virtual PFSymbol*	resolve(const vector<Sort*>&)					const = 0;
		virtual PFSymbol*	disambiguate(const vector<Sort*>&,Vocabulary*)  const = 0;

		// Output
		virtual std::ostream&	put(std::ostream&)	const = 0;
				string			to_string()			const;
		virtual	bool			infix()				const = 0;
};

std::ostream& operator<< (std::ostream&,const PFSymbol&);

#ifdef OLD
/** Predicate symbols **/

class Predicate : public PFSymbol {

	private:
		static int	_npnr;	// used to create unique new names for internal predicates

	public:

		// Constructors
		Predicate(const string& name,const vector<Sort*>& sorts, const ParseInfo& pi) : PFSymbol(name,sorts,pi) { }
		Predicate(const string& name,const vector<Sort*>& sorts) : PFSymbol(name,sorts) { }
		Predicate(const vector<Sort*>& sorts);	// constructor for internal/tseitin predicates

		// Destructors
		virtual ~Predicate() { }

		// Inspectors
				unsigned int	arity()		const { return _sorts.size();	}
				bool			ispred()	const { return true;			}

		// Built-in symbols
				PredInter*		predinter(const AbstractStructure& s)	const { return inter(s);	}
		virtual	PredInter*		inter(const AbstractStructure&)			const { return 0;			}

		// Overloaded symbols 
		virtual bool				contains(Predicate* p)				const { return p == this;	}
		virtual Predicate*			resolve(const vector<Sort*>&);
		virtual Predicate*			disambiguate(const vector<Sort*>&,Vocabulary*);
		virtual vector<Predicate*>	nonbuiltins();
		virtual	vector<Sort*>		allsorts()							const;

		// Visitor
        void accept(Visitor*) const;

};

/** Overloaded predicate symbols **/

class OverloadedPredicate : public Predicate {

	protected:
		vector<Predicate*>	_overpreds;	// the overloaded predicates

	public:

		// Constructors
		OverloadedPredicate(const string& n, unsigned int ar) : Predicate(n,vector<Sort*>(ar,0)) { }

		// Destructor
		~OverloadedPredicate() { }

		// Mutators
		void	overpred(Predicate* p);	

		// Inspectors
				bool				overloaded()						const { return true;		}
		virtual bool				contains(Predicate* p)				const;
		virtual Predicate*			resolve(const vector<Sort*>&);
		virtual Predicate*			disambiguate(const vector<Sort*>&,Vocabulary*);
				vector<Predicate*>	nonbuiltins();	//!< All non-builtin predicates 
													//!< that are overloaded by the predicate
		const	vector<Predicate*>&	overpreds()							const { return _overpreds;	}
				vector<Sort*>		allsorts()							const;

		// Visitor
		void 	accept(Visitor*) const;

		// Debugging of GidL
		void	inspect()	const;
		virtual string	to_string()	const;

};

namespace PredUtils {
	
	/**
	 * return a new overloaded predicate containing the two given predicates
	 */
	Predicate* overload(Predicate* p1, Predicate* p2);

	/**
	 * return a new overloaded predicate containing the given predicates
	 */
	Predicate* overload(const vector<Predicate*>&);

}

/** Functions **/

class Function : public PFSymbol {

	protected:
		bool	_partial;		// true iff the function is declared as partial function
		bool	_constructor;	// true iff the function generates new domain elements

	public:

		// Constructors
		Function(const string& name, const vector<Sort*>& is, Sort* os, const ParseInfo& pi) : 
			PFSymbol(name,is,pi), _partial(false), _constructor(false) { _sorts.push_back(os); }
		Function(const string& name, const vector<Sort*>& sorts, const ParseInfo& pi) : 
			PFSymbol(name,sorts,pi), _partial(false), _constructor(false) { }
		Function(const string& name, const vector<Sort*>& is, Sort* os) : 
			PFSymbol(name,is), _partial(false), _constructor(false) { _sorts.push_back(os); }
		Function(const string& name, const vector<Sort*>& sorts) : 
			PFSymbol(name,sorts), _partial(false), _constructor(false) { }

		// Destructors
		virtual ~Function() { }

		// Mutators
		void	partial(const bool& b)		{ _partial = b;		}
		void	constructor(const bool& b)	{ _constructor = b;	}
	
		// Inspectors
				vector<Sort*>	insorts()				const;	// returns the input sorts of the function
				unsigned int	arity()					const { return _sorts.size()-1;			}
				Sort*			insort(const int& n)	const { return _sorts[n];				}
				Sort*			outsort()				const { return _sorts.back();			}
				bool			partial()				const { return _partial;				}
				bool			constructor()			const { return _constructor;			}
				bool			ispred()				const { return false;					}

		// Built-in symbols
				PredInter*		predinter(const AbstractStructure& s)	const;
		virtual	FuncInter*		inter(const AbstractStructure&)			const { return 0;		}

		// Overloaded symbols 
		virtual bool				contains(Function* f)	const { return f == this;				}
		virtual Function*			resolve(const vector<Sort*>&);
		virtual Function*			disambiguate(const vector<Sort*>&,Vocabulary*);
		virtual	vector<Function*>	nonbuiltins();	
		virtual	vector<Sort*>		allsorts()				const;	

		// Visitor
        void accept(Visitor*) const;

};

/** Overloaded function symbols **/

class OverloadedFunction : public Function {

	protected:
		vector<Function*>	_overfuncs;	// the overloaded functions

	public:

		// Constructors
		OverloadedFunction(const string& n, unsigned int ar) : Function(n,vector<Sort*>(ar+1,0)) { }

		// Destructor
		~OverloadedFunction() { }

		// Mutators
		void	overfunc(Function* f);

		// Inspectors
				bool				overloaded()						const { return true;		}
		virtual bool				contains(Function* f)				const;
		virtual Function*			resolve(const vector<Sort*>&);
		virtual Function*			disambiguate(const vector<Sort*>&,Vocabulary*);
				vector<Function*>	nonbuiltins();	//!< All non-builtin functions 
													//!< that are overloaded by the function
		const	vector<Function*>&	overfuncs()							const { return _overfuncs;	}
				vector<Sort*>		allsorts()							const;	

		// Visitor
		void 	accept(Visitor*) const;

		// Debugging of GidL
		void	inspect()	const;
		virtual string	to_string()	const;


};

namespace FuncUtils {
	
	/**
	 * return an new overloaded function containing the two given functions
	 */
	Function* overload(Function* p1, Function* p2);

	/**
	 * return a new overloaded predicate containing the given predicates
	 */
	Function* overload(const vector<Function*>&);

}



/*****************
	Vocabulary
*****************/

class Vocabulary {

	private:

		string		_name;	//<! name of the vocabulary. Default name is the empty string
		ParseInfo	_pi;	//<! place where the vocabulary was parsed

		// map a name to a symbol of the vocabulary
		// NOTE: the strings that map to predicates and functions end on /arity
		// NOTE: if there is more than one sort/predicate/function with the same name and arity,
		// this maps to an overloaded sort/predicate/function
		map<string,set<Sort*> >	_name2sort;
		map<string,Predicate*>	_name2pred;
		map<string,Function*>	_name2func;

		// map non-builtin, non-overloaded symbols of the vocabulary to a unique index and vice versa
		map<Sort*,unsigned int>			_sort2index;		
		map<Predicate*,unsigned int>	_predicate2index;
		map<Function*,unsigned int>		_function2index;
		vector<Sort*>					_index2sort;
		vector<Predicate*>				_index2predicate;
		vector<Function*>				_index2function;

	public:

		// Constructors
		Vocabulary(const string& name); 
		Vocabulary(const string& name, const ParseInfo& pi); 

		// Destructor
		virtual ~Vocabulary() { }

		// Mutators
		void addSort(Sort*);		 //!< Add the given sort (and its ancestors) to the vocabulary
		void addPred(Predicate*);	 //!< Add the given predicate (and its sorts) to the vocabulary
		void addFunc(Function*);	 //!< Add the given function (and its sorts) to the vocabulary
		void addVocabulary(Vocabulary*);

		// Inspectors
		const string&		name()					const { return _name;					}
		const ParseInfo&	pi()					const { return _pi;						}
		bool				contains(Sort* s)		const;	//!< true if the vocabulary contains the sort
		bool				contains(Predicate* p)	const;	//!< true if the vocabulary contains the predicate
		bool				contains(Function* f)	const;	//!< true if the vocabulary contains the function
		bool				contains(PFSymbol* s)	const; 	//!< true if the vocabulary contains the symbol
		unsigned int		index(Sort*)			const;	//!< return the index of the given sort
		unsigned int		index(Predicate*)		const;	//!< return the index of the given predicate
		unsigned int		index(Function*)		const;	//!< return the index of the given function
		unsigned int		nrNBSorts()				const { return _index2sort.size();	}
		unsigned int		nrNBPreds()				const { return _index2predicate.size();	}
		unsigned int		nrNBFuncs()				const { return _index2function.size();	}
		Sort*				nbsort(unsigned int n)	const { return _index2sort[n];			}
		Predicate*			nbpred(unsigned int n)	const { return _index2predicate[n];		}
		Function*			nbfunc(unsigned int n)	const { return _index2function[n];		}

		map<string,set<Sort*> >::iterator	firstsort()	{ return _name2sort.begin();	}
		map<string,Predicate*>::iterator	firstpred()	{ return _name2pred.begin();	}
		map<string,Function*>::iterator		firstfunc()	{ return _name2func.begin();	}
		map<string,set<Sort*> >::iterator	lastsort()	{ return _name2sort.end();		}
		map<string,Predicate*>::iterator	lastpred()	{ return _name2pred.end();		}
		map<string,Function*>::iterator		lastfunc()	{ return _name2func.end();		}

		const set<Sort*>*	sort(const string&)	const;	// return the sorts with the given name
		Predicate*			pred(const string&)	const;	// return the predicate with the given name (ending on /arity)
		Function*			func(const string&)	const;	// return the function with the given name (ending on /arity)

		vector<Predicate*>	pred_no_arity(const string&)	const;	// return all predicates with the given name (not including the arity)
		vector<Function*>	func_no_arity(const string&)	const;	// return all functions with the given name (not including the arity)

		// Visitor
		void accept(Visitor*) const;

        // Lua
		TypedInfArg getObject(const string& str) const;

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/**********************
	Domain elements
**********************/

/*
 * The four different types of domain elements 
 *		ELINT: an integer 
 *		ELDOUBLE: a floating point number 
 *		ELSTRING: a string (characters are strings of length 1)
 *		ELCOMPOUND: a constructor function applied to a number of domain elements
 *
 *		These types form a hierarchy: ints are also doubles, doubles are also strings, strings are also compounds.
 *
 */


// A single domain element
union Element {
	int			_int;
	double		_double;
	string*		_string;
	compound*	_compound;
};

// A pair of a domain element and its type
struct TypedElement {
	Element		_element;
	ElementType	_type;
	TypedElement(Element e, ElementType t) : _element(e), _type(t) { }
	TypedElement(int n) : _type(ELINT) { _element._int = n;	}
	TypedElement(double d) : _type(ELDOUBLE) { _element._double = d;	}
	TypedElement(string* s) : _type(ELSTRING) { _element._string = s;	}
	TypedElement(compound* c) : _type(ELCOMPOUND) { _element._compound = c;	}
	TypedElement() : _type(ELINT) { }
};

/*
 *		Compound domain elements
 *		
 *		NOTE: sometimes, an int/double/string is represented as a compound domain element
 *		in that case, _function==0 and the only element in _args is the int/double/string
 *
 */
struct compound {
	Function*				_function;
	vector<TypedElement>	_args;

	compound(Function* f, const vector<TypedElement>& a) : _function(f), _args(a) { }
	string to_string() const;
};
typedef compound* domelement;

// Class that implements the relation 'less-than-or-equal' on tuples of domain elements with the same types
class ElementWeakOrdering {
	
	private:
		vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementWeakOrdering() { }
		ElementWeakOrdering(const vector<ElementType>& t) : _types(t) { }
		bool operator()(const vector<Element>&,const vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}
};

// Class that implements the relation 'equal' on tuples of domain elements with the same types
class ElementEquality {

	private:
		vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementEquality() { }
		ElementEquality(const vector<ElementType>& t) : _types(t) { }
		bool operator()(const vector<Element>&,const vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}

};

// Useful functions on domain elements
namespace ElementUtil {
	
	// Return the least precise elementtype of the two given types
	ElementType	resolve(ElementType,ElementType);

	// Return the most precise elementtype
	ElementType leasttype(); 

	// Return the most precise type of the given element
	ElementType reduce(Element,ElementType);
	ElementType reduce(TypedElement);

	// Convert a domain element to a string
	string	ElementToString(Element,ElementType);
	string	ElementToString(TypedElement);

	// Return the unique non-existing domain element of a given type. 
	//Non-existing domain elements are used as return value when a partial function is applied on elements outside its domain
	Element	nonexist(ElementType);				

	// Checks if the element exists, i.e., if it is not equal to the unique non-existing element
	bool		exists(Element,ElementType);	
	bool		exists(TypedElement);					

	// Convert an element from one type to another
	// If this is impossible, the non-existing element of the requested type is returned
	Element		convert(TypedElement,ElementType newtype);		
	Element		convert(Element,ElementType oldtype,ElementType newtype);
	vector<TypedElement> convert(const vector<domelement>&);

	// Compare elements
	bool	equal(Element e1, ElementType t1, Element e2, ElementType t2);				// equality
	bool	strlessthan(Element e1, ElementType t1, Element e2, ElementType t2);		// strictly less than
	bool	lessthanorequal(Element e1, ElementType t1, Element e2, ElementType t2);	// less than or equal

	// Compare tuples
	bool	equal(const vector<TypedElement>&, const vector<Element>&, const vector<ElementType>&);

}

bool operator==(TypedElement e1, TypedElement e2);
bool operator!=(TypedElement e1, TypedElement e2);
bool operator<=(TypedElement e1, TypedElement e2);
bool operator<(TypedElement e1, TypedElement e2);

#endif
#endif
