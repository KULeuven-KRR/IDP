/************************************
	vocabulary.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VOCABULARY_HPP
#define VOCABULARY_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>

#include "common.hpp"
#include "visitor.hpp"
#include "lua.hpp"

class Formula;
class Predicate;
class SortTable;
class PredInter;
class FuncInter;
class AbstractStructure;
class Vocabulary;
struct compound;
struct TypedInfArg;

/***************************************
	Parse location of parsed objects	
***************************************/

/*
 *		A ParseInfo contains a line number, a column number and a file. 
 *
 *		Almost every object that can be written by a user has a ParseInfo object as attribute. 
 *		The ParseInfo stores where the object was parsed. This is used for producing precise error and warning messages.
 *		Objects that are not parsed (e.g., internally created variables) have line number 0 in their ParseInfo.
 *
 */
class ParseInfo {
	private:
		unsigned int	_line;	// line number where the object is declared (0 for non-parsed objects)
		unsigned int	_col;	// column number where the object is declared
		std::string*	_file;	// file name where the object is declared
								// NOTE: _file == 0 when parsed on stdin
	
	public:
		// Constructors
		ParseInfo() : _line(0), _col(0), _file(0) { }
		ParseInfo(unsigned int line, unsigned int col, std::string* file) : _line(line), _col(col), _file(file) { }
		ParseInfo(const ParseInfo& p) : _line(p.line()), _col(p.col()), _file(p.file()) { }

		// Destructor
		virtual ~ParseInfo() { }

		// Inspectors
		unsigned int	line()		const { return _line;		}
		unsigned int	col()		const { return _col;		}
		std::string*	file()		const { return _file;		}
		bool			isParsed()	const { return _line != 0;	}
};

/*
 *		ParseInfo for formulas.
 *		
 *		Besides the attributes of a ParseInfo object, it contains also the originally parsed formula
 *
 */
class FormParseInfo : public ParseInfo {
	private:
		Formula*	_original;	// The original formula written by the user
								// Null-pointer when associated to an internally created formula with no
								// corresponding original formula

	public:
		// Constructors
		FormParseInfo() : ParseInfo(), _original(0) { }
		FormParseInfo(unsigned int line, unsigned int col, std::string* file, Formula* orig) : 
			ParseInfo(line,col,file), _original(orig) { }
		FormParseInfo(const FormParseInfo& p) : ParseInfo(p.line(),p.col(),p.file()), _original(p.original()) { }

		// Destructor
		~FormParseInfo() { }

		// Inspectors
		Formula*	original()	const { return _original;	}
};


/*************
	Sorts
*************/

class Sort {
	private:
		std::string			_name;		// name of the sort
		std::vector<Sort*>	_parents;	// the parent sorts of the sort in the sort hierarchy
		std::vector<Sort*>	_children;	// the children of the sort in the sort hierarchy 
		Predicate*			_pred;		// the predicate that corresponds to the sort
		ParseInfo			_pi;		// the place where the sort was declared 

	public:
		// Constructors
		Sort(const std::string& name);
		Sort(const std::string& name, const ParseInfo& pi);  

		// Destructor
		virtual ~Sort() { }	// NOTE: deleting a sort creates dangling pointers
							// Only delete sorts when all vocabularies where the sort occurs are deleted 

		// Mutators
		void	addParent(Sort* p);	// Add p as a parent. Also add this as a child of p.
		void	addChild(Sort* c);		// Add c as a child. Also add this as a parent of c.
		void	pred(Predicate* p)	{ _pred = p;	}
		
		// Inspectors
		std::string			name()						const	{ return _name;				} 
		const ParseInfo&	pi()						const	{ return _pi;				}
		unsigned int		nrParents()					const	{ return _parents.size();	}
		Sort*				parent(unsigned int n)		const	{ return _parents[n];		}
		Predicate*			pred()						const	{ return _pred;				}
		unsigned int		nrChildren()				const	{ return _children.size();	}
		Sort*				child(unsigned int n)		const	{ return _children[n];		}
		std::string			to_string()					const	{ return _name;				}
		std::set<Sort*>		ancestors(Vocabulary* v)	const;
		std::set<Sort*>		descendents(Vocabulary* v)	const;

