/************************************
 vocabulary.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef VOCABULARY_HPP
#define VOCABULARY_HPP

#include <vector>
#include <set>
#include <map>
#include <ostream>
#include "parseinfo.hpp"

/**
 * \file vocabulary.hpp
 * 
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
	std::string _name; //!< Name of the sort
	std::set<const Vocabulary*> _vocabularies; //!< All vocabularies the sort belongs to
	std::set<Sort*> _parents; //!< The parent sorts of the sort in the sort hierarchy
	std::set<Sort*> _children; //!< The children of the sort in the sort hierarchy
	Predicate* _pred; //!< The predicate that corresponds to the sort
	ParseInfo _pi; //!< The place where the sort was declared
	SortTable* _interpretation; //!< The interpretation of the sort if it is built-in.
								//!< A null-pointer otherwise.

	~Sort(); //!< Destructor
	void removeParent(Sort* p); //!< Removes parent p
	void removeChild(Sort* c); //!< Removes child c
	void generatePred(SortTable*); //!< Generate the predicate that corresponds to the sort

	void removeVocabulary(const Vocabulary*); //!< Removes a vocabulary from the list of vocabularies
	void addVocabulary(const Vocabulary*); //!< Add a vocabulary to the list of vocabularies

public:
	// Constructors
	Sort(const std::string& name, SortTable* inter = 0); //!< Create an internal sort
	Sort(const std::string& name, const ParseInfo& pi, SortTable* inter = 0); //!< Create a user-declared sort

	// Mutators
	void addParent(Sort* p); //!< Adds p as a parent. Also adds this as a child of p.
	void addChild(Sort* c); //!< Adds c as a child. Also add this as a parent of c.

	// Inspectors
	const std::string& name() const; //!< Returns the name of the sort
	const ParseInfo& pi() const; //!< Returns the parse info of the sort
	Predicate* pred() const; //!< Returns the corresponding predicate
	const std::set<Sort*>& parents() const;
	const std::set<Sort*>& children() const;
	std::set<Sort*> ancestors(const Vocabulary* v = 0) const; //!< Returns the ancestors of the sort
	std::set<Sort*> descendents(const Vocabulary* v = 0) const; //!< Returns the descendents of the sort
	bool builtin() const; //!< True iff the sort is built-in
	SortTable* interpretation() const; //!< Returns the interpretaion for built-in sorts
	std::set<const Vocabulary*>::const_iterator  firstVocabulary() const ;
	std::set<const Vocabulary*>::const_iterator lastVocabulary() const ;

	// Output
	std::ostream& put(std::ostream&, bool longnames = false) const;
	std::string toString(bool longnames = false) const;

	friend class Vocabulary;
};

std::ostream& operator<<(std::ostream&, const Sort&);

namespace SortUtils {
Sort* resolve(Sort* s1, Sort* s2, const Vocabulary* v = 0);
//!< Return the unique nearest common ancestor of two sorts
bool isSubsort(Sort* a, Sort* b);
//!< returns true iff sort a is a subsort of sort b
}

/****************
 Variables
 ****************/

/**
 *	\brief	Class to represent variables.
 */
class Variable {
private:
	std::string _name; //!< Name of the variable
	Sort* _sort; //!< Sort of the variable (0 if the sort is not derived)
	static int _nvnr; //!< Used to create unique new names for internal variables
	ParseInfo _pi; //!< The place where the variable was quantified

public:
	// Constructors
	Variable(const std::string& name, Sort* sort, const ParseInfo& pi);
	//!< Constructor for a named variable
	Variable(Sort* s);
	//!< Constructor for an internal variable

	// Destructors
	~Variable(); //!< Destructor

	// Mutators
	void sort(Sort* s); //!< Change the sort of the variable

	// Inspectors
	const std::string& name() const; //!< Returns the name of the variable
	Sort* sort() const; //!< Returns the sort of the variable (null-pointer if the variable has no sort)
	const ParseInfo& pi() const; //!< Returns the parse info of the variable

	// Output
	std::ostream& put(std::ostream&, bool longnames = false) const;
	std::string toString(bool longnames = false) const;
};

