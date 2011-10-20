/************************************
	EnumLookupGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ENUMLOOKUPGENERATOR_HPP_
#define ENUMLOOKUPGENERATOR_HPP_

#include <map>
#include "generators/InstGenerator.hpp"

typedef std::map<ElementTuple,std::vector<ElementTuple>,Compare<ElementTuple> > LookupTable;

class EnumLookupGenerator : public InstGenerator {
	private:
		const LookupTable&							_table;
		std::vector<const DomElemContainer*>		_invars;
		std::vector<const DomElemContainer*>		_outvars;
		mutable std::vector<const DomainElement*>	_currargs;
		mutable LookupTable::const_iterator			_currpos;
		mutable std::vector<std::vector<const DomainElement*> >::const_iterator	_iter;
	public:
		EnumLookupGenerator(const LookupTable& t, const std::vector<const DomElemContainer*>& in, const std::vector<const DomElemContainer*>& out) : _table(t), _invars(in), _outvars(out), _currargs(in.size()) { }
		bool first()	const;
		bool next()		const;
};


#endif /* ENUMLOOKUPGENERATOR_HPP_ */
