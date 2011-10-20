/************************************
	UminGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef UMINGENERATOR_HPP_
#define UMINGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

using namespace std;

class UminGenerator : public InstGenerator {
	private:
		const DomElemContainer*	_in;
		const DomElemContainer*	_out;
		bool					_int;
		SortTable*				_outdom;
	public:
		UminGenerator(const DomElemContainer* in, const DomElemContainer* out, bool i, SortTable* dom) :
			_in(in), _out(out), _int(i), _outdom(dom) { }
		bool first() const {
			if(_int) {
				*_out =  DomainElementFactory::instance()->create(-(_in->get()->value()._int));
			}
			else {
				double d = _in->get()->type() == DET_DOUBLE ? _in->get()->value()._double : double(_in->get()->value()._int);
				*_out = DomainElementFactory::instance()->create(-d);
			}
			return _outdom->contains(_out->get());
		}
		bool next()	const { return false;	}
};

#endif /* UMINGENERATOR_HPP_ */
