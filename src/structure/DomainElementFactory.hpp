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

#ifndef DOMAINELEMENTFACTORY_HPP_
#define DOMAINELEMENTFACTORY_HPP_

#include "common.hpp"

#include "DomainElement.hpp"
#include "Compound.hpp"
#include "GlobalData.hpp"

class Function;

/**
 *	Class to create domain elements. This class is a singleton class that ensures all domain elements
 *	with the same value are stored at the same address in memory. As a result, two domain elements are
 *	equal iff they have the same address. It also ensures that all Compounds with the same function
 *	and arguments are stored at the same address.
 *
 *	Obtaining the address of a domain element with a given value and type should take logaritmic time
 *	in the number of created domain elements of that type. For a specified integer range, obtaining
 *	the address is optimized to constant time.
 */
class DomainElementFactory {
private:
	std::map<Function*, std::map<ElementTuple, Compound*> > _compounds;
	//!< Maps a function and tuple of elements to the corresponding compound.

	int _firstfastint; //!< The first integer in the optimized range
	int _lastfastint; //!< One past the last integer in the optimized range
	std::vector<DomainElement*> _fastintelements; //!< Stores pointers to integers in the optimized range.
												  //!< The domain element with value n is stored at
												  //!< _fastintelements[n+_firstfastint]

	std::map<int, DomainElement*> _intelements;
	//!< Maps an integer outside of the optimized range to its corresponding doman element address.
	std::map<double, DomainElement*> _doubleelements;
	//!< Maps a floating point number to its corresponding domain element address.
	std::unordered_map<const std::string*, DomainElement*> _stringelements;
	//!< Maps a string pointer to its corresponding domain element address.
	std::map<const Compound*, DomainElement*> _compoundelements;
	//!< Maps a compound pointer to its corresponding domain element address.

	DomainElementFactory(int firstfastint = 0, int lastfastint = 10001);

public:
	~DomainElementFactory();

	static DomainElementFactory* createGlobal();

	const DomainElement* create(int value);
	const DomainElement* create(double value, NumType type = NumType::POSSIBLYINT);
	const DomainElement* create(const std::string& value);
	const DomainElement* create(const Compound* value);
	const DomainElement* create(Function*, const ElementTuple&);

	const Compound* compound(Function*, const ElementTuple&);
};

template<typename Value>
const DomainElement* createDomElem(const Value& value) {
	return GlobalData::getGlobalDomElemFactory()->create(value);
}

template<typename Value, typename Type>
const DomainElement* createDomElem(const Value& value, const Type& t) {
	return GlobalData::getGlobalDomElemFactory()->create(value, t);
}

template<typename Function, typename Value>
const Compound* createCompound(Function* f, const Value& tuple) {
	return GlobalData::getGlobalDomElemFactory()->compound(f, tuple);
}

#endif /* DOMAINELEMENTFACTORY_HPP_ */
