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

#ifndef DOMELEM_HPP_
#define DOMELEM_HPP_

#include <vector>
#include <string>

class DomElemContainer;
class DomainElementFactory;
class DomainElement;
class Compound;

/**
 *	The different types of domain elements
 *	- DET_INT: integers
 *	- DET_DOUBLE: floating point numbers
 *	- DET_STRING: strings
 *	- DET_COMPOUND: a function applied to domain elements
 */
enum DomainElementType {
	DET_INT = 0, DET_DOUBLE = 1, DET_STRING = 3, DET_COMPOUND = 4
};

/**
 *	A value for a single domain element.
 */
union DomainElementValue {
	int _int; //!< Value if the domain element is an integer
	double _double; //!< Value if the domain element is a floating point number
	const std::string* _string; //!< Value if the domain element is a string
	const Compound* _compound; //!< Value if the domain element is a function applied to domain elements

	DomainElementValue(int i)
			: _int(i) {
	}
	DomainElementValue(double i)
			: _double(i) {
	}
	DomainElementValue(const std::string* i)
			: _string(i) {
	}
	DomainElementValue(const Compound* i)
			: _compound(i) {
	}
};

/**
 *	A domain element
 */
class DomainElement {
private:
	const DomainElementType _type; //!< The type of the domain element
	const DomainElementValue _value; //!< The value of the domain element

	DomainElement();
	DomainElement(int value);
	DomainElement(double value);
	DomainElement(const std::string* value);
	DomainElement(const Compound* value);

public:
	~DomainElement() {
	}

	inline const DomainElementType& type() const {
		return _type;
	}
	inline const DomainElementValue& value() const {
		return _value;
	}

	std::ostream& put(std::ostream&) const;

	friend class DomainElementFactory;
	friend class DomElemContainer;
};

typedef std::vector<const DomainElement*> ElementTuple;
typedef std::vector<ElementTuple> ElementTable;

#endif /* DOMELEM_HPP_ */
