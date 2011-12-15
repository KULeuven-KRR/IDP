/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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

	SortLookUpGenerator* clone() const{
		return new SortLookUpGenerator(*this);
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
