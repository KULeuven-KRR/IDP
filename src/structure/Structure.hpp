/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include "common.hpp"
#include "parseinfo.hpp"

#include "Universe.hpp"

class Function;
class FuncInter;
class Predicate;
class PredInter;
class PFSymbol;
class Sort;
class SortTable;
class Vocabulary;
class TableIterator;

class Structure {
private:
	std::string _name; // The name of the structure
	ParseInfo _pi; // The place where this structure was parsed.
	Vocabulary* _vocabulary; // The vocabulary of the structure.

	std::map<Sort*, SortTable*> _sortinter; //!< The domains of the structure.
	std::map<Predicate*, PredInter*> _predinter; //!< The interpretations of the predicate symbols.
	std::map<Function*, FuncInter*> _funcinter; //!< The interpretations of the function symbols.
	mutable std::map<const Function*, FuncInter*> _fixedfuncinter; //!< The interpretations of the function symbols.

	mutable std::vector<PredInter*> _intersToDelete; // Interpretations which were created and not yet deleted // TODO do this in a cleaner way!
	void canIncrement(TableIterator & domainIterator) const;

public:
	Structure(const std::string& name, const ParseInfo& pi);
	Structure(const std::string& name, Vocabulary* v, const ParseInfo& pi);
	~Structure();

	void addInterToDelete(PredInter* pi) const {
		_intersToDelete.push_back(pi);
	}

	// Mutators
	void notifyAddedToVoc(Sort* sort);
	void notifyAddedToVoc(PFSymbol* symbol);
	void changeVocabulary(Vocabulary* v); //!< CHANGE the vocabulary of the structure

	void addStructure(Structure*);

	void changeInter(Sort* f, SortTable* i); // CHANGES the interpretation of f to i
	void changeInter(Predicate* p, PredInter* i); // CHANGES the interpretation of p to i
	void changeInter(Function* f, FuncInter* i); // CHANGES the interpretation of f to i

	void clean(); //!< Try to represent two-valued interpretations by one table instead of two.
	void materialize(); //!< Convert symbolic tables containing a finite number of tuples to enumerated tables.
	void reset(); //<! resets pred- and func inters to a state where everyhing is unknown, and resets sorttables for all pred and func inters.
	void createPredAndFuncTables(bool forced); //<! creates predicate and function tables. If forced is true, this is in fact a reset, otherwise, only tables for symbols without interpretation are created.

	void sortCheck() const; // Checks whether any sorts are empty and throws a warning for those
	// Check whether no term maps to multiple elements and whether non-partial functions are specified completely.
	bool satisfiesFunctionConstraints(bool throwerrors = false) const;
	bool satisfiesFunctionConstraints(const Function* f, bool throwerrors = false) const;
	void checkAndAutocomplete(); //!< make the domains consistent with the predicate and function tables

	// Inspectors
	bool hasInter(const Sort* s) const;
	SortTable* inter(const Sort* s) const; // Return the domain of s. NOTE: it is unsafe to store this SortTable
	SortTable* storableInter(const Sort* s) const; // Return a clone of the domain of s. NOTE: it is safe to store this
PredInter* inter(const Predicate* p) const; // Return the interpretation of p.
	FuncInter* inter(const Function* f) const; // Return the interpretation of f.
	PredInter* inter(const PFSymbol* s) const; // Return the interpretation of s.
	Structure* clone() const; //!< take a clone of this structure
	bool approxTwoValued() const;
	bool isConsistent() const;

	const std::map<Sort*, SortTable*>& getSortInters() const {
		return _sortinter;
	}
	const std::map<Predicate*, PredInter*>& getPredInters() const {
		return _predinter;
	}
	const std::map<Function*, FuncInter*>& getFuncInters() const {
		return _funcinter;
	}

	void makeTwoValued();

	Universe universe(const PFSymbol*) const;

	void name(std::string& name){
		_name = name;
	}

	const std::string& name() const {
		return _name;
	}
	ParseInfo pi() const {
		return _pi;
	}
	Vocabulary* vocabulary() const {
		return _vocabulary;
	}

	void put(std::ostream& s) const;

private:
	bool functionCheck(const Function* f, bool throwErrors = false) const;
	void autocompleteFromSymbol(PFSymbol* symbol, PredInter* inter);
};

void clean(Predicate*, PredInter* inter);
void clean(Function* function, FuncInter* inter);

/**
 * Changes the interpretation to make all unknown atoms false by
 * setting pt to ct and then cf and pf to NOT ct.
 * NOTE: might result in an inconsistent structure for function symbols!
 */
void makeUnknownsFalse(PredInter* inter);
void makeUnknownsFalse(Structure* structure);

void makeTwoValued(Function* function, FuncInter* inter);
void makeTwoValued(Predicate* p, PredInter* inter);
