/************************************
	SortInstGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SORTINSTGENERATOR_HPP_
#define SORTINSTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

/**
 * Instantiate a given variable with all possible values for the given sort
 */
class SortInstGenerator : public InstGenerator {
private:
	const InternalSortTable*	_table;
	const DomElemContainer*		_var;
	SortIterator				_curr;
public:
	SortInstGenerator(const InternalSortTable* table, const DomElemContainer* var)
			:_table(table), _var(var), _curr(_table->sortBegin()) {
	}

	void reset(){
		_curr = _table->sortBegin();
		if(_curr.isAtEnd()){
			notifyAtEnd();
		}
	}

	void next(){
		*_var = *_curr;
		++_curr;
	}
};


#endif /* SORTINSTGENERATOR_HPP_ */
