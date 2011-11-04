/************************************
 UminGenerator.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef UMINGENERATOR_HPP_
#define UMINGENERATOR_HPP_

#include "generators/InstGenerator.hpp"
#include "commontypes.hpp"
#include <cassert>

using namespace std;

class InvertNumericGenerator: public InstGenerator {
private:
	const DomElemContainer* _in;
	const DomElemContainer* _out;
	SortTable* _outdom;
	bool _reset;
	NumType _outputShouldBeInt;
public:
	InvertNumericGenerator(const DomElemContainer* in, const DomElemContainer* out, SortTable* dom, NumType outputShouldBeInt) :
			_in(in), _out(out), _outdom(dom), _reset(true), _outputShouldBeInt(outputShouldBeInt) {
	}

	void reset(){
		_reset = true;
	}

	void next(){
		if(_reset){
			auto var = _in->get();
			if(var->type() == DET_INT){
				int newvalue = - var->value()._int;
				*_out = DomainElementFactory::instance()->create(newvalue, _outputShouldBeInt);
			}else{
				assert(var->type() == DET_DOUBLE);
				double newvalue = -1 * var->value()._double;
				*_out = DomainElementFactory::instance()->create(newvalue, _outputShouldBeInt);
			}
			if(_outdom->contains(_out->get())){
				notifyAtEnd();
			}
			_reset = false;
		}else{
			notifyAtEnd();
		}
	}
};

#endif /* UMINGENERATOR_HPP_ */