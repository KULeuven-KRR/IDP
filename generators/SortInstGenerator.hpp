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
	bool _reset;
public:
	SortInstGenerator(const InternalSortTable* table, const DomElemContainer* var)
			:_table(table), _var(var), _curr(_table->sortBegin()), _reset(true) {
	}

	SortInstGenerator* clone() const{
		return new SortInstGenerator(*this);
	}

	void reset(){
		_reset = true;
	}

	void next(){
		if(_reset){
			_reset = false;
			_curr = _table->sortBegin();
		}else{
			++_curr;
		}
		if(_curr.isAtEnd()){
			notifyAtEnd();
		}else{
			*_var = *_curr;
		}
	}
};

#endif /* SORTINSTGENERATOR_HPP_ */
