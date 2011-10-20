/************************************
	MinusGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MINUSGENERATOR_HPP_
#define MINUSGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class MinusGenerator : public InstGenerator {
	private:
		const DomElemContainer*	_in1;
		const DomElemContainer*	_in2;
		const DomElemContainer*	_out;
		bool					_int;
		SortTable*				_outdom;
	public:
		MinusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, bool i, SortTable* dom) :
			_in1(in1), _in2(in2), _out(out), _int(i), _outdom(dom) { }
		bool first() const {
			if(_int) *_out = DomainElementFactory::instance()->create(_in1->get()->value()._int - _in2->get()->value()._int);
			else {
				double d1 = _in1->get()->type() == DET_DOUBLE ? _in1->get()->value()._double : double(_in1->get()->value()._int);
				double d2 = _in2->get()->type() == DET_DOUBLE ? _in2->get()->value()._double : double(_in2->get()->value()._int);
				*_out = DomainElementFactory::instance()->create(d1 - d2);
			}
			return _outdom->contains(_out->get());
		}
		bool next()	const	{ return false;	}
};

#endif /* MINUSGENERATOR_HPP_ */
