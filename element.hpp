/************************************
	element.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ELEMENT_HPP
#define ELEMENT_HPP

#include <string>
#include <vector>

#include "commontypes.hpp"

class Function;

/**********************
	Domain elements
**********************/

/** \file element.hpp
 * The four different types of domain elements 
 *		ELINT: an integer 
 *		ELDOUBLE: a floating point number 
 *		ELSTRING: a string (characters are strings of length 1)
 *		ELCOMPOUND: a constructor function applied to a number of domain elements
 *
 *		These types form a hierarchy: ints are also doubles, doubles are also strings, strings are also compounds.
 *
 */

// A single domain element
union Element {
	int				_int;
	double			_double;
	std::string*	_string;
	compound*		_compound;
};

// A pair of a domain element and its type
struct TypedElement {
	Element		_element;
	ElementType	_type;

	TypedElement(Element e, ElementType t) : _element(e), _type(t) { }
	TypedElement(int n) : _type(ELINT) { _element._int = n;	}
	TypedElement(double d) : _type(ELDOUBLE) { _element._double = d;	}
	TypedElement(std::string* s) : _type(ELSTRING) { _element._string = s;	}
	TypedElement(compound* c) : _type(ELCOMPOUND) { _element._compound = c;	}
	TypedElement() : _type(ELINT) { }
};

/**
 *		Compound domain elements
 *		
 *		NOTE: sometimes, an int/double/string is represented as a compound domain element
 *		in that case, _function==0 and the only element in _args is the int/double/string
 *
 */
struct compound {
	Function*					_function;
	std::vector<TypedElement>	_args;

	compound(Function* f, const std::vector<TypedElement>& a) : _function(f), _args(a) { }
	std::string to_string() const;
};

// Class that implements the relation 'less-than-or-equal' on tuples of domain elements with the same types
class ElementWeakOrdering {
	private:
		std::vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementWeakOrdering() { }
		ElementWeakOrdering(const std::vector<ElementType>& t) : _types(t) { }
		bool operator()(const std::vector<Element>&,const std::vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}
};

// Class that implements the relation 'equal' on tuples of domain elements with the same types
class ElementEquality {
	private:
		std::vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementEquality() { }
		ElementEquality(const std::vector<ElementType>& t) : _types(t) { }
		bool operator()(const std::vector<Element>&,const std::vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}
};

// Useful functions on domain elements
namespace ElementUtil {
	// Return the least precise elementtype of the two given types
	ElementType	resolve(ElementType,ElementType);

	// Return the most precise elementtype
	ElementType leasttype(); 

	// Return the most precise type of the given element
	ElementType reduce(Element,ElementType);
	ElementType reduce(TypedElement);

	// Convert a domain element to a string
	std::string	ElementToString(Element,ElementType);
	std::string	ElementToString(TypedElement);

	// Return the unique non-existing domain element of a given type. 
	//Non-existing domain elements are used as return value when a partial function is applied on elements outside its domain
	Element	nonexist(ElementType);				

	// Checks if the element exists, i.e., if it is not equal to the unique non-existing element
	bool	exists(Element,ElementType);	
	bool	exists(TypedElement);					

	// Convert an element from one type to another
	// If this is impossible, the non-existing element of the requested type is returned
	Element						convert(TypedElement,ElementType newtype);		
	Element						convert(Element,ElementType oldtype,ElementType newtype);
	std::vector<TypedElement>	convert(const std::vector<domelement>&);

	// Compare elements
	bool	equal(Element e1, ElementType t1, Element e2, ElementType t2);				// equality
	bool	strlessthan(Element e1, ElementType t1, Element e2, ElementType t2);		// strictly less than
	bool	lessthanorequal(Element e1, ElementType t1, Element e2, ElementType t2);	// less than or equal

	// Compare tuples
	bool	equal(const std::vector<TypedElement>&, const std::vector<Element>&, const std::vector<ElementType>&);
}

bool operator==(TypedElement e1, TypedElement e2);
bool operator!=(TypedElement e1, TypedElement e2);
bool operator<=(TypedElement e1, TypedElement e2);
bool operator<(TypedElement e1, TypedElement e2);

#endif
