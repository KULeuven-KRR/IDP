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

#ifndef DOMELEMNUMOPS_HPP_
#define DOMELEMNUMOPS_HPP_

#include <cmath>

#include "DomainElement.hpp"
#include "DomainElementFactory.hpp"

template<typename CheckType, typename RealType>
bool isNum(const RealType& value);

template<typename RequestedType>
const DomainElement* sum(double l, double r) {
	auto result = l + r;
	if (not isNum<RequestedType>(result)) {
		return NULL;
	}
	return createDomElem(result);
}

template<typename RequestedType>
const DomainElement* difference(double l, double r) {
	auto result = l - r;
	if (not isNum<RequestedType>(result)) {
		return NULL;
	}
	return createDomElem(result);
}

template<typename RequestedType>
const DomainElement* product(double l, double r) {
	auto result = l * r;
	if (not isNum<RequestedType>(result)) {
		return NULL;
	}
	return createDomElem(result);
}

template<typename RequestedType>
const DomainElement* division(double l, double r) {
	if (r == 0) {
		return NULL;
	}
	auto result = l / r;
	if (not isNum<RequestedType>(result)) {
		return NULL;
	}
	return createDomElem(result);
}

template<typename RequestedType>
const DomainElement* pow(double l, double r) {
	auto result = std::pow(l, r);
	if (not isNum<RequestedType>(result)) {
		return NULL;
	}
	return createDomElem(result);
}

const DomainElement* domElemSum(const DomainElement* d1, const DomainElement* d2);
const DomainElement* domElemProd(const DomainElement* d1, const DomainElement* d2);
const DomainElement* domElemPow(const DomainElement* d1, const DomainElement* d2);
const DomainElement* domElemAbs(const DomainElement* d);
const DomainElement* domElemUmin(const DomainElement* d);

bool isPositive(const DomainElement* d); //<! returns true if d >= 0
bool isNegative(const DomainElement* d); //<! returns true if d < 0



#endif /* DOMELEMNUMOPS_HPP_ */