std::ostream& operator<<(std::ostream&, const Variable&);

namespace VarUtils {
/**
 * Make a vector of fresh variables of given sorts.
 */
std::vector<Variable*> makeNewVariables(const std::vector<Sort*>&);
}

/*************************************
 Predicate and function symbols
 *************************************/

enum SymbolType {
	ST_NONE, ST_CT, ST_CF, ST_PT, ST_PF
};

/** 
 *	\brief	Abstract base class to represent predicate and function symbols
 */
class PFSymbol {
protected:
	std::string _name; //!< Name of the symbol (ending with the /arity)
	ParseInfo _pi; //!< The place where the symbol was declared
	std::set<const Vocabulary*> _vocabularies; //!< All vocabularies the symbol belongs to
	std::vector<Sort*> _sorts; //!< Sorts of the arguments.
							   //!< For a function symbol, the last sort is the output sort.
	bool _infix; //!< True iff the symbol is infix

	std::map<SymbolType, Predicate*> _derivedsymbols; //!< The symbols this<ct>, this<cf>, this<pt>, and this<pf>

	virtual ~PFSymbol();

public:
	// Constructors
	PFSymbol(const std::string& name, size_t nrsorts, bool infix = false);
	PFSymbol(const std::string& name, const std::vector<Sort*>& sorts, bool infix = false);
	PFSymbol(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi, bool infix = false);

	// Mutators
	virtual bool removeVocabulary(const Vocabulary*) = 0; //!< Removes a vocabulary from the list of vocabularies
	virtual void addVocabulary(const Vocabulary*) = 0; //!< Add a vocabulary to the list of vocabularies

	// Inspectors
	const std::string& name() const; //!< Returns the name of the symbol (ends on /arity)
	const ParseInfo& pi() const; //!< Returns the parse info of the symbol
	size_t nrSorts() const; //!< Returns the number of sorts of the symbol
							//!< (arity for predicates, arity+1 for functions)
	Sort* sort(size_t n) const; //!< Returns the n'th sort of the symbol
	const std::vector<Sort*>& sorts() const; //!< Returns the sorts of the symbol
	bool infix() const; //!< True iff the symbol is infix
	bool hasVocabularies() const; //!< Returns true iff the symbol occurs in a
								  //!< vocabulary
	Predicate* derivedSymbol(SymbolType); //!< Return the derived symbol of the given type
	std::vector<unsigned int> argumentNrs(const Sort*) const; //!< Returns the numbers of the arguments where this
															  //!< PFSymbol ranges over the given sort

	virtual bool builtin() const = 0; //!< Returns true iff the symbol is built-in
	virtual bool overloaded() const = 0; //!< Returns true iff the symbol is in fact a set of overloaded
										 //!< symbols
	virtual std::set<Sort*> allsorts() const = 0; //!< Return all sorts that occur in the (overloaded) symbol(s)

	// Disambiguate overloaded symbols
	virtual PFSymbol* resolve(const std::vector<Sort*>&) = 0;
	virtual PFSymbol* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0) = 0;

	// Output
	virtual std::ostream& put(std::ostream&, bool longnames = false) const = 0;
	std::string toString(bool longnames = false) const;

	friend class Vocabulary;
};

std::ostream& operator<<(std::ostream&, const PFSymbol&);

class PredGenerator;
class PredInter;
class PredInterGenerator;
class AbstractStructure;

/**
 * \brief	Class to represent predicate symbols
 */
class Predicate: public PFSymbol {
private:
	SymbolType _type; //!< The type of the symbol
	PFSymbol* _parent; //!< The symbol this predicate is derived from.
					   //!< Nullpointer if _type == ST_NONE.
	static int _npnr; //!< Used to create unique new names for internal predicates
	PredInterGenerator* _interpretation; //!< The interpretation if the predicate is built-in, a null-pointer
										 //!< otherwise.
	PredGenerator* _overpredgenerator; //!< Generates new built-in, overloaded predicates.
									   //!< Null-pointer if the predicate is not overloaded.

public:
	// Constructors
	Predicate(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi, bool infix = false);
	Predicate(const std::string& name, const std::vector<Sort*>& sorts, bool infix = false);
	Predicate(const std::vector<Sort*>& sorts); //!< constructor for internal/tseitin predicates
	Predicate(const std::string& name, const std::vector<Sort*>& sorts, PredInterGenerator* inter, bool infix);
	Predicate(PredGenerator* generator);

