/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef ENUMLOOKUPGENERATOR_HPP_
#define ENUMLOOKUPGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "structure/HashElementTuple.hpp"

typedef std::unordered_map<ElementTuple, std::vector<ElementTuple>, HashTuple> LookupTable;

/**
 * Given a map from tuples to a list of tuples, with given input variables and output variables, go over the list of tuples of the corresponding input tuple.
 */
class EnumLookupGenerator: public InstGenerator {
private:
	std::shared_ptr<const LookupTable> _table;
	LookupTable::const_iterator _currpos;
	std::vector<ElementTuple>::const_iterator _iter;
	std::vector<const DomElemContainer*> _invars, _outvars;
	bool _reset;
	mutable ElementTuple _currargs;
public:
	EnumLookupGenerator(std::shared_ptr<const LookupTable> t, const std::vector<const DomElemContainer*>& in, const std::vector<const DomElemContainer*>& out);

	EnumLookupGenerator* clone() const;
	void reset();
	// Increment is done AFTER returning a tuple!
	void next();
	void internalSetVarsAgain();
	virtual void put(std::ostream& stream) const;
};

#endif /* ENUMLOOKUPGENERATOR_HPP_ */
