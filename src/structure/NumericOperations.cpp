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

#include "NumericOperations.hpp"
#include "structure/StructureComponents.hpp"

using namespace std;

template<>
bool isNum<int>(const double& value) {
	return isInt(value);
}

template<>
bool isNum<double>(const double&) {
	return true;
}

std::string getDomElemOpErrorMessage(const std::string& operation) {
	stringstream ss;
	ss << "Taking the " << operation << " of domain elements of nonnumerical types";
	return ss.str();
}

template<typename IntOpType, typename DoubleOpType>
const DomainElement* domElemOp(const DomainElement* d1, const DomainElement* d2, const DomainElement* (*opint)(IntOpType, IntOpType),
		const DomainElement* (*opdouble)(DoubleOpType, DoubleOpType), const std::string& operation) {
	if(d1==NULL || d2==NULL){
		return NULL;
	}
	const DomainElement* dnew = NULL;
	switch (d1->type()) {
	case DET_INT:
		switch (d2->type()) {
		case DET_INT:
			dnew = opint(d1->value()._int, d2->value()._int);
			break;
		case DET_DOUBLE:
			dnew = opdouble(double(d1->value()._int), d2->value()._double);
			break;
		case DET_STRING:
		case DET_COMPOUND:
			throw notyetimplemented(getDomElemOpErrorMessage(operation));
		}
		break;
	case DET_DOUBLE:
		switch (d2->type()) {
		case DET_INT:
			dnew = opdouble(d1->value()._double, double(d2->value()._int));
			break;
		case DET_DOUBLE:
			dnew = opdouble(d1->value()._double, d2->value()._double);
			break;
		case DET_STRING:
		case DET_COMPOUND:
			throw notyetimplemented(getDomElemOpErrorMessage(operation));
		}
		break;
	case DET_STRING:
	case DET_COMPOUND:
		throw notyetimplemented(getDomElemOpErrorMessage(operation));
	}
	return dnew;
}

const DomainElement* domElemSum(const DomainElement* d1, const DomainElement* d2) {
	return domElemOp(d1, d2, &sum<int>, &sum<double>, "sum");
}

const DomainElement* domElemProd(const DomainElement* d1, const DomainElement* d2) {
	return domElemOp(d1, d2, &product<int>, &product<double>, "product");
}

const DomainElement* domElemPow(const DomainElement* d1, const DomainElement* d2) {
	return domElemOp<double, double>(d1, d2, &pow<double>, &pow<double>, "power");
}

const DomainElement* domElemAbs(const DomainElement* d) {
	const DomainElement* dnew = NULL;
	switch (d->type()) {
	case DET_INT:
		dnew = createDomElem(std::abs(d->value()._int));
		break;
	case DET_DOUBLE:
		dnew = createDomElem(std::abs(d->value()._double));
		break;
	case DET_STRING:
	case DET_COMPOUND:
		throw notyetimplemented("Absolute value of domain elements of nonnumerical types");
	}
	return dnew;
}

const DomainElement* domElemUmin(const DomainElement* d) {
	const DomainElement* dnew = NULL;
	switch (d->type()) {
	case DET_INT:
		dnew = createDomElem(-d->value()._int);
		break;
	case DET_DOUBLE:
		dnew = createDomElem(-d->value()._double);
		break;
	case DET_STRING:
	case DET_COMPOUND:
		throw notyetimplemented("Negative value of domain elements of nonnumerical types");
	}
	return dnew;
}

bool isPositive(const DomainElement* d) {
	return (*d >= *createDomElem(0));
}
bool isNegative(const DomainElement* d) {
	return not isPositive(d);
}