	~Predicate(); //!< Destructor

	// Mutators
	bool removeVocabulary(const Vocabulary*); //!< Removes a vocabulary from the list of vocabularies
	void addVocabulary(const Vocabulary*); //!< Add a vocabulary to the list of vocabularies
	void type(SymbolType, PFSymbol* parent); //!< Set the type and parent of the predicate

	// Inspectors
	SymbolType type() const {
		return _type;
	}
	PFSymbol* parent() const {
		return _parent;
	}
	unsigned int arity() const; //!< Returns the arity of the predicate
	bool builtin() const;
	bool overloaded() const;
	bool isSortPredicate() const; //!< Returns true iff this is a predicate representing a sort
	std::set<Sort*> allsorts() const;

	// Built-in symbols
	PredInter* interpretation(const AbstractStructure*) const;

	// Overloaded symbols
	bool contains(const Predicate* p) const;
	Predicate* resolve(const std::vector<Sort*>&);
	Predicate* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0);
	std::set<Predicate*> nonbuiltins(); //!< Returns the set of predicates that are not builtin
										//!< and that are overloaded by 'this'.

										// Output
	std::ostream& put(std::ostream&, bool longnames = false) const;

	friend class Sort;
	friend class Vocabulary;
	friend class PredGenerator;
};

std::ostream& operator<<(std::ostream&, const Predicate&);

/**
 * Class to overload predicates.
 */
class PredGenerator {
protected:
	std::string _name; //!< The name of the generated predicates
	unsigned int _arity; //!< The arity of the generated predicates
	bool _infix; //!< True iff the generated predicates are infix
public:
	virtual ~PredGenerator() {
	}

	PredGenerator(const std::string& name, unsigned int arity, bool infix);

	const std::string& name() const; //!< Returns the name of the generated predicates
	unsigned int arity() const; //!< Returns the arity of the generated predicates
	bool infix() const; //!< Returns true iff the generated predicates are infix

	virtual bool contains(const Predicate* predicate) const = 0;
	virtual Predicate* resolve(const std::vector<Sort*>&) = 0;
	virtual Predicate* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0) = 0;
	virtual std::set<Sort*> allsorts() const = 0;
	//!< Returns all sorts that occur in the predicates generated by the predicate generator
	virtual void addVocabulary(const Vocabulary*) = 0; //!< Add a vocabulary to all overloaded predicates
	virtual void removeVocabulary(const Vocabulary*) = 0; //!< Remove a vocabulary from all overloaded predicates

	virtual std::set<Predicate*> nonbuiltins() const = 0;
};

/**
 * PredGenerator containing a finite, enumerated number of predicates
 */
class EnumeratedPredGenerator: public PredGenerator {
private:
	std::set<Predicate*> _overpreds; //!< The overloaded predicates
public:
	~EnumeratedPredGenerator() {
	}

	EnumeratedPredGenerator(const std::set<Predicate*>&);

	bool contains(const Predicate* predicate) const;
	Predicate* resolve(const std::vector<Sort*>&);
	Predicate* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0);
	std::set<Sort*> allsorts() const;
	void addVocabulary(const Vocabulary*);
	void removeVocabulary(const Vocabulary*);

	std::set<Predicate*> nonbuiltins() const;
};

class PredInterGeneratorGenerator;

/**
 * Class to generate new predicates that are overloaded by </2, >/2, or =/2
 */
class ComparisonPredGenerator: public PredGenerator {
private:
	mutable std::map<Sort*, Predicate*> _overpreds;
	PredInterGeneratorGenerator* _interpretation;
public:
	ComparisonPredGenerator(const std::string& name, PredInterGeneratorGenerator* inter);
	~ComparisonPredGenerator();

	bool contains(const Predicate* predicate) const;

