/************************************
 SortLookUpGenerator.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef SORTLOOKUPGENERATOR_HPP_
#define SORTLOOKUPGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

/**
 * Generate a domelem if the input is in the sorttable (TODO is in fact a checker).
 */
class SortLookUpGenerator: public InstGenerator {
private:
	const InternalSortTable* _table;
	const DomElemContainer* _invar;
	bool _reset;
public:
	SortLookUpGenerator(const InternalSortTable* t, const DomElemContainer* in)
			: _table(t), _invar(in), _reset(true) {
	}

	void reset() {
		_reset = true;
	}

	void next() {
		if(_reset){
			if(not _table->contains(_invar->get())){
				notifyAtEnd();
			}
			_reset = false;
			return;
		}
		notifyAtEnd();
	}
};

#endif /* SORTLOOKUPGENERATOR_HPP_ */