		// Built-in sorts
		virtual bool		builtin()	const	{ return false;	}
		virtual SortTable*	inter()		const	{ return 0;		}	// returns the built-in
																	// interpretation for
																	// built-in sorts
		// Visitor
        void accept(Visitor*) const;
};

namespace SortUtils {
	/**
	 * return the common ancestor with maximum depth of the given sorts. 
	 * Return 0 if such an ancestor does not exist.
	 */ 
	Sort* resolve(Sort* s1, Sort* s2, Vocabulary* v);
}


/****************
	Variables	
****************/

class Variable {
	private:
		std::string	_name;	// name of the variable
		Sort*		_sort;	// sort of the variable (0 if the sort is not derived)
		static int	_nvnr;	// used to create unique new names for internal variables
		ParseInfo	_pi;	// the place where the variable was quantified 

	public:
		// Constructors
		Variable(const std::string& name, Sort* sort, const ParseInfo& pi) : _name(name), _sort(sort), _pi(pi) { }
		Variable(Sort* s);	// constructor for an internal variable 

		// Destructor
		~Variable() { }	// NOTE: deleting variables creates dangling pointers
						// Only delete a variable when deleting its quantifier!

		// Mutators
		void	sort(Sort* s)	{ _sort = s; }

		// Inspectors
		std::string			name()	const { return _name;	}
		Sort*				sort()	const { return _sort;	}
		const ParseInfo&	pi()	const { return _pi;		}

		// Debugging
		std::string	to_string()		const;
};

namespace VarUtils {
	/**
	 * Sort and remove duplicates from a given vector of variables.
	 */
	void sortunique(std::vector<Variable*>& vv);

	/**
 	 * Make a vector of fresh variables of given sorts.
 	 */
	std::vector<Variable*> makeNewVariables(const std::vector<Sort*>&);
}


/*******************************
	Predicates and functions
*******************************/

/** Abstract base class **/

class PFSymbol {
	protected:
		std::string			_name;	// Name of the symbol (ending with the /arity)
		std::vector<Sort*>	_sorts;	// Sorts of the arguments
		ParseInfo			_pi;	// the place where the symbol was declared 

	public:
		// Constructors
		PFSymbol(const std::string& name, const std::vector<Sort*>& sorts) : 
			_name(name), _sorts(sorts) { }
		PFSymbol(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi) : 
			_name(name), _sorts(sorts), _pi(pi) { }

		// Destructor
		virtual ~PFSymbol() { }		// NOTE: deleting a PFSymbol creates dangling pointers
									// Only delete a PFSymbol if the global namespace is deleted

		// Inspectors
				std::string					name()					const { return _name;							}
				const ParseInfo&			pi()					const { return _pi;								}
				unsigned int				nrSorts()				const { return _sorts.size();					}
				Sort*						sort(unsigned int n)	const { return _sorts[n];						}
				const std::vector<Sort*>&	sorts()					const { return _sorts;							}
		virtual	std::string					to_string()				const;
		virtual	bool						ispred()				const = 0;	// true iff the symbol is a predicate

		// Built-in symbols 
		virtual bool		builtin()							const { return false;	}
		virtual PredInter*	predinter(const AbstractStructure&)	const { return 0;		}	// Returns the interpretation of the symbol if it is built-in

		// Overloaded symbols
		virtual bool		overloaded()						const	{ return false;	}	// true iff the symbol 
																							// is overloaded.
		virtual PFSymbol*	resolve(const std::vector<Sort*>&) = 0;
		virtual PFSymbol*	disambiguate(const std::vector<Sort*>&, Vocabulary*)  = 0;	// this method tries to
																						// disambiguate 
																						// overloaded symbols.
};


/** Predicate symbols **/

class Predicate : public PFSymbol {
	private:
		static int	_npnr;	// used to create unique new names for internal predicates