	Predicate* resolve(const std::vector<Sort*>&);
	Predicate* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0);
	std::set<Sort*> allsorts() const;
	void addVocabulary(const Vocabulary*);
	void removeVocabulary(const Vocabulary*);

	std::set<Predicate*> nonbuiltins() const;
};

namespace PredUtils {
/**
 * \brief Return a new overloaded predicate containing the two given predicates
 */
Predicate* overload(Predicate* p1, Predicate* p2);

/**
 * \brief Return a new overloaded predicate containing the given predicates
 */
Predicate* overload(const std::set<Predicate*>&);

}

class FuncGenerator;
class FuncInter;
class FuncInterGenerator;

/**
 * \brief	Class to represent function symbols
 */
class Function: public PFSymbol {

private:
	bool _partial; //!< true iff the function is declared as partial function
	std::vector<Sort*> _insorts; //!< the input sorts of the function symbol
	Sort* _outsort; //!< the output sort of the function symbol
	FuncInterGenerator* _interpretation; //!< the interpretation if the function is built-in, a null-pointer
										 //!< otherwise
	FuncGenerator* _overfuncgenerator; //!< generates new built-in, overloaded functions.
									   //!< Null-pointer if the function is not overloaded.
	unsigned int _binding; //!< Binding strength of infix functions.

public:
	// Constructors
	Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, const ParseInfo& pi, unsigned int binding = 0);
	Function(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi, unsigned int binding = 0);
	Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, unsigned int binding = 0);
	Function(const std::string& name, const std::vector<Sort*>& sorts, unsigned int binding = 0);
	Function(const std::string& name, const std::vector<Sort*>& sorts, FuncInterGenerator*, unsigned int binding);
	Function(FuncGenerator*);

	~Function();

	// Mutators
	void partial(bool b); //!< Make the function total/partial if b is false/true
	bool removeVocabulary(const Vocabulary*); //!< Removes a vocabulary from the list of vocabularies
	void addVocabulary(const Vocabulary*); //!< Add a vocabulary to the list of vocabularies

	// Inspectors
	const std::vector<Sort*>& insorts() const; //!< Return the input sorts of the function
	unsigned int arity() const; //!< Returns the arity of the function
	Sort* insort(unsigned int n) const; //!< Returns the n'th input sort of the function
	Sort* outsort() const; //!< Returns the output sort of the function
	bool partial() const; //!< Returns true iff the function is partial
	bool builtin() const;
	bool overloaded() const;
	unsigned int binding() const; //!< Returns binding strength
	std::set<Sort*> allsorts() const;

	// Built-in symbols
	FuncInter* interpretation(const AbstractStructure*) const;

	// Overloaded symbols
	bool contains(const Function* f) const;
	Function* resolve(const std::vector<Sort*>&);
	Function* disambiguate(const std::vector<Sort*>&, const Vocabulary*);
	std::set<Function*> nonbuiltins(); //!< Returns the set of predicates that are not builtin
									   //!< and that are overloaded by 'this'.

									   // Output
	std::ostream& put(std::ostream&, bool longnames = false) const;
	std::string toString(bool longnames = false) const;

	friend class Vocabulary;
};

std::ostream& operator<<(std::ostream&, const Function&);

/**
 * Class to generate new function.
 * Used to represent the infinite number of function that are overloaded by some built-in functions (e.g. SUCC/1)
 */
class FuncGenerator {
protected:
	std::string _name; //!< The name of the generated functions
	unsigned int _arity; //!< The arity of the generated functions
	unsigned int _binding; //!< The binding strength of the generated functions

public:
	virtual ~FuncGenerator() {
	}

	FuncGenerator(const std::string& name, unsigned int arity, unsigned int binding) :
			_name(name), _arity(arity), _binding(binding) {
	}

	const std::string& name() const; //!< Returns the name of the generated functions
	unsigned int arity() const; //!< Returns the arity of the generated functions
	unsigned int binding() const; //!< Returns the binding strength of the generated functions

