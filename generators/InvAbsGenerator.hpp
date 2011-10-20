/************************************
	InvAbsGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INVABSGENERATOR_HPP_
#define INVABSGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class InvAbsGenerator : public InstGenerator {
	private:
		const DomElemContainer*	_in;
		const DomElemContainer*	_out;
		bool					_int;
		SortTable*				_outdom;
	public:
		InvAbsGenerator(const DomElemContainer* in, const DomElemContainer* out, bool i, SortTable* dom) :
			_in(in), _out(out), _int(i), _outdom(dom) { }
		bool first() const {
			if(_int) {
				*_out =  DomainElementFactory::instance()->create(-(_in->get()->value()._int));
			}
			else {
				double d = _in->get()->type() == DET_DOUBLE ? _in->get()->value()._double : double(_in->get()->value()._int);
				*_out = DomainElementFactory::instance()->create(-d);
			}
			if(_outdom->contains(_out->get())) return true;
			else return next();
		}
		bool next()	const {
			if(*_out == *_in) return false;
			else { *_out = *_in; return _outdom->contains(_out->get());	}
		}
};

#endif /* INVABSGENERATOR_HPP_ */
