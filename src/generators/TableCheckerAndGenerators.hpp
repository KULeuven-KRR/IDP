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

#ifndef GENERATEANDTESTGENERATOR_HPP_
#define GENERATEANDTESTGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "structure/Universe.hpp"
#include "structure/MainStructureComponents.hpp"
#include "commontypes.hpp"

class PredTable;
class DomElemContainer;
class DomainElement;


/**
 * A generator which checks whether a fully instantiated list of variables is a valid tuple for a certain predicate
 */
class TableChecker: public InstGenerator {
private:
	const PredTable* _table;
	std::vector<const DomElemContainer*> _vars;
	Universe _universe;
	bool _reset;
	std::vector<const DomainElement*> _currargs;

public:
	// NOTE: does not take ownership of the table
	TableChecker(const PredTable* t, const std::vector<const DomElemContainer*>& vars, const Universe& univ);
	TableChecker* clone() const;
	virtual void internalSetVarsAgain();
	void reset();
	void next();
	virtual void put(std::ostream& stream) const;
};

class TableGenerator: public InstGenerator {
private:
	const PredTable* _fulltable;
	PredTable* _outputtable;
	std::vector<const DomElemContainer*> _allvars;
	std::vector<const DomElemContainer*> _outvars;
	std::vector<unsigned int> _outvaroccurence, _uniqueoutvarindex;
	ElementTuple _currenttuple;
	bool _reset;
	TableIterator _current;
public:
	TableGenerator(const PredTable* table, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars,
			const std::vector<unsigned int>& firstocc, const Universe& univ);
	~TableGenerator();
	TableGenerator* clone() const;
	void reset();
	void next();

private:
	bool inFullTable();
};

/**
 * Given a predicate table, generate tuples which, given the input, are not in the predicate table
 */
// TODO optimize generators if we can guarantee that sorts are always traversed from smallest to largest. This leads to problems as it might be easier to iterate
// over Z as 0 1 -1 2 -2 to avoid having to start at -infinity
// TODO Code below is correct, but can be optimized in case there are output variables that occur more than once in 'vars'
// TODO can probably be optimized a lot if with the order in which we run over it.
class InverseTableGenerator: public InstGenerator {
private:
	InstGenerator *_universegen;
	InstChecker *_predchecker;
	bool _reset;

public:
	// NOTE: Does not take ownership of table;
	InverseTableGenerator(PredTable* table, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars);
	InverseTableGenerator* clone() const;
	virtual void internalSetVarsAgain();
	void reset();
	void next();
	virtual void put(std::ostream& stream) const;
};

#endif /* GENERATEANDTESTGENERATOR_HPP_ */