	virtual bool contains(const Function* function) const = 0;
	virtual Function* resolve(const std::vector<Sort*>&) = 0;
	virtual Function* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0) = 0;
	virtual std::set<Sort*> allsorts() const = 0;
	virtual void addVocabulary(const Vocabulary*) = 0;
	virtual void removeVocabulary(const Vocabulary*) = 0;

	virtual std::set<Function*> nonbuiltins() const = 0;
};

/**
 * FuncGenerator containing a finite, enumerated number of functions
 */
class EnumeratedFuncGenerator: public FuncGenerator {
private:
	std::set<Function*> _overfuncs; //!< The overloaded predicates
public:
	~EnumeratedFuncGenerator() {
	}

	EnumeratedFuncGenerator(const std::set<Function*>&);

	bool contains(const Function* function) const;
	Function* resolve(const std::vector<Sort*>&);
	Function* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0);
	std::set<Sort*> allsorts() const;
	void addVocabulary(const Vocabulary*);
	void removeVocabulary(const Vocabulary*);

	std::set<Function*> nonbuiltins() const;
};

/**
 * FuncGenerator containing two functions: one with sorts [int,int:int], one with sorts [float,float:float].
 * Used for many standard arithmetic functions.
 */
class IntFloatFuncGenerator: public FuncGenerator {
private:
	Function* _intfunction;
	Function* _floatfunction;
public:

	IntFloatFuncGenerator(Function* intfunc, Function* floatfunc);
	~IntFloatFuncGenerator() {
	}

	bool contains(const Function* function) const;
	Function* resolve(const std::vector<Sort*>&);
	Function* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0);
	std::set<Sort*> allsorts() const;
	void addVocabulary(const Vocabulary*);
	void removeVocabulary(const Vocabulary*);

	std::set<Function*> nonbuiltins() const;
};

class FuncInterGeneratorGenerator;

/**
 * Class to overload the functions MIN/0, MAX/0, SUCC/1, and PRED/1
 */
class OrderFuncGenerator: public FuncGenerator {
private:
	mutable std::map<Sort*, Function*> _overfuncs;
	FuncInterGeneratorGenerator* _interpretation;
public:
	OrderFuncGenerator(const std::string& name, unsigned int arity, FuncInterGeneratorGenerator* inter);
	~OrderFuncGenerator();

	bool contains(const Function* function) const;
	Function* resolve(const std::vector<Sort*>&);
	Function* disambiguate(const std::vector<Sort*>&, const Vocabulary* v = 0);
	std::set<Sort*> allsorts() const;
	void addVocabulary(const Vocabulary*);
	void removeVocabulary(const Vocabulary*);

	std::set<Function*> nonbuiltins() const;
};

namespace FuncUtils {
/**
 * return an new overloaded function containing the two given functions
 */
Function* overload(Function* p1, Function* p2);

/**
 * return a new overloaded function containing the given functions
 */
Function* overload(const std::set<Function*>&);

/**
 * check whether the output sort of a function is integer
 */
bool isIntFunc(const Function*, const Vocabulary*);

/**
 * check whether the function is a sum over integers
 */
bool isIntSum(const Function* function, const Vocabulary* voc);
}

/*****************
 Vocabulary
 *****************/

class InfArg;
class Namespace;

class Vocabulary {
private:
	std::string _name; //!< Name of the vocabulary. Default name is the empty string.
	ParseInfo _pi; //!< Place where the vocabulary was parsed
	Namespace* _namespace; //!< The namespace the vocabulary belongs to.

	std::map<std::string, std::set<Sort*> > _name2sort; //!< Map a name to the sorts having that name in the vocabulary
	std::map<std::string, Predicate*> _name2pred; //!< Map a name to the (overloaded) predicate having
												  //!< that name in the vocabulary. Name should end on /arity.
	std::map<std::string, Function*> _name2func; //!< Map a name to the (overloaded) function having
												 //!< that name in the vocabulary. Name should end on /arity.

	static Vocabulary* _std; //!< The standard vocabulary

public:
	Vocabulary(const std::string& name);
	Vocabulary(const std::string& name, const ParseInfo& pi);

	~Vocabulary();