	public:
		// Constructors
		Predicate(const std::string& name,const std::vector<Sort*>& sorts, const ParseInfo& pi) : PFSymbol(name,sorts,pi) { }
		Predicate(const std::string& name,const std::vector<Sort*>& sorts) : PFSymbol(name,sorts) { }
		Predicate(const std::vector<Sort*>& sorts);	// constructor for internal/tseitin predicates

		// Destructors
		virtual ~Predicate() { }

		// Inspectors
				unsigned int	arity()		const { return _sorts.size();	}
				bool			ispred()	const { return true;			}

		// Built-in symbols
				PredInter*		predinter(const AbstractStructure& s)	const { return inter(s);	}
		virtual	PredInter*		inter(const AbstractStructure&)			const { return 0;			}

		// Overloaded symbols 
		virtual bool					contains(Predicate* p)				const { return p == this;	}
		virtual Predicate*				resolve(const std::vector<Sort*>&);
		virtual Predicate*				disambiguate(const std::vector<Sort*>&,Vocabulary*);
		virtual std::vector<Predicate*>	nonbuiltins();
		virtual	std::vector<Sort*>		allsorts()							const;

		// Visitor
        		void accept(Visitor*) const;
};

/** Overloaded predicate symbols **/

class OverloadedPredicate : public Predicate {
	protected:
		std::vector<Predicate*>	_overpreds;	// the overloaded predicates

	public:
		// Constructors
		OverloadedPredicate(const std::string& n, unsigned int ar) : Predicate(n,std::vector<Sort*>(ar,0)) { }

		// Destructor
		~OverloadedPredicate() { }

		// Mutators
				void	overpred(Predicate* p);	

		// Inspectors
				bool							overloaded()			const { return true;		}
		virtual bool							contains(Predicate* p)	const;
		virtual Predicate*						resolve(const std::vector<Sort*>&);
		virtual Predicate*						disambiguate(const std::vector<Sort*>&,Vocabulary*);
				std::vector<Predicate*>			nonbuiltins();	//!< All non-builtin predicates 
														//!< that are overloaded by the predicate
				const std::vector<Predicate*>&	overpreds()				const { return _overpreds;	}
				std::vector<Sort*>				allsorts()				const;

		// Visitor
				void 	accept(Visitor*) const;

		// Debugging of GidL
				void		inspect()	const;
		virtual std::string	to_string()	const;

};

namespace PredUtils {
	/**
	 * return a new overloaded predicate containing the two given predicates
	 */
	Predicate* overload(Predicate* p1, Predicate* p2);

	/**
	 * return a new overloaded predicate containing the given predicates
	 */
	Predicate* overload(const std::vector<Predicate*>&);
}

/** Functions **/

class Function : public PFSymbol {
	protected:
		bool	_partial;		// true iff the function is declared as partial function
		bool	_constructor;	// true iff the function generates new domain elements

	public:
		// Constructors
		Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, const ParseInfo& pi) : 
			PFSymbol(name,is,pi), _partial(false), _constructor(false) { _sorts.push_back(os); }
		Function(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi) : 
			PFSymbol(name,sorts,pi), _partial(false), _constructor(false) { }
		Function(const std::string& name, const std::vector<Sort*>& is, Sort* os) : 
			PFSymbol(name,is), _partial(false), _constructor(false) { _sorts.push_back(os); }
		Function(const std::string& name, const std::vector<Sort*>& sorts) : 
			PFSymbol(name,sorts), _partial(false), _constructor(false) { }

		// Destructors
		virtual ~Function() { }

		// Mutators
				void	partial(const bool& b)		{ _partial = b;		}
				void	constructor(const bool& b)	{ _constructor = b;	}
	
		// Inspectors
				std::vector<Sort*>	insorts()				const;	// returns the input sorts of the function
				unsigned int		arity()					const { return _sorts.size()-1;	}
				Sort*				insort(const int& n)	const { return _sorts[n];	   	}
				Sort*				outsort()				const { return _sorts.back();  	}
				bool				partial()				const { return _partial;	   	}
				bool				constructor()			const { return _constructor;   	}
				bool				ispred()				const { return false;		   	}

		// Built-in symbols
				PredInter*		predinter(const AbstractStructure& s)	const;
		virtual	FuncInter*		inter(const AbstractStructure&)			const { return 0;	}

		// Overloaded symbols 
		virtual bool					contains(Function* f)	const { return f == this;	}
		virtual Function*				resolve(const std::vector<Sort*>&);
		virtual Function*				disambiguate(const std::vector<Sort*>&,Vocabulary*);
		virtual	std::vector<Function*>	nonbuiltins();	
		virtual	std::vector<Sort*>		allsorts()				const;	

		// Visitor
        		void accept(Visitor*) const;

};

