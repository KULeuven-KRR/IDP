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

#include "common.hpp"
#include "DomainElementFactory.hpp"

using namespace std;

/**
 *	Constructor for a domain element factory. The constructor gets two arguments,
 *	specifying the range of integer for which creation of domain elements is optimized.
 *
 * PARAMETERS
 *		- firstfastint:	the lowest 'efficient' integer
 *		- lastfastint:	one past the highest 'efficient' integer
 */
DomainElementFactory::DomainElementFactory(int firstfastint, int lastfastint)
		: _firstfastint(firstfastint), _lastfastint(lastfastint) {
	Assert(firstfastint < lastfastint);
	_fastintelements = vector<DomainElement*>(lastfastint - firstfastint, (DomainElement*) 0);
}

/**
 *	\brief Returns the unique instance of DomainElementFactory
 */
DomainElementFactory* DomainElementFactory::createGlobal() {
	return new DomainElementFactory();
}

/**
 *	\brief Destructor for DomainElementFactory. Deletes all domain elements and compounds it created.
 */
DomainElementFactory::~DomainElementFactory() {
	for (auto it = _fastintelements.cbegin(); it != _fastintelements.cend(); ++it) {
		delete (*it);
	}
	for (auto it = _intelements.cbegin(); it != _intelements.cend(); ++it) {
		delete (it->second);
	}
	for (auto it = _doubleelements.cbegin(); it != _doubleelements.cend(); ++it) {
		delete (it->second);
	}
	for (auto it = _stringelements.cbegin(); it != _stringelements.cend(); ++it) {
		delete (it->second);
	}
	for (auto it = _compoundelements.cbegin(); it != _compoundelements.cend(); ++it) {
		delete (it->second);
	}
	for (auto it = _compounds.cbegin(); it != _compounds.cend(); ++it) {
		for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
			delete (jt->second);
		}
	}
}

/**
 * \brief Returns the unique compound that consists of the given function and arguments.
 *
 * PARAMETERS
 *		- function:	the given function
 *		- args:		the given arguments
 */
const Compound* DomainElementFactory::compound(Function* function, const ElementTuple& args) {
	map<Function*, map<ElementTuple, Compound*> >::const_iterator it = _compounds.find(function);
	if (it != _compounds.cend()) {
		map<ElementTuple, Compound*>::const_iterator jt = it->second.find(args);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}
	Compound* newcompound = new Compound(function, args);
	_compounds[function][args] = newcompound;
	return newcompound;
}

/**
 * \brief Returns the unique domain element of type int that has a given value
 *
 * PARAMETERS
 *		- value: the given value
 */
const DomainElement* DomainElementFactory::create(int value) {
	DomainElement* element = NULL;
	// Check if the value is within the efficient range
	if (value >= _firstfastint && value < _lastfastint) {
		int lookupvalue = value - _firstfastint;
		element = _fastintelements[lookupvalue];
		if (element != NULL) {
			return element;
		} else {
			element = new DomainElement(value);
			_fastintelements[lookupvalue] = element;
		}
	} else { // The value is not within the efficient range
		map<int, DomainElement*>::const_iterator it = _intelements.find(value);
		if (it == _intelements.cend()) {
			element = new DomainElement(value);
			_intelements[value] = element;
		} else {
			element = it->second;
		}
	}
	return element;
}

/**
 * \brief Returns the unique domain element that has a given floating point value
 *
 * PARAMETERS
 *		- value:		the given value
 */
const DomainElement* DomainElementFactory::create(double value, NumType type) {
	if (type == NumType::CERTAINLYINT || isInt(value)) {
		return create(int(value));
	}

	DomainElement* element;
	auto it = _doubleelements.find(value);
	if (it == _doubleelements.cend()) {
		element = new DomainElement(value);
		_doubleelements[value] = element;
	} else {
		element = it->second;
	}
	return element;
}

/*********************
 Shared strings
 *********************/

#include <unordered_map>
typedef std::unordered_map<std::string, std::string*> MSSP;
class StringPointers {
private:
	MSSP _sharedstrings; //!< map a string to its shared pointer
public:
	~StringPointers();
	string* stringpointer(const std::string&); //!< get the shared pointer of a string
};

StringPointers::~StringPointers() {
	for (auto it = _sharedstrings.begin(); it != _sharedstrings.end(); ++it) {
		delete (it->second);
	}
}

string* StringPointers::stringpointer(const string& s) {
	MSSP::iterator it = _sharedstrings.find(s);
	if (it != _sharedstrings.end()) {
		return it->second;
	} else {
		string* sp = new string(s);
		_sharedstrings[s] = sp;
		return sp;
	}
}

StringPointers sharedstrings;

string* StringPointer(const char* str) {
	return sharedstrings.stringpointer(string(str));
}

string* StringPointer(const string& str) {
	return sharedstrings.stringpointer(str);
}

/**
 * \brief Returns the unique domain element that has a given string value
 *
 * PARAMETERS
 *		- value:			the given value
 */
const DomainElement* DomainElementFactory::create(const string& value) {
	DomainElement* element = NULL;
	auto sharedstring = StringPointer(value);
	auto it2 = _stringelements.find(sharedstring);
	if (it2 == _stringelements.cend()) {
		element = new DomainElement(sharedstring);
		_stringelements[sharedstring] = element;
	}else{
		element = (*it2).second;
	}
	return element;
}

/**
 * \brief Returns the unique domain element that has a given compound value
 *
 * PARAMETERS
 *		- value:	the given value
 */
const DomainElement* DomainElementFactory::create(const Compound* value) {
	DomainElement* element;
	map<const Compound*, DomainElement*>::const_iterator it = _compoundelements.find(value);
	if (it == _compoundelements.cend()) {
		element = new DomainElement(value);
		_compoundelements[value] = element;
	} else {
		element = it->second;
	}
	return element;
}

/**
 * \brief Returns the unique domain element that has a given compound value
 *
 * PARAMETERS
 *		- function:	the function of the given compound value
 *		- args:		the arguments of the given compound value
 */
const DomainElement* DomainElementFactory::create(Function* function, const ElementTuple& args) {
	Assert(function != NULL);
	const Compound* value = compound(function, args);
	return create(value);
}