	// Mutators
	void add(Sort*); //!< Add the given sort (and its ancestors) to the vocabulary
	void add(PFSymbol*); //!< Add the given predicate (and its sorts) to the vocabulary
	void add(Predicate*); //!< Add the given predicate (and its sorts) to the vocabulary
	void add(Function*); //!< Add the given function (and its sorts) to the vocabulary
	void add(Vocabulary*); //!< Add all symbols of a given vocabulary to the vocabulary
	void setNamespace(Namespace* n) {
		_namespace = n;
	}

	// Inspectors
	static Vocabulary* std(); //!< Returns the standard vocabulary
	const std::string& name() const; //!< Returns the name of the vocabulary
	const ParseInfo& pi() const; //!< Returns the parse info of the vocabulary
	bool contains(const Sort* s) const; //!< True iff the vocabulary contains the sort
	bool contains(const Predicate* p) const; //!< True iff the vocabulary contains the predicate
	bool contains(const Function* f) const; //!< True iff the vocabulary contains the function
	bool contains(const PFSymbol* s) const; //!< True iff the vocabulary contains the symbol

	std::map<std::string, std::set<Sort*> >::iterator firstSort() {
		return _name2sort.begin();
	}
	std::map<std::string, Predicate*>::iterator firstPred() {
		return _name2pred.begin();
	}
	std::map<std::string, Function*>::iterator firstFunc() {
		return _name2func.begin();
	}
	std::map<std::string, std::set<Sort*> >::iterator lastSort() {
		return _name2sort.end();
	}
	std::map<std::string, Predicate*>::iterator lastPred() {
		return _name2pred.end();
	}
	std::map<std::string, Function*>::iterator lastFunc() {
		return _name2func.end();
	}

	std::map<std::string, std::set<Sort*> >::const_iterator firstSort() const {
		return _name2sort.cbegin();
	}
	std::map<std::string, Predicate*>::const_iterator firstPred() const {
		return _name2pred.cbegin();
	}
	std::map<std::string, Function*>::const_iterator firstFunc() const {
		return _name2func.cbegin();
	}
	std::map<std::string, std::set<Sort*> >::const_iterator lastSort() const {
		return _name2sort.cend();
	}
	std::map<std::string, Predicate*>::const_iterator lastPred() const {
		return _name2pred.cend();
	}
	std::map<std::string, Function*>::const_iterator lastFunc() const {
		return _name2func.cend();
	}

	const std::set<Sort*>* sort(const std::string&) const;
	//!< return the sorts with the given name
	Predicate* pred(const std::string&) const;
	//!< return the predicate with the given name (ending on /arity)
	Function* func(const std::string&) const;
	//!< return the function with the given name (ending on /arity)

	std::set<Predicate*> pred_no_arity(const std::string&) const;
	//!< return all predicates with the given name (not including the arity)
	std::set<Function*> func_no_arity(const std::string&) const;
	//!< return all functions with the given name (not including the arity)

	// Lua
	InfArg getObject(const std::string& str) const;

	// Output
	std::ostream& putName(std::ostream&) const;
	std::ostream& put(std::ostream&, size_t tabs = 0, bool longnames = false) const;
	std::string toString(size_t tabs = 0, bool longnames = false) const;

	friend class Namespace;
};

std::ostream& operator<<(std::ostream&, const Vocabulary&);

namespace VocabularyUtils {
Sort* natsort(); //!< returns the sort 'nat' of the standard vocabulary
Sort* intsort(); //!< returns the sort 'int' of the standard vocabulary
Sort* floatsort(); //!< returns the sort 'float' of the standard vocabulary
Sort* stringsort(); //!< returns the sort 'string' of the standard vocabulary
Sort* charsort(); //!< returns the sort 'char' of the standard vocabulary

Predicate* equal(Sort* s); //!< returns the predicate =/2 with sorts (s,s)
Predicate* lessThan(Sort* s); //!< returns the predicate </2 with sorts (s,s)
Predicate* greaterThan(Sort* s); //!< returns the predicate >/2 with sorts (s,s)

bool isComparisonPredicate(const PFSymbol*); //!< returns true iff the given symbol is =/2, </2, or >/2
bool isNumeric(Sort*); //!< returns true iff the given sort is a subsort of float

}

#endif
