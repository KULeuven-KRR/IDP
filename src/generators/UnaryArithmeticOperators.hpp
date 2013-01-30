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

#ifndef ABSVALCH_HPP
#define ABSVALCH_HPP

#include "InstGenerator.hpp"
#include "structure/Universe.hpp"
#include "commontypes.hpp"

class DomElemContainer;
class SortTable;

enum class State {
	RESET, FIRSTDONE, SECONDDONE
};
/**
 * Check if the input-output pattern matches absvalue --> Check if |in| = out
 */
class UnaryArithmeticOperatorsChecker: public InstGenerator {
protected:
	const DomElemContainer* _in;
	const DomElemContainer* _out;
	Universe _universe;
	bool _reset;
	virtual bool checkOperation()=0;
public:
	UnaryArithmeticOperatorsChecker(const DomElemContainer* in, const DomElemContainer* out, Universe universe);
	void reset();
	void next();
};

class UnaryArithmeticOperatorsGenerator: public UnaryArithmeticOperatorsChecker {
public:
	UnaryArithmeticOperatorsGenerator(const DomElemContainer* in, const DomElemContainer* out, Universe universe);
protected:
	virtual void doOperation() = 0;
	virtual bool checkOperation();
public:
	void next();
};

class UnaryMinusGenerator: public UnaryArithmeticOperatorsGenerator {
public:
	UnaryMinusGenerator(const DomElemContainer* in, const DomElemContainer* out, Universe universe);
	UnaryMinusGenerator* clone() const;
protected:
	void doOperation();
};

class AbsValueChecker: public UnaryArithmeticOperatorsChecker {
public:
	AbsValueChecker(const DomElemContainer* in, const DomElemContainer* out, Universe universe);
	AbsValueChecker* clone() const;
	void internalSetVarsAgain();
protected:
	virtual bool checkOperation();
};

/**
 * Given the output of the abs function, generate all values that could have been input.
 */
class InverseAbsValueGenerator: public InstGenerator {
private:
	const DomElemContainer* _in;
	const DomElemContainer* _out;
	SortTable* _outdom;
	State _state;
	NumType _outputShouldBeInt;
public:
	InverseAbsValueGenerator(const DomElemContainer* in, const DomElemContainer* out, SortTable* dom, NumType outputShouldBeInt);
	InverseAbsValueGenerator* clone() const;
	void internalSetVarsAgain();
	void reset();
	void next();

private:
	void setValue(bool negate);
};
#endif /* ABSVALCH_HPP */
