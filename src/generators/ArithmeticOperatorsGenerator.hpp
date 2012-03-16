/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef PLUSGENERATOR_HPP_
#define PLUSGENERATOR_HPP_

#include "common.hpp"
#include "InstGenerator.hpp"
#include "structure/structure.hpp"

enum class ARITHRESULT {
	VALID, INVALID
};

/**
 * Instance generator for the formula x op y = z
 * where x and y are input and numeric and z is the output.
 */
class ArithOpGenerator: public InstGenerator {
private:
	const DomElemContainer* _in1;
	const DomElemContainer* _in2;
	const DomElemContainer* _out;
	SortTable* _outdom;
	NumType _requestedType;
	bool alreadyrun;
protected:
	virtual ARITHRESULT doCalculation(double left, double right, double& result) const = 0;
	virtual DomainElementType getOutType() {
		auto leftt = getIn1()->get()->type();
		auto rightt = getIn2()->get()->type();
		Assert(leftt == DomainElementType::DET_INT || leftt == DomainElementType::DET_DOUBLE);
		Assert(rightt == DomainElementType::DET_INT || rightt == DomainElementType::DET_DOUBLE);
		if (leftt == rightt) {
			return leftt;
		}
		return DomainElementType::DET_DOUBLE;
	}

	const DomElemContainer* getIn1() const {
		return _in1;
	}
	const DomElemContainer* getIn2() const {
		return _in2;
	}
	const DomElemContainer* getOut1() const {
		return _out;
	}
	SortTable* getOutDom() const {
		return _outdom;
	}

public:
	ArithOpGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
			: _in1(in1), _in2(in2), _out(out), _outdom(dom), _requestedType(requestedType), alreadyrun(false) {
	}

	void reset() {
		alreadyrun = false;
	}

	void next() {
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
			*_out = (const DomainElement*)NULL;
			return;
		}
		*_out = createDomElem(getOutType() == DET_INT ? int(result) : result, _requestedType);
		if (not _outdom->contains(_out->get())) {
			notifyAtEnd();
			*_out = (const DomainElement*)NULL;
		}
		alreadyrun = true;
	}

	bool check() const {
		double result;
		ARITHRESULT status = doCalculation(getValue(_in1), getValue(_in2), result);
		return status == ARITHRESULT::VALID && result == getValue(_out);
	}

private:
	double getValue(const DomElemContainer* cont) const {
		auto domelem = cont->get();
		Assert(domelem->type()==DET_DOUBLE || domelem->type()==DET_INT);
		if (domelem->type() == DET_DOUBLE) {
			return domelem->value()._double;
		} else {
			return domelem->value()._int;
		}
	}

};

class DivGenerator: public ArithOpGenerator {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const {
		if (right == 0) { // cannot divide by zero
			return ARITHRESULT::INVALID;
		}
		result = left / right;
		return ARITHRESULT::VALID;
	}
	;
public:
	DivGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
			: ArithOpGenerator(in1, in2, out, requestedType, dom) {
	}

	DivGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	virtual void put(std::ostream& stream) {
		stream << toString(getIn1()) << "(in)" << " / " << toString(getIn2()) << "(in)" << " = " << toString(getOutDom()) << "[" << toString(getOutDom()) << "]"
				<< "(out)";
	}
	virtual DomainElementType getOutType() {
#ifdef DEBUG
		auto leftt = getIn1()->get()->type();
		auto rightt = getIn2()->get()->type();
		Assert(leftt == DomainElementType::DET_INT || leftt == DomainElementType::DET_DOUBLE);
		Assert(rightt == DomainElementType::DET_INT || rightt == DomainElementType::DET_DOUBLE);
#endif
		return DomainElementType::DET_DOUBLE;
	}
};

class TimesGenerator: public ArithOpGenerator {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const {
		result = left * right;
		return ARITHRESULT::VALID;
	}
	;
public:
	TimesGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
			: ArithOpGenerator(in1, in2, out, requestedType, dom) {
	}

	TimesGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	virtual void put(std::ostream& stream) {
		stream << toString(getIn1()) << "(in)" << " * " << toString(getIn2()) << "(in)" << " = " << toString(getOutDom()) << "[" << toString(getOutDom()) << "]"
				<< "(out)";
	}
};

class MinusGenerator: public ArithOpGenerator {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const {
		result = left - right;
		return ARITHRESULT::VALID;
	}
	;
public:
	MinusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
			: ArithOpGenerator(in1, in2, out, requestedType, dom) {
	}

	MinusGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	virtual void put(std::ostream& stream) {
		stream << toString(getIn1()) << "(in)" << " - " << toString(getIn2()) << "(in)" << " = " << toString(getOutDom()) << "[" << toString(getOutDom()) << "]"
				<< "(out)";
	}
};

class PlusGenerator: public ArithOpGenerator {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const {
		result = left + right;
		return ARITHRESULT::VALID;
	}
	;
public:
	PlusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom)
			: ArithOpGenerator(in1, in2, out, requestedType, dom) {
	}

	PlusGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	virtual void put(std::ostream& stream) {
		stream << toString(getIn1()) << "(in)" << " + " << toString(getIn2()) << "(in)" << " = " << toString(getOutDom()) << "[" << toString(getOutDom()) << "]"
				<< "(out)";
	}
};

#endif /* PLUSGENERATOR_HPP_ */
