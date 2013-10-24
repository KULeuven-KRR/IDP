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

#ifndef ARITHRESULTV
#define ARITHRESULTV
enum class ARITHRESULT {
	VALID, INVALID
};
#endif //ARITHRESULTV
#pragma once

#include "InstGenerator.hpp"
#include "structure/Universe.hpp"

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
	Universe _universe;
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
	ArithOpChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ);
	void reset();
	void next();
	bool check() const;
	void internalSetVarsAgain(){} // No-op for any checker
private:
	double getValue(const DomElemContainer* cont) const;

};

class DivChecker: public ArithOpChecker {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	DivChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ);
	DivChecker* clone() const;
	virtual void put(std::ostream& stream)const;
};

class TimesChecker: public ArithOpChecker {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	TimesChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ);
	TimesChecker* clone() const;
	virtual void put(std::ostream& stream)const;
};

class MinusChecker: public ArithOpChecker {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	MinusChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ);
	MinusChecker* clone() const;
	virtual void put(std::ostream& stream)const;
};

class PlusChecker: public ArithOpChecker {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	PlusChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ);
	PlusChecker* clone() const;
	virtual void put(std::ostream& stream)const;
};

class ModChecker: public ArithOpChecker {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	ModChecker(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* in3, const Universe univ);
	ModChecker* clone() const;
	virtual void put(std::ostream& stream)const;
};
