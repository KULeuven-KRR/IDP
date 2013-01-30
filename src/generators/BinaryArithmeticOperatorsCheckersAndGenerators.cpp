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

#include "structure/MainStructureComponents.hpp"
#include "BinaryArithmeticOperatorsChecker.hpp"
#include "BinaryArithmeticOperatorsGenerator.hpp"

ArithOpChecker::ArithOpChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
		: _in1(in1), _in2(in2), _in3(in3), _universe(univ), alreadyrun(false) {
}

void ArithOpChecker::reset() {
	alreadyrun = false;
}

void ArithOpChecker::next() {
	if (alreadyrun) {
		notifyAtEnd();
		return;
	}

	Assert(_in1->get()->type()==DET_INT || _in1->get()->type()==DET_DOUBLE);
	Assert(_in2->get()->type()==DET_INT || _in2->get()->type()==DET_DOUBLE);
	Assert(_in3->get()->type()==DET_INT || _in3->get()->type()==DET_DOUBLE);

	if (not check()) {
		notifyAtEnd();
		return;
	}
	alreadyrun = true;
}

bool ArithOpChecker::check() const {
	double result;
	ARITHRESULT status = doCalculation(getValue(_in1), getValue(_in2), result);
	return status == ARITHRESULT::VALID && result == getValue(_in3) && _universe.contains( { _in1->get(), _in2->get(), _in3->get() });
}

double ArithOpChecker::getValue(const DomElemContainer* cont) const {
	auto domelem = cont->get();
	Assert(domelem->type()==DET_DOUBLE || domelem->type()==DET_INT);
	if (domelem->type() == DET_DOUBLE) {
		return domelem->value()._double;
	} else {
		return domelem->value()._int;
	}
}

ARITHRESULT DivChecker::doCalculation(double left, double right, double& result) const {
	if (right == 0) { // cannot divide by zero
		return ARITHRESULT::INVALID;
	}
	result = left / right;
	return ARITHRESULT::VALID;
}
DivChecker::DivChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
		: ArithOpChecker(in1, in2, in3, univ) {
}

DivChecker* DivChecker::clone() const {
	return new DivChecker(*this);
}

void DivChecker::put(std::ostream& stream) const{
	stream << print(getIn1()) << "(in)" << " / " << print(getIn2()) << "(in)" << " = " << print(getIn3()) << "(in)";
}

ARITHRESULT TimesChecker::doCalculation(double left, double right, double& result) const {
	result = left * right;
	return ARITHRESULT::VALID;
}
TimesChecker::TimesChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
		: ArithOpChecker(in1, in2, in3, univ) {
}

TimesChecker* TimesChecker::clone() const {
	return new TimesChecker(*this);
}

void TimesChecker::put(std::ostream& stream)const {
	stream << print(getIn1()) << "(in)" << " * " << print(getIn2()) << "(in)" << " = " << print(getIn3()) << "(in)";
}

ARITHRESULT MinusChecker::doCalculation(double left, double right, double& result) const {
	result = left - right;
	return ARITHRESULT::VALID;
}
MinusChecker::MinusChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
		: ArithOpChecker(in1, in2, in3, univ) {
}

MinusChecker* MinusChecker::clone() const {
	return new MinusChecker(*this);
}

void MinusChecker::put(std::ostream& stream)const {
	stream << print(getIn1()) << "(in)" << " - " << print(getIn2()) << "(in)" << " = " << print(getIn3()) << "(in)";
}

ARITHRESULT PlusChecker::doCalculation(double left, double right, double& result) const {
	result = left + right;
	return ARITHRESULT::VALID;
}
PlusChecker::PlusChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
		: ArithOpChecker(in1, in2, in3, univ) {
}

PlusChecker* PlusChecker::clone() const {
	return new PlusChecker(*this);
}

void PlusChecker::put(std::ostream& stream) const{
	stream << print(getIn1()) << "(in)" << " + " << print(getIn2()) << "(in)" << " = " << print(getIn3()) << "(in)";
}

ARITHRESULT ModChecker::doCalculation(double left, double right, double& result) const {
	if (right == 0) { // cannot divide by zero
		return ARITHRESULT::INVALID;
	}
	result = (int) left % (int) right;
	if (result < 0) {
		result += right;
	}
	return ARITHRESULT::VALID;
}
ModChecker::ModChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
		: ArithOpChecker(in1, in2, in3, univ) {
}

ModChecker* ModChecker::clone() const {
	return new ModChecker(*this);
}

void ModChecker::put(std::ostream& stream)const {
	stream << print(getIn1()) << "(in)" << " % " << print(getIn2()) << "(in)" << " = " << print(getIn3()) << "(in)";
}

DomainElementType ArithOpGenerator::getOutType() {
	auto leftt = getIn1()->get()->type();
	auto rightt = getIn2()->get()->type();
	Assert(leftt == DomainElementType::DET_INT || leftt == DomainElementType::DET_DOUBLE);
	Assert(rightt == DomainElementType::DET_INT || rightt == DomainElementType::DET_DOUBLE);
	if (leftt == rightt) {
		return leftt;
	}
	return DomainElementType::DET_DOUBLE;
}

ArithOpGenerator::ArithOpGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
		: _in1(in1), _in2(in2), _out(out), _outdom(dom), _requestedType(requestedType), alreadyrun(false) {
}

