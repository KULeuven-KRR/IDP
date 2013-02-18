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

#include "AbstractStructure.hpp"

class TableIterator;

class Structure: public AbstractStructure {
private:
	std::map<Sort*, SortTable*> _sortinter; //!< The domains of the structure.
	std::map<Predicate*, PredInter*> _predinter; //!< The interpretations of the predicate symbols.
	std::map<Function*, FuncInter*> _funcinter; //!< The interpretations of the function symbols.

	mutable std::vector<PredInter*> _intersToDelete; // Interpretations which were created and not yet deleted // TODO do this in a cleaner way!
	void canIncrement(TableIterator & domainIterator) const;

public:
	Structure(const std::string& name, const ParseInfo& pi)
			: AbstractStructure(name, pi) {
	}
	Structure(const std::string& name, Vocabulary* v, const ParseInfo& pi)
			: AbstractStructure(name, pi) {
		changeVocabulary(v);
	}
	~Structure();

	// Mutators
	void changeVocabulary(Vocabulary* v); //!< CHANGE the vocabulary of the structure

	void addStructure(AbstractStructure*);

	void changeInter(Sort* f, SortTable* i); // CHANGES the interpretation of f to i
	void changeInter(Predicate* p, PredInter* i); // CHANGES the interpretation of p to i
	void changeInter(Function* f, FuncInter* i); // CHANGES the interpretation of f to i

	void clean(); //!< Try to represent two-valued interpretations by one table instead of two.
	void materialize(); //!< Convert symbolic tables containing a finite number of tuples to enumerated tables.

	void sortCheck() const; // Checks whether any sorts are empty and throws a warning for those
	// Check whether no term maps to multiple elements and whether non-partial functions are specified completely.
	void functionCheck();
	void checkAndAutocomplete(); //!< make the domains consistent with the predicate and function tables

	// Inspectors
	virtual bool hasInter(const Sort* s) const;
	virtual SortTable* inter(const Sort* s) const; // Return the domain of s.
	virtual PredInter* inter(const Predicate* p) const; // Return the interpretation of p.
	virtual FuncInter* inter(const Function* f) const; // Return the interpretation of f.
	virtual PredInter* inter(const PFSymbol* s) const; // Return the interpretation of s.
	Structure* clone() const; //!< take a clone of this structure
	bool approxTwoValued() const;
	bool isConsistent() const;

	virtual const std::map<Sort*, SortTable*>& getSortInters() const {
		return _sortinter;
	}
	virtual const std::map<Predicate*, PredInter*>& getPredInters() const {
		return _predinter;
	}
	virtual const std::map<Function*, FuncInter*>& getFuncInters() const {
		return _funcinter;
	}

	void makeTwoValued();

	Universe universe(const PFSymbol*) const;
};

/**
 * Changes the interpretation to make all unknown atoms false by
 * setting pt to ct and then cf and pf to NOT ct.
 */
void makeUnknownsFalse(PredInter* inter);
