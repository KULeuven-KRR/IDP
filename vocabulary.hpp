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
#include "parseinfo.hpp"
#include "visitor.hpp"
#include "lua.hpp"

class AbstractStructure;
struct compound;
struct TypedInfArg;

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

	/**
	 * Check whether the output sort of a function is integer.
	 */
	bool isIntFunc(const Function*, Vocabulary*);
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

#endif
