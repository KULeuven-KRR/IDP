/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef STRUCTURE_HPP_
#define STRUCTURE_HPP_

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
			: AbstractStructure(name, v, pi) {
		changeVocabulary(v);
	}
	~Structure();

	// Mutators
	void changeVocabulary(Vocabulary* v); //!< CHANGE the vocabulary of the structure

	void addStructure(AbstractStructure*);

	void changeInter(Predicate* p, PredInter* i); //!< CHANGES the interpretation of p to i
	void changeInter(Function* f, FuncInter* i); //!< CHANGES the interpretation of f to i

	void clean(); //!< Try to represent two-valued interpretations by one table instead of two.
	void materialize(); //!< Convert symbolic tables containing a finite number of tuples to enumerated tables.

	void sortCheck() const; // Checks whether any sorts are empty and throws a warning for those
	void functionCheck(); //!< check the correctness of the function tables
	void autocomplete(); //!< make the domains consistent with the predicate and function tables

	// Inspectors
	SortTable* inter(Sort* s) const; //!< Return the domain of s.
	PredInter* inter(Predicate* p) const; //!< Return the interpretation of p.
	FuncInter* inter(Function* f) const; //!< Return the interpretation of f.
	PredInter* inter(PFSymbol* s) const; //!< Return the interpretation of s.
	Structure* clone() const; //!< take a clone of this structure
	bool approxTwoValued() const;
	bool isConsistent() const;

	virtual const std::map<Predicate*, PredInter*>& getPredInters() const {
		return _predinter;
	}
	virtual const std::map<Function*, FuncInter*>& getFuncInters() const {
		return _funcinter;
	}

	void makeTwoValued();

	Universe universe(const PFSymbol*) const;
};

#endif /* STRUCTURE_HPP_ */
