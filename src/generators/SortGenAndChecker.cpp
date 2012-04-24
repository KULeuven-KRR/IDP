/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "SortGenAndChecker.hpp"

SortGenerator::SortGenerator(const InternalSortTable* table, const DomElemContainer* var)
		: _table(table), _var(var), _curr(_table->sortBegin()), _reset(true) {
}

SortGenerator* SortGenerator::clone() const {
	return new SortGenerator(*this);
}

void SortGenerator::reset() {
	_reset = true;
}

void SortGenerator::setVarsAgain() {
	*_var = *_curr;
}

void SortGenerator::next() {
	if (_reset) {
		_reset = false;
		_curr = _table->sortBegin();
	} else {
		++_curr;
	}
	if (_curr.isAtEnd()) {
		notifyAtEnd();
	} else {
		*_var = *_curr;
	}
}

void SortGenerator::put(std::ostream& stream) {
	pushtab();
	stream << "generator for sort: " << toString(_table);
	poptab();
}

SortChecker::SortChecker(const InternalSortTable* t, const DomElemContainer* in)
		: _table(t), _invar(in), _reset(true) {
}

SortChecker* SortChecker::clone() const {
	return new SortChecker(*this);
}

void SortChecker::reset() {
	_reset = true;
}

void SortChecker::next() {
	if (_reset) {
		if (not _table->contains(_invar->get())) {
			notifyAtEnd();
		}
		_reset = false;
		return;
	}
	notifyAtEnd();
}

