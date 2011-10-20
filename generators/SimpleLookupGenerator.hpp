/************************************
	SimpleLookupGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SIMPLELOOKUPGENERATOR_HPP_
#define SIMPLELOOKUPGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class SimpleLookupGenerator : public InstGenerator {
	private:
		const PredTable*						_table;
		std::vector<const DomElemContainer*>			_invars;
		Universe								_universe;
		mutable std::vector<const DomainElement*>	_currargs;
	public:
		SimpleLookupGenerator(const PredTable* t, const std::vector<const DomElemContainer*> in, const Universe& univ) :
			_table(t), _invars(in), _universe(univ), _currargs(in.size()) { }
		bool first()	const;
		bool next()		const { return false;	}
};


#endif /* SIMPLELOOKUPGENERATOR_HPP_ */
