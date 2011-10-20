/************************************
	StrLessGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef STRLESSGENERATOR_HPP_
#define STRLESSGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

using namespace std;

class StrLessGenerator : public InstGenerator {
	// FIXME: allow for different domains for left and right variable
	private:
		SortTable*				_table;
		const DomElemContainer*	_leftvar;
		const DomElemContainer*	_rightvar;
		bool					_leftisinput;
		mutable SortIterator	_left;
		mutable SortIterator	_right;
	public:
		StrLessGenerator(SortTable* st, SortTable* fixme, const DomElemContainer* lv, const DomElemContainer* rv, bool inp) :
			_table(st), _leftvar(lv), _rightvar(rv), _leftisinput(inp), _left(_table->sortBegin()), _right(_table->sortBegin()) { }
		bool first() const {
			if(_leftisinput) _left = _table->sortIterator(_leftvar->get());
			else _left = _table->sortBegin();
			_right = _left;
			if(_right.hasNext()) {
				++_right;
				if(_right.hasNext()) {
					*_rightvar = *_right;
					if(not _leftisinput) { *_leftvar = *_left; }
					return true;
				}
				else { return false; }
			}
			else { return false; }
		}

		bool next() const {
			++_right;
			if(_right.hasNext()) {
				*_rightvar = *_right;
				if(not _leftisinput) { *_leftvar = *_left; }
				return true;
			}
			else if(not _leftisinput) {
				++_left;
				if(_left.hasNext()) {
					_right = _left;
					++_right;
					if(_right.hasNext()) {
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

#endif /* STRLESSGENERATOR_HPP_ */
