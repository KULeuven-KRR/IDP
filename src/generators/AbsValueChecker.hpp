/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ABSVALCH_HPP
#define ABSVALCH_HPP

#include "InstGenerator.hpp"
#include "common.hpp"

/**
 * Given the output of the abs function, generate all values that could have been input.
 */
class AbsValueChecker: public InstGenerator {
private:
	const DomElemContainer* _in;
	const DomElemContainer* _out;
	Universe _universe;
	bool _reset;

public:
	AbsValueChecker(const DomElemContainer* in, const DomElemContainer* out, Universe universe) :
			_in(in), _out(out), _universe(universe) {
	}

	InverseAbsValueGenerator* clone() const {
		throw notyetimplemented("Cloning InverseAbsValueGenerators.");
	}

	void reset() {
		_reset = true;
	}
	virtual bool check() {
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
		return (outval > 0 && (inval == outval || inval == -outval)) && _universe.contains( { _in->get(), _out->get() });
	}

	void next() {
		if (not _reset) {
			notifyAtEnd();
		} else {
			_reset = false;
			Assert(_in->get()->type()==DET_INT || _in->get()->type()==DET_DOUBLE);
			Assert(_out->get()->type()==DET_INT || _out->get()->type()==DET_DOUBLE);
			if (not check()) {
				notifyAtEnd();
				return;
			}
		}
	}
};

#endif /* ABSVALCH_HPP */
