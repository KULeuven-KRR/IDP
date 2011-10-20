/************************************
	SortLookUpGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SORTLOOKUPGENERATOR_HPP_
#define SORTLOOKUPGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class SortLookUpGenerator : public InstGenerator {
	private:
		const InternalSortTable*	_table;
		const DomElemContainer*		_invar;
	public:
		SortLookUpGenerator(const InternalSortTable* t, const DomElemContainer* in) : _table(t), _invar(in) { }
		bool first()	const { return _table->contains(_invar->get());	}
		bool next()		const { return false;						}
};


#endif /* SORTLOOKUPGENERATOR_HPP_ */
