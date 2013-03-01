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

#include "TreeInstGenerator.hpp"

OneChildGenerator* OneChildGenerator::clone() const {
	auto t = new OneChildGenerator(*this);
	t->_generator = _generator->clone();
	t->_child = _child->clone();
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
		CHECKTERMINATION;
		_child->begin();
		if (not _child->isAtEnd()) {
			return;
		}
	}
	Assert(_generator->isAtEnd());
	notifyAtEnd();
}

OneChildGenerator::~OneChildGenerator() {
	delete (_generator);
	delete (_child);
}

void OneChildGenerator::internalSetVarsAgain() {
	_generator->setVarsAgain();
	_child->setVarsAgain();
}

void OneChildGenerator::put(std::ostream& stream) const {
	stream << "generate values for: " << print(_generator) << nt();
	stream << "then for each such instantiation ";
	pushtab();
	stream << print(_child);
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
		CHECKTERMINATION;
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

TwoChildGenerator::~TwoChildGenerator() {
	delete (_checker);
	delete (_generator);
	delete (_falsecheckbranch);
	delete (_truecheckbranch);
}

void TwoChildGenerator::internalSetVarsAgain() {
	_generator->setVarsAgain();
	_truecheckbranch->setVarsAgain();
	_falsecheckbranch->setVarsAgain();
}

void TwoChildGenerator::put(std::ostream& stream) const {
	stream << "generate: ";
	pushtab();
	stream << print(_generator);
	poptab();
	stream << nt() << "if result is in ";
	pushtab();
	stream << print(_checker);
	poptab();

	stream << nt() << "THEN";
	pushtab();
	stream << nt() << print(_truecheckbranch);
	poptab();
	stream << nt() << "ELSE";
	pushtab();
	stream << nt() << print(_falsecheckbranch);
	poptab();
}
