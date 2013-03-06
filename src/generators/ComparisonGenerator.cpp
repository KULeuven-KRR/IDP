/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "ComparisonGenerator.hpp"

ComparisonGenerator::ComparisonGenerator(SortTable* leftsort, SortTable* rightsort,
		const DomElemContainer* leftvalue, const DomElemContainer* rightvalue,
		Input input, CompType type)
		: _leftsort(leftsort), _rightsort(rightsort), _leftvar(leftvalue), _rightvar(rightvalue), _comparison(type), _input(input),
			_left(leftsort->sortBegin()), _right(rightsort->sortBegin()), _reset(true), increaseouter(false) {
	if (_input == Input::RIGHT) {
		_leftsort = rightsort;
		_rightsort = leftsort;
		_leftvar = rightvalue;
		_rightvar = leftvalue;
		_input = Input::LEFT;
		_comparison = invertComp(_comparison);
	}
	if (not leftIsInput() && not _left.isAtEnd()) {
		_latestleft = *_left;
	}
	if (not rightIsInput() && not _right.isAtEnd()) {
		_latestright = *_right;
	}
}

void ComparisonGenerator::put(std::ostream& stream) const {
	stream << _leftvar << "[" << print(_leftsort) << "]" << (_input != Input::NONE ? "(in)" : "(out)");
	stream << print(_comparison);
	stream << _rightvar << "[" << print(_rightsort) << "]" << (_input == Input::BOTH ? "(in)" : "(out)");
}

ComparisonGenerator* ComparisonGenerator::clone() const {
	return new ComparisonGenerator(*this);
}

void ComparisonGenerator::internalSetVarsAgain() {
	if (not leftIsInput()) {
		_leftvar->operator =(_latestleft);
	}
	if (not rightIsInput()) {
		_rightvar->operator =(_latestright);
	}
}

void ComparisonGenerator::reset() {
	_reset = true;
	increaseouter = false;
}

void ComparisonGenerator::findNext(SortIterator* finiteside, SortIterator* undefinedside, SortTable* undefinedSort, const DomElemContainer* finiteContainer,
		const DomElemContainer* undefinedContainer, bool finiteGTorGEQundef) {
	bool stop = false;
	for (; not finiteside->isAtEnd() && not stop; ++(*finiteside)) {
		CHECKTERMINATION;
		if (_comparison != CompType::EQ) {
			for (; not (*undefinedside).isAtEnd() && not stop; ++(*undefinedside)) {
				CHECKTERMINATION;
				if (checkAndSet() == CompResult::VALID) {
					stop = true;
					break; // NOTE: essential to prevent ++
				} else if (finiteGTorGEQundef) {
					//in case l>r, we can stop increasing r whenever this fails
					break;
				}
			}
			if (stop) {
				break;
			}
			*undefinedside = undefinedSort->sortBegin();
		} else {
			if (undefinedSort->contains(**finiteside)) {
				finiteContainer->operator =(**finiteside);
				undefinedContainer->operator =(**finiteside);
				increaseouter = true;
				return;
			}
		}
	}
	if ((*finiteside).isAtEnd()) {
		notifyAtEnd();
	}
}

void ComparisonGenerator::next() {
	switch (_input) {
	case Input::BOTH:
		if (_reset && correct()) {
			_reset = false;
		} else {
			notifyAtEnd();
		}
		break;
	case Input::NONE: {
		if (_reset) {
			_reset = false;
			_left = _leftsort->sortBegin();
			_right = _rightsort->sortBegin();
		} else {
			if (_leftsort->finite()) {
				if (increaseouter) {
					++_left;
					increaseouter = false;
				} else {
					++_right;
				}
			} else {
				if (increaseouter) {
					++_right;
					increaseouter = false;
				} else {
					++_left;
				}
			}
		}
		if (_leftsort->finite()) { // NOTE: optimized for looping over non-finite sort first (if available)
			findNext(&_left, &_right, _rightsort, _leftvar, _rightvar, (_comparison == CompType::GT or _comparison == CompType::GEQ));
		} else {
			findNext(&_right, &_left, _leftsort, _rightvar, _leftvar, (_comparison == CompType::LT or _comparison == CompType::LEQ));
		}
		break;
	}
	case Input::LEFT: // NOTE: optimized EQ comparison
		if (_reset) {
			_reset = false;
			if (_comparison == CompType::EQ) {
				if (_rightsort->contains(_leftvar->get())) {
					_rightvar->operator =(_leftvar->get());
				} else {
					notifyAtEnd();
				}
				break;
			} else {
				_right = _rightsort->sortBegin();
			}
		} else {
			if (_comparison == CompType::EQ) {
				notifyAtEnd();
				break;
			}
			++_right;
		}
		for (; not _right.isAtEnd(); ++_right) {
			CHECKTERMINATION;
			if (checkAndSet() == CompResult::VALID) {
				break;
			} else if (_comparison == CompType::GEQ || _comparison == CompType::GT) {
				//Uses the fact that we generate in < order
				//TODO: do the same for input both
				notifyAtEnd();
				break;
			}
		}
		if (_right.isAtEnd()) {
			notifyAtEnd();
		}
		break;
	case Input::RIGHT:
		throw InternalIdpException("Invalid code path in comparisongenerator.");
	}
	_latestleft = _leftvar->get();
	_latestright = _rightvar->get();
}

bool ComparisonGenerator::leftIsInput() const {
	return _input == Input::BOTH || _input == Input::LEFT;
}
bool ComparisonGenerator::rightIsInput() const {
	return _input == Input::BOTH;
}

CompResult ComparisonGenerator::checkAndSet() {
	if (correct()) {
		if (not leftIsInput()) {
			_leftvar->operator =(*_left);
		}
		if (not rightIsInput()) {
			_rightvar->operator =(*_right);
		}
		return CompResult::VALID;
	}
	return CompResult::INVALID;
}

bool ComparisonGenerator::correct() {
	const DomainElement *leftelem, *rightelem;
	if (leftIsInput()) {
		leftelem = _leftvar->get();
	} else {
		leftelem = *_left;
	}
	if (rightIsInput()) {
		rightelem = _rightvar->get();
	} else {
		rightelem = *_right;
	}
	bool compsuccess = false;
	switch (_comparison) {
	case CompType::LT:
		if (*leftelem < *rightelem) {
			compsuccess = true;
		}
		break;
	case CompType::LEQ:
		if (*leftelem <= *rightelem) {
			compsuccess = true;
		}
		break;
	case CompType::GT:
		if (*leftelem > *rightelem) {
			compsuccess = true;
		}
		break;
	case CompType::GEQ:
		if (*leftelem >= *rightelem) {
			compsuccess = true;
		}
		break;
	case CompType::EQ:
		if (*leftelem == *rightelem) {
			compsuccess = true;
		}
		break;
	case CompType::NEQ:
		if (*leftelem != *rightelem) {
			compsuccess = true;
		}
		break;
	}
	return compsuccess;
}
