/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef UMINGENERATOR_HPP_
#define UMINGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "common.hpp"

class InvertNumericGenerator: public InstGenerator {
private:
	const DomElemContainer* _in;
	const DomElemContainer* _out;
	SortTable* _outdom;
	bool _reset;
	NumType _outputShouldBeInt;
public:
	InvertNumericGenerator(const DomElemContainer* in, const DomElemContainer* out, SortTable* dom, NumType outputShouldBeInt)
			: _in(in), _out(out), _outdom(dom), _reset(true), _outputShouldBeInt(outputShouldBeInt) {
	}

	InvertNumericGenerator* clone() const {
		return new InvertNumericGenerator(*this);
	}

	void reset() {
		_reset = true;
	}

	void next() {
		if (_reset) {
			auto var = _in->get();
			if (var->type() == DET_INT) {
				int newvalue = -var->value()._int;
				*_out = createDomElem(newvalue, _outputShouldBeInt);
			} else {
				Assert(var->type() == DET_DOUBLE);
				double newvalue = -1 * var->value()._double;
				*_out = createDomElem(newvalue, _outputShouldBeInt);
			}
			if (_outdom->contains(_out->get())) {
				notifyAtEnd();
			}
			_reset = false;
		} else {
			notifyAtEnd();
		}
	}
};

#endif /* UMINGENERATOR_HPP_ */
