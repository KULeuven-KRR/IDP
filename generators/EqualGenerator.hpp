/************************************
	EqualGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EQUALGENERATOR_HPP_
#define EQUALGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class EqualGenerator : public InstGenerator {
	private:
		const DomElemContainer*	_in;
		const DomElemContainer*	_out;
		SortTable*				_indom;
		SortTable*				_outdom;
		mutable SortIterator	_curr;
	public:
		EqualGenerator(const DomElemContainer* in, const DomElemContainer* out, SortTable* outdom) :
			_in(in), _out(out), _indom(0), _outdom(outdom), _curr(_outdom->sortBegin()) { }
		EqualGenerator(const DomElemContainer* in, const DomElemContainer* out, SortTable* indom, SortTable* outdom) :
			_in(in), _out(out), _indom(indom), _outdom(outdom), _curr(_indom->sortBegin()) { }
		bool first() const {
			if(_indom) {
				_curr = _indom->sortBegin();
				if(not _curr.hasNext()) { return false; }
				*_in = *_curr;
			}
			*_out = *_in;
			if(_outdom->contains(_out->get())) return true;
			else return next();
		}
		bool next()	const {
			if(_indom) {
				++_curr;
				while(_curr.hasNext()) {
					*_in = *_curr;
					*_out = *_in;
					if(_outdom->contains(_out->get())) return true;
					++_curr;
				}
			}
			return false;
		}
};

#endif /* EQUALGENERATOR_HPP_ */
