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

#include "UnaryArithmeticOperators.hpp"
#include "structure/MainStructureComponents.hpp"

UnaryArithmeticOperatorsChecker::UnaryArithmeticOperatorsChecker(const DomElemContainer* in, const DomElemContainer* out, Universe universe)
		: _in(in), _out(out), _universe(universe) {
}

void UnaryArithmeticOperatorsChecker::reset() {
	_reset = true;
}

void UnaryArithmeticOperatorsChecker::next() {
	if (not _reset) {
		notifyAtEnd();
	} else {
		_reset = false;
		Assert(_in->get()->type()==DET_INT || _in->get()->type()==DET_DOUBLE);
		Assert(_out->get()->type()==DET_INT || _out->get()->type()==DET_DOUBLE);
		if (not checkOperation()) {
			notifyAtEnd();
			return;
		}
	}
}

UnaryArithmeticOperatorsGenerator::UnaryArithmeticOperatorsGenerator(const DomElemContainer* in, const DomElemContainer* out, Universe universe)
		: UnaryArithmeticOperatorsChecker(in, out, universe) {
}

bool UnaryArithmeticOperatorsGenerator::checkOperation() {
	Assert(false);
	//NOt needed for generators
	return true;
}
void UnaryArithmeticOperatorsGenerator::next() {
	if (not _reset) {
		notifyAtEnd();
		return;
	}
	_reset = false;
	Assert(_in->get()->type()==DET_INT || _in->get()->type()==DET_DOUBLE);
	Assert(_out->get()->type()==DET_INT || _out->get()->type()==DET_DOUBLE);
	doOperation();
	ElementTuple tuple = { _in->get(), _out->get() };
	if (not _universe.contains(tuple)) {
		notifyAtEnd();

	}
}

UnaryMinusGenerator::UnaryMinusGenerator(const DomElemContainer* in, const DomElemContainer* out, Universe universe)
		: UnaryArithmeticOperatorsGenerator(in, out, universe) {
}
void UnaryMinusGenerator::doOperation() {
	auto var = _in->get();
	if (var->type() == DET_INT) {
		int newvalue = -var->value()._int;
		*_out = createDomElem(newvalue);
	} else {
		Assert(var->type() == DET_DOUBLE);
		double newvalue = -1 * var->value()._double;
		*_out = createDomElem(newvalue);
	}
}

UnaryMinusGenerator* UnaryMinusGenerator::clone() const {
	return new UnaryMinusGenerator(*this);
}

AbsValueChecker::AbsValueChecker(const DomElemContainer* in, const DomElemContainer* out, Universe universe)
		: UnaryArithmeticOperatorsChecker(in, out, universe) {
}

bool AbsValueChecker::checkOperation() {
	double inval;
	double outval;
	if (_in->get()->type() == DET_INT) {
		inval = _in->get()->value()._int;
	} else {
		Assert(_in->get()->type() == DET_DOUBLE);
		inval = _in->get()->value()._double;
	}
	if (_out->get()->type() == DET_INT) {
		outval = _out->get()->value()._int;
	} else {
		Assert(_out->get()->type() == DET_DOUBLE);
		outval = _out->get()->value()._double;
	}
	return (outval >= 0 && (inval == outval || inval == -outval)) && _universe.contains( { _in->get(), _out->get() });
}

AbsValueChecker* AbsValueChecker::clone() const {
	return new AbsValueChecker(*this);
}

void AbsValueChecker::internalSetVarsAgain() {
}

InverseAbsValueGenerator::InverseAbsValueGenerator(const DomElemContainer* in, const DomElemContainer* out, SortTable* dom, NumType outputShouldBeInt)
		: _in(in), _out(out), _outdom(dom), _state(State::RESET), _outputShouldBeInt(outputShouldBeInt) {
}

InverseAbsValueGenerator* InverseAbsValueGenerator::clone() const {
	return new InverseAbsValueGenerator(*this);
}

void InverseAbsValueGenerator::internalSetVarsAgain() {
	if (not isAtEnd()) {
		setValue(_state!=State::FIRSTDONE);
	}
}

void InverseAbsValueGenerator::reset() {
	_state = State::RESET;
}

void InverseAbsValueGenerator::next() {
	bool nowatend = false;
	switch (_state) {
	case State::RESET:
		setValue(true);
		_state = State::FIRSTDONE;
		break;
	case State::FIRSTDONE:
		setValue(false);
		_state = State::SECONDDONE;
		break;
	case State::SECONDDONE:
		notifyAtEnd();
		nowatend = true;
		break;
	}
	if (not nowatend && not _outdom->contains(_out->get())) {
		next();
	}
}

void InverseAbsValueGenerator::setValue(bool negate) {
	auto var = _in->get();
	if (var->type() == DET_INT) {
		int newvalue = var->value()._int;
		if (negate) {
			newvalue = -newvalue;
		}
		*_out = createDomElem(newvalue, _outputShouldBeInt);
	} else {
		Assert(var->type() == DET_DOUBLE);
		double newvalue = var->value()._double;
		if (negate) {
			newvalue = -newvalue;
		}
		*_out = createDomElem(newvalue, _outputShouldBeInt);
	}
}