/** Overloaded function symbols **/

class OverloadedFunction : public Function {
	protected:
		std::vector<Function*>	_overfuncs;	// the overloaded functions

	public:
		// Constructors
		OverloadedFunction(const std::string& n, unsigned int ar) : Function(n,std::vector<Sort*>(ar+1,0)) { }

		// Destructor
		~OverloadedFunction() { }

		// Mutators
				void	overfunc(Function* f);

		// Inspectors
				bool					overloaded()						const { return true;		}
		virtual bool					contains(Function* f)				const;
		virtual Function*				resolve(const std::vector<Sort*>&);
		virtual Function*				disambiguate(const std::vector<Sort*>&,Vocabulary*);
				std::vector<Function*>	nonbuiltins();	//!< All non-builtin functions 
														//!< that are overloaded by the function
		const	std::vector<Function*>&	overfuncs()							const { return _overfuncs;	}
				std::vector<Sort*>		allsorts()							const;	

		// Visitor
				void 	accept(Visitor*) const;

		// Debugging of GidL
				void		inspect()	const;
		virtual std::string	to_string()	const;

};

namespace FuncUtils {
	/**
	 * return an new overloaded function containing the two given functions
	 */
	Function* overload(Function* p1, Function* p2);

	/**
	 * return a new overloaded predicate containing the given predicates
	 */
	Function* overload(const std::vector<Function*>&);
}


/*****************
	Vocabulary
*****************/

class Vocabulary {
	private:
		std::string	_name;	//<! name of the vocabulary. Default name is the empty string
		ParseInfo	_pi;	//<! place where the vocabulary was parsed

		// map a name to a symbol of the vocabulary
		// NOTE: the strings that map to predicates and functions end on /arity
		// NOTE: if there is more than one sort/predicate/function with the same name and arity,
		// this maps to an overloaded sort/predicate/function
		std::map<std::string,std::set<Sort*> >	_name2sort;
		std::map<std::string,Predicate*>		_name2pred;
		std::map<std::string,Function*>			_name2func;

		// map non-builtin, non-overloaded symbols of the vocabulary to a unique index and vice versa
		std::map<Sort*,unsigned int>		_sort2index;		
		std::map<Predicate*,unsigned int>	_predicate2index;
		std::map<Function*,unsigned int>	_function2index;
		std::vector<Sort*>					_index2sort;
		std::vector<Predicate*>				_index2predicate;
		std::vector<Function*>				_index2function;

	public:
		// Constructors
		Vocabulary(const std::string& name); 
		Vocabulary(const std::string& name, const ParseInfo& pi); 

		// Destructor
		virtual ~Vocabulary() { }

		// Mutators
		void addSort(Sort*);		 //!< Add the given sort (and its ancestors) to the vocabulary
		void addPred(Predicate*);	 //!< Add the given predicate (and its sorts) to the vocabulary
		void addFunc(Function*);	 //!< Add the given function (and its sorts) to the vocabulary
		void addVocabulary(Vocabulary*);

		// Inspectors
		const std::string&	name()					const { return _name;					}
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

