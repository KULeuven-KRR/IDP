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
	bool first;
public:
	SortInstGenerator(const InternalSortTable* table, const DomElemContainer* var)
			:_table(table), _var(var), first(true) {
		if(_table->approxEmpty()){ // FIXME what if approxempty is false, but it is in fact empty?
									// FIXME 2 is also not checked everywhere (in other generator)
			notifyAtEnd();
		}
	}

	void reset(){
		first = true;
		if(_table->approxEmpty()){ // FIXME what if approxempty is false, but it is in fact empty?
			notifyAtEnd();
		}
	}

	void next(){ // TODO check invariant everywhere that reset is called before first next
		if(first){
			_curr = _table->sortBegin(); // FIXME also check whether the curr it not immediately at end!
			first = false; // FIXME this is quite bug-prone, change it?
		}else{
			++_curr;
		}
		*var = *_curr;
		if(_curr.isAtEnd()){
			notifyAtEnd();
		}
	}
};


#endif /* SORTINSTGENERATOR_HPP_ */
