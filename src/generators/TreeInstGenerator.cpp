/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "TreeInstGenerator.hpp"

OneChildGenerator* OneChildGenerator::clone() const {
	auto t = new OneChildGenerator(*this);
	bool initgen = _generator->isInitialized();
	t->_generator = _generator->clone();
	t->_child = _child->clone();
	Assert(not initgen || t->_generator->isInitialized());
	return t;
}

void OneChildGenerator::next() {
	if (_reset) {
		_reset = false;
		_generator->begin();
	} else {
		if (not _child->isAtEnd()) {
			_child->operator ++();
		}
		if (not _child->isAtEnd()) {
			return;
		}
		_generator->operator ++();
	}
	// Here, the generator is at a new value
	for (; not _generator->isAtEnd(); _generator->operator ++()) {
		_child->begin();
		if (not _child->isAtEnd()) {
			return;
		}
	}Assert(_generator->isAtEnd());
	notifyAtEnd();
}

void OneChildGenerator::put(std::ostream& stream) {
	stream << "generate: " << toString(_generator) << nt();
	stream << "then ";
	pushtab();
	stream << toString(_child);
	poptab();
}

TwoChildGenerator* TwoChildGenerator::clone() const {
	auto t = new TwoChildGenerator(*this);
	t->_checker = _checker->clone();
	t->_generator = _generator->clone();
	t->_falsecheckbranch = _falsecheckbranch->clone();
	t->_truecheckbranch = _truecheckbranch->clone();
	return t;
}

void TwoChildGenerator::next() {
	if (_reset) {
		_reset = false;
		_generator->begin();
	} else {
		if (_checker->check()) {
			if (_truecheckbranch->isAtEnd()) {
				_generator->operator ++();
			} else {
				_truecheckbranch->operator ++();
				if (not _truecheckbranch->isAtEnd()) {
					return;
				} else {
					_generator->operator ++();
				}
			}
		} else {
			if (_falsecheckbranch->isAtEnd()) {
				_generator->operator ++();
			} else {
				_falsecheckbranch->operator ++();
				if (not _falsecheckbranch->isAtEnd()) {
					return;
				} else {
					_generator->operator ++();
				}
			}
		}
	}
	// Here, the generator is at a new value
	for (; not _generator->isAtEnd(); _generator->operator ++()) {
		if (_checker->check()) {
			_truecheckbranch->begin();
			if (not _truecheckbranch->isAtEnd()) {
				return;
			}
		} else {
			_falsecheckbranch->begin();
			if (not _falsecheckbranch->isAtEnd()) {
				return;
			}
		}
	}
	if (_generator->isAtEnd()) {
		notifyAtEnd();
	}
}

void TwoChildGenerator::put(std::ostream& stream) {
	stream << "generate: ";
	pushtab();
	stream << toString(_generator);
	poptab();
	stream << nt() << "if result is in ";
	pushtab();
	toString(_checker);
	poptab();

	stream << nt() << "THEN";
	pushtab();
	stream << nt() << toString(_truecheckbranch);
	poptab();
	stream << nt() << "ELSE";
	pushtab();
	stream << nt() << toString(_falsecheckbranch);
	poptab();
}
