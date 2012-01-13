/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef INVABSGENERATOR_HPP_
#define INVABSGENERATOR_HPP_

#include "generators/InstGenerator.hpp"
#include "common.hpp"

/**
 * Given the output of the abs function, generate all values that could have been input.
 */
class InverseAbsValueGenerator: public InstGenerator {
private:
	const DomElemContainer* _in;
	const DomElemContainer* _out;
	SortTable* _outdom;

	enum class State { RESET, FIRSTDONE, SECONDDONE};
	State _state;
	NumType _outputShouldBeInt;
public:
	InverseAbsValueGenerator(const DomElemContainer* in, const DomElemContainer* out, SortTable* dom, NumType outputShouldBeInt) :
			_in(in), _out(out), _outdom(dom), _state(State::RESET), _outputShouldBeInt(outputShouldBeInt) {
	}

	InverseAbsValueGenerator* clone() const{
		return new InverseAbsValueGenerator(*this);
	}

	void reset(){
		_state = State::RESET;
	}

	void next(){
		switch(_state){
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
			break;
		}
		if(not isAtEnd() && not _outdom->contains(_out->get())){
			next();
		}
	}

private:
	void setValue(bool negate){
		auto var = _in->get();
		if(var->type() == DET_INT){
			int newvalue = var->value()._int;
			if(negate){
				newvalue = -newvalue;
			}
			*_out = createDomElem(newvalue, _outputShouldBeInt);
		}else{
			Assert(var->type() == DET_DOUBLE);
			double newvalue = var->value()._double;
			if(negate){
				newvalue = -newvalue;
			}
			*_out = createDomElem(newvalue, _outputShouldBeInt);
		}
	}
};

#endif /* INVABSGENERATOR_HPP_ */