void ArithOpGenerator::reset() {
	alreadyrun = false;
}

void ArithOpGenerator::internalSetVarsAgain(){ // NOTE: duplication with next()
	double result;
	ARITHRESULT status = doCalculation(getValue(_in1), getValue(_in2), result);
	if (getOutType() == DET_INT && not isInt(result)) { // NOTE: checks whether no overflow occurred
		status = ARITHRESULT::INVALID;
	}
	if (status != ARITHRESULT::VALID) {
		*_out = (const DomainElement*) NULL;
	}else{
		*_out = createDomElem(getOutType() == DET_INT ? int(result) : result, _requestedType);
		if (not _outdom->contains(_out->get())) {
			*_out = (const DomainElement*) NULL;
		}
	}
}

void ArithOpGenerator::next() {
	if (alreadyrun) {
		notifyAtEnd();
		return;
	}
	double result;
	// FIXME code duplication with calculations with overflow checking in NumericOperations.hpp (add outputtype to those functions)
	ARITHRESULT status = doCalculation(getValue(_in1), getValue(_in2), result);
	if (getOutType() == DET_INT && not isInt(result)) { // NOTE: checks whether no overflow occurred
		status = ARITHRESULT::INVALID;
	}
	if (status != ARITHRESULT::VALID) {
		notifyAtEnd();
		*_out = (const DomainElement*) NULL;
		return;
	}
	*_out = createDomElem(getOutType() == DET_INT ? int(result) : result, _requestedType);
	if (not _outdom->contains(_out->get())) {
		notifyAtEnd();
		*_out = (const DomainElement*) NULL;
	}
	alreadyrun = true;
}

bool ArithOpGenerator::check() const {
	double result;
	ARITHRESULT status = doCalculation(getValue(_in1), getValue(_in2), result);
	return status == ARITHRESULT::VALID && result == getValue(_out);
}

double ArithOpGenerator::getValue(const DomElemContainer* cont) const {
	auto domelem = cont->get();
	Assert(domelem->type()==DET_DOUBLE || domelem->type()==DET_INT);
	if (domelem->type() == DET_DOUBLE) {
		return domelem->value()._double;
	} else {
		return domelem->value()._int;
	}
}

ARITHRESULT DivGenerator::doCalculation(double left, double right, double& result) const {
	if (right == 0) { // cannot divide by zero
		return ARITHRESULT::INVALID;
	}
	result = left / right;
	return ARITHRESULT::VALID;
}

DivGenerator::DivGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
		: ArithOpGenerator(in1, in2, out, requestedType, dom) {
}

DivGenerator* DivGenerator::clone() const {
	return new DivGenerator(*this);
}

void DivGenerator::put(std::ostream& stream) const {
	stream << print(getIn1()) << "(in)" << " / " << print(getIn2()) << "(in)" << " = " << print(getOutDom()) << "[" << print(getOutDom()) << "]"
			<< "(out)";
}
DomainElementType DivGenerator::getOutType() {
#ifdef DEBUG
	auto leftt = getIn1()->get()->type();
	auto rightt = getIn2()->get()->type();
	Assert(leftt == DomainElementType::DET_INT || leftt == DomainElementType::DET_DOUBLE);
	Assert(rightt == DomainElementType::DET_INT || rightt == DomainElementType::DET_DOUBLE);
#endif
	return DomainElementType::DET_DOUBLE;
}

ARITHRESULT TimesGenerator::doCalculation(double left, double right, double& result) const {
	result = left * right;
	return ARITHRESULT::VALID;
}

TimesGenerator::TimesGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
		: ArithOpGenerator(in1, in2, out, requestedType, dom) {
}

TimesGenerator* TimesGenerator::clone() const {
	return new TimesGenerator(*this);
}

void TimesGenerator::put(std::ostream& stream) const {
	stream << print(getIn1()) << "(in)" << " * " << print(getIn2()) << "(in)" << " = " << print(getOutDom()) << "[" << print(getOutDom()) << "]"
			<< "(out)";
}

ARITHRESULT MinusGenerator::doCalculation(double left, double right, double& result) const {
	result = left - right;
	return ARITHRESULT::VALID;
}

MinusGenerator::MinusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
		: ArithOpGenerator(in1, in2, out, requestedType, dom) {
}

MinusGenerator* MinusGenerator::clone() const {
	return new MinusGenerator(*this);
}

void MinusGenerator::put(std::ostream& stream) const {
	stream << print(getIn1()) << "(in)" << " - " << print(getIn2()) << "(in)" << " = " << print(getOutDom()) << "[" << print(getOutDom()) << "]"
			<< "(out)";
}

ARITHRESULT PlusGenerator::doCalculation(double left, double right, double& result) const {
	result = left + right;
	return ARITHRESULT::VALID;
}

PlusGenerator::PlusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
		: ArithOpGenerator(in1, in2, out, requestedType, dom) {
}

PlusGenerator* PlusGenerator::clone() const {
	return new PlusGenerator(*this);
}

void PlusGenerator::put(std::ostream& stream) const {
	stream << print(getIn1()) << "(in)" << " + " << print(getIn2()) << "(in)" << " = " << print(getOutDom()) << "[" << print(getOutDom()) << "]"
			<< "(out)";
}

