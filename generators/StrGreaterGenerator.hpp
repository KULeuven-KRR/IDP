/************************************
	StrGreaterGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef STRGREATERGENERATOR_HPP_
#define STRGREATERGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class StrGreaterGenerator : public InstGenerator {
	// FIXME: allow for different domains for left and right variable
	private:
		SortTable*				_table;
		const DomElemContainer*	_leftvar;
		const DomElemContainer*	_rightvar;
		bool					_leftisinput;
		mutable SortIterator	_left;
		mutable SortIterator	_right;
	public:
		StrGreaterGenerator(SortTable* st, SortTable* fixme, const DomElemContainer* lv, const DomElemContainer* rv, bool inp) :
			_table(st), _leftvar(lv), _rightvar(rv), _leftisinput(inp), _left(_table->sortBegin()), _right(_table->sortBegin()) { }
		bool first() const {
			if(_leftisinput) _left = _table->sortIterator(_leftvar->get());
			else _left = _table->sortBegin();
			_right = _table->sortBegin();
			if(_left.hasNext() && _right.hasNext()) {
				if(*(*_left) > *(*_right)) {
					*_rightvar = *_right;
					if(not _leftisinput) { *_leftvar = *_left; }
					return true;
				}
				else { return next(); }
			}
			else { return false; }
		}

		bool next() const {
			++_right;
			if(_left.hasNext() && _right.hasNext()) {
				if(*(*_left) > *(*_right)) {
					*_rightvar = *_right;
					if(not _leftisinput) { *_leftvar = *_left; }
					return true;
				}
			}
			if(not _leftisinput && _left.hasNext()) {
				++_left;
				_right = _table->sortBegin();
				if(_left.hasNext() && _right.hasNext()) {
					if(*(*_left) > *(*_right)) {
						*_rightvar = *_right;
						*_leftvar = *_left;
						return true;
					}
					else { return false; }
				}
				else { return false; }
			}
			else { return false; }
		}
};

#endif /* STRGREATERGENERATOR_HPP_ */
