/************************************
	SortInstGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SORTINSTGENERATOR_HPP_
#define SORTINSTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class SortInstGenerator : public InstGenerator {
	private:
		const InternalSortTable*	_table;
		const DomElemContainer*		_outvar;
		mutable SortIterator		_currpos;
	public:
		SortInstGenerator(const InternalSortTable* t, const DomElemContainer* out) :
			_table(t), _outvar(out), _currpos(t->sortBegin()) { }
		bool first() const;
		bool next() const;
};


#endif /* SORTINSTGENERATOR_HPP_ */
