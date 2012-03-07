/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef SORTINSTGENERATOR_HPP_
#define SORTINSTGENERATOR_HPP_

#include "InstGenerator.hpp"

/**
 * Instantiate a given variable with all possible values for the given sort
 */
class SortInstGenerator: public InstGenerator {
private:
	const InternalSortTable* _table;
	const DomElemContainer* _var;
	SortIterator _curr;
	bool _reset;
public:
	SortInstGenerator(const InternalSortTable* table, const DomElemContainer* var)
			: _table(table), _var(var), _curr(_table->sortBegin()), _reset(true) {
	}

	SortInstGenerator* clone() const {
		return new SortInstGenerator(*this);
	}

	void reset() {
		_reset = true;
	}

	void setVarsAgain() {
		*_var = *_curr;
	}

	void next() {
		if (_reset) {
			_reset = false;
			_curr = _table->sortBegin();
		} else {
			++_curr;
		}
		if (_curr.isAtEnd()) {
			notifyAtEnd();
		} else {
			*_var = *_curr;
		}
	}

	virtual void put(std::ostream& stream) {
		pushtab();
		stream << "generator for sort: " << toString(_table);
		poptab();
	}
};

#endif /* SORTINSTGENERATOR_HPP_ */
