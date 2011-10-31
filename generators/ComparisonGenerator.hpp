/************************************
 StrGreaterGenerator.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef GTGenerator_HPP_
#define GTGenerator_HPP_

#include <cassert>
#include "generators/InstGenerator.hpp"

enum class Comp {
	GT, LT, LEQ, GEQ, EQ
};
enum class Input {
	BOTH, NONE, LEFT, RIGHT
};

/**
 * Formula: x op y, with op one of {=<, <, =>, >, =}, possibly different domains
 *
 * TODO can halve runtime in the case of both output and certainty of enumerating according to < order (by storing the previous first match).
 */
class ComparisonGenerator: public InstGenerator {
private:
	SortTable *_leftsort, *_rightsort;
	const DomElemContainer *_leftvar, *_rightvar;
	SortIterator _left, _right;
	Comp comparison;

	enum class CompResult {
		VALID, INVALID
	};

	Input input; // NOTE: is never RIGHT after initialization

public:
	ComparisonGenerator(SortTable* leftsort, SortTable* rightsort, const DomElemContainer* leftvalue, const DomElemContainer* rightvalue, Input input,
			Comp type) :
			_leftsort(leftsort), _rightsort(rightsort), _leftvar(leftvalue), _rightvar(rightvalue), comparison(type), input(input), _left(leftsort->sortBegin()), _right(rightsort->sortBegin()) {
		if (input == Input::RIGHT) {
			_leftsort = rightsort;
			_rightsort = leftsort;
			_leftvar = rightvalue;
			_rightvar = leftvalue;
			input == Input::LEFT;
		}
	}

	void reset() {
		switch (input) {
		case Input::NONE:
			notifyAtEnd();
			break;
		case Input::BOTH:
			_left = _leftsort->sortBegin();
			_right = _rightsort->sortBegin();
			if (_left.isAtEnd() || _right.isAtEnd()) {
				notifyAtEnd();
			}
			break;
		case Input::LEFT:
			_right = _rightsort->sortBegin();
			if (_right.isAtEnd()) {
				notifyAtEnd();
			}
			break;
		default: assert(false); break;
		}
	}

	void next() {
		switch (input) {
		case Input::NONE:
			break;
		case Input::BOTH:
			while (not _right.isAtEnd()) {
				while (not _left.isAtEnd()) {
					if (checkAndSet() == CompResult::VALID) {
						++_left;
						break;
					}
					++_left;
				}
				_left = _leftsort->sortBegin();
				++_right;
			}

			if (_left.isAtEnd() || _right.isAtEnd()) {
				notifyAtEnd();
			}
			break;
		case Input::LEFT:
			while (not _right.isAtEnd()) {
				if (checkAndSet() == CompResult::VALID) {
					++_right;
					break;
				}
				++_right;
			}

			if (_right.isAtEnd()) {
				notifyAtEnd();
			}
			break;
		default: assert(false); break;
		}
	}

private:
	CompResult checkAndSet() {
		auto leftelem = *_left;
		auto rightelem = *_right;
		bool compsuccess = false;
		switch (comparison) {
		case Comp::LT:
			if (*leftelem < *rightelem) {
				compsuccess = true;
			}
			break;
		case Comp::LEQ:
			if (*leftelem <= *rightelem) {
				compsuccess = true;
			}
			break;
		case Comp::GT:
			if (*leftelem > *rightelem) {
				compsuccess = true;
			}
			break;
		case Comp::GEQ:
			if (*leftelem >= *rightelem) {
				compsuccess = true;
			}
			break;
		case Comp::EQ:
			if (*leftelem == *rightelem) {
				compsuccess = true;
			}
			break;
		}
		if (compsuccess) {
			_leftvar->operator =(leftelem);
			_rightvar->operator =(rightelem);
			return CompResult::VALID;
		}
		return CompResult::INVALID;
	}
};

#endif /* GTGenerator_HPP_ */