		std::map<std::string,std::set<Sort*> >::iterator	firstsort()	{ return _name2sort.begin();	}
		std::map<std::string,Predicate*>::iterator			firstpred()	{ return _name2pred.begin();	}
		std::map<std::string,Function*>::iterator			firstfunc()	{ return _name2func.begin();	}
		std::map<std::string,std::set<Sort*> >::iterator	lastsort()	{ return _name2sort.end();		}
		std::map<std::string,Predicate*>::iterator			lastpred()	{ return _name2pred.end();		}
		std::map<std::string,Function*>::iterator			lastfunc()	{ return _name2func.end();		}

		const std::set<Sort*>*	sort(const std::string&)	const;	// return the sorts with the given name
		Predicate*				pred(const std::string&)	const;	// return the predicate with the given name (ending on /arity)
		Function*				func(const std::string&)	const;	// return the function with the given name (ending on /arity)

		std::vector<Predicate*>	pred_no_arity(const std::string&)	const;	// return all predicates with the given name (not including the arity)
		std::vector<Function*>	func_no_arity(const std::string&)	const;	// return all functions with the given name (not including the arity)

		// Visitor
		void accept(Visitor*) const;

        // Lua
		TypedInfArg getObject(const std::string& str) const;

		// Debugging
		std::string to_string(unsigned int spaces = 0) const;
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
	int				_int;
	double			_double;
	std::string*	_string;
	compound*		_compound;
};

// A pair of a domain element and its type
struct TypedElement {
	Element		_element;
	ElementType	_type;

	TypedElement(Element e, ElementType t) : _element(e), _type(t) { }
	TypedElement(int n) : _type(ELINT) { _element._int = n;	}
	TypedElement(double d) : _type(ELDOUBLE) { _element._double = d;	}
	TypedElement(std::string* s) : _type(ELSTRING) { _element._string = s;	}
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
	Function*					_function;
	std::vector<TypedElement>	_args;

	compound(Function* f, const std::vector<TypedElement>& a) : _function(f), _args(a) { }
	std::string to_string() const;
};
typedef compound* domelement;

// Class that implements the relation 'less-than-or-equal' on tuples of domain elements with the same types
class ElementWeakOrdering {
	private:
		std::vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementWeakOrdering() { }
		ElementWeakOrdering(const std::vector<ElementType>& t) : _types(t) { }
		bool operator()(const std::vector<Element>&,const std::vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}
};

// Class that implements the relation 'equal' on tuples of domain elements with the same types
class ElementEquality {
	private:
		std::vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementEquality() { }
		ElementEquality(const std::vector<ElementType>& t) : _types(t) { }
		bool operator()(const std::vector<Element>&,const std::vector<Element>&) const;
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
	std::string	ElementToString(Element,ElementType);
	std::string	ElementToString(TypedElement);

	// Return the unique non-existing domain element of a given type. 
	//Non-existing domain elements are used as return value when a partial function is applied on elements outside its domain
	Element	nonexist(ElementType);				

	// Checks if the element exists, i.e., if it is not equal to the unique non-existing element
	bool	exists(Element,ElementType);	
	bool	exists(TypedElement);					

	// Convert an element from one type to another
	// If this is impossible, the non-existing element of the requested type is returned
	Element						convert(TypedElement,ElementType newtype);		
	Element						convert(Element,ElementType oldtype,ElementType newtype);
	std::vector<TypedElement>	convert(const std::vector<domelement>&);

	// Compare elements
	bool	equal(Element e1, ElementType t1, Element e2, ElementType t2);				// equality
	bool	strlessthan(Element e1, ElementType t1, Element e2, ElementType t2);		// strictly less than
	bool	lessthanorequal(Element e1, ElementType t1, Element e2, ElementType t2);	// less than or equal

	// Compare tuples
	bool	equal(const std::vector<TypedElement>&, const std::vector<Element>&, const std::vector<ElementType>&);
}

bool operator==(TypedElement e1, TypedElement e2);
bool operator!=(TypedElement e1, TypedElement e2);
bool operator<=(TypedElement e1, TypedElement e2);
bool operator<(TypedElement e1, TypedElement e2);

#endif
