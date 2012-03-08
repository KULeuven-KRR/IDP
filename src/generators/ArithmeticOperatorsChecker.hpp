/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ARITHCH_HPP_
#define ARITHCH_HPP_

#include "common.hpp"
#include "InstGenerator.hpp"
#include "ArithmeticOperatorsGenerator.hpp"// for the arithresult
/**
 * Instance checker for the formula x op y = z
 * where x and y and z are input.
 * This is an InstGenerator, since at some places in the code we want to be able to call next,...
 */
class ArithOpChecker: public InstGenerator {
private:
	const DomElemContainer* _in1;
	const DomElemContainer* _in2;
	const DomElemContainer* _in3;
	const Universe _universe;
	bool alreadyrun;
protected:
	virtual ARITHRESULT doCalculation(double left, double right, double& result) const = 0;

	const DomElemContainer* getIn1() const {
		return _in1;
	}
	const DomElemContainer* getIn2() const {
		return _in2;
	}
	const DomElemContainer* getIn3() const {
		return _in3;
	}

public:
	ArithOpChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
			: _in1(in1), _in2(in2), _in3(in3), alreadyrun(false), _universe(univ) {
	}

	void reset() {
		alreadyrun = false;
	}

	void next() {
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

	bool check() const {
		double result;
		ARITHRESULT status = doCalculation(getValue(_in1), getValue(_in2), result);
		return status == ARITHRESULT::VALID && result == getValue(_in3) && _universe.contains( { _in1->get(), _in2->get(), _in3->get() });
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

// FIXME handle overflows
class DivChecker: public ArithOpChecker {
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
	DivChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
			: ArithOpChecker(in1, in2, in3, univ) {
	}

	DivGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	virtual void put(std::ostream& stream) {
		stream << toString(getIn1()) << "(in)" << " / " << toString(getIn2()) << "(in)" << " = " << toString(getIn3()) << "(in)";
	}
};

class TimesChecker: public ArithOpChecker {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const {
		result = left * right;
		return ARITHRESULT::VALID;
	}
	;
public:
	TimesChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
			: ArithOpChecker(in1, in2, in3, univ) {
	}

	TimesGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	virtual void put(std::ostream& stream) {
		stream << toString(getIn1()) << "(in)" << " * " << toString(getIn2()) << "(in)" << " = " << toString(getIn3()) << "(in)";
	}
};

class MinusChecker: public ArithOpChecker {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const {
		result = left - right;
		return ARITHRESULT::VALID;
	}
	;
public:
	MinusChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
			: ArithOpChecker(in1, in2, in3, univ) {
	}

	MinusGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	virtual void put(std::ostream& stream) {
		stream << toString(getIn1()) << "(in)" << " - " << toString(getIn2()) << "(in)" << " = " << toString(getIn3()) << "(in)";
	}
};

class PlusChecker: public ArithOpChecker {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const {
		result = left + right;
		return ARITHRESULT::VALID;
	}
	;
public:
	PlusChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ)
			: ArithOpChecker(in1, in2, in3, univ) {
	}

	PlusGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	virtual void put(std::ostream& stream) {
		stream << toString(getIn1()) << "(in)" << " + " << toString(getIn2()) << "(in)" << " = " << toString(getIn3()) << "(in)";
	}
};

#endif /* ARITHCH_HPP_ */
