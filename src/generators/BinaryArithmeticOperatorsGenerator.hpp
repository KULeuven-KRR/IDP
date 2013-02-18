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

#ifndef PLUSGENERATOR_HPP_
#define PLUSGENERATOR_HPP_

class DomelemContainer;
class SortTable;
#include "commontypes.hpp"
#include "structure/DomainElement.hpp"
#include "InstGenerator.hpp"


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
	virtual DomainElementType getOutType();

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
	ArithOpGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom);

	void reset();
	void next();
	bool check() const;
	void internalSetVarsAgain();
private:
	double getValue(const DomElemContainer* cont) const;
};

class DivGenerator: public ArithOpGenerator {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	DivGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom);
	DivGenerator* clone() const;
	virtual void put(std::ostream& stream) const;
	virtual DomainElementType getOutType();
};

class TimesGenerator: public ArithOpGenerator {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	TimesGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom);
	TimesGenerator* clone() const;
	virtual void put(std::ostream& stream) const;
};

class MinusGenerator: public ArithOpGenerator {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	MinusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom);
	MinusGenerator* clone() const;
	virtual void put(std::ostream& stream) const;
};

class PlusGenerator: public ArithOpGenerator {
protected:
	ARITHRESULT doCalculation(double left, double right, double& result) const;
public:
	PlusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, NumType requestedType, SortTable* dom);
	PlusGenerator* clone() const;
	virtual void put(std::ostream& stream) const;
};

#endif /* PLUSGENERATOR_HPP_ */
