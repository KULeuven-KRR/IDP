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

#include "QuantKernelGenerators.hpp"


ElementTuple TrueQuantKernelGenerator::getDomainElements() {
	ElementTuple tuple;
	for (auto i = _outvars.cbegin(); i < _outvars.cend(); ++i) {
		tuple.push_back((*i)->get());
	}
	return tuple;
}
TrueQuantKernelGenerator::TrueQuantKernelGenerator(InstGenerator* subBddTrueGenerator, std::vector<const DomElemContainer*> outvars)
		: _subBddTrueGenerator(subBddTrueGenerator), _outvars(outvars), _reset(true) {
}

TrueQuantKernelGenerator* TrueQuantKernelGenerator::clone() const {
	auto t = new TrueQuantKernelGenerator(*this);
	t->_subBddTrueGenerator = _subBddTrueGenerator->clone();
	return t;
}
void TrueQuantKernelGenerator::internalSetVarsAgain(){
	_subBddTrueGenerator->setVarsAgain();
}

void TrueQuantKernelGenerator::reset() {
	_reset = true;
	_alreadySeen.clear();
	_subBddTrueGenerator->begin();
	if (_subBddTrueGenerator->isAtEnd()) {
		notifyAtEnd();
	}
}

void TrueQuantKernelGenerator::next() {
	if (_reset) {
		_reset = false;
	} else {
		_subBddTrueGenerator->operator ++();
	}
	for (; not _subBddTrueGenerator->isAtEnd(); _subBddTrueGenerator->operator ++()) {
		auto elements = getDomainElements();
		if (_alreadySeen.find(elements) == _alreadySeen.cend()) {
			_alreadySeen.insert(elements);
			return;
		}
	}
	if (_subBddTrueGenerator->isAtEnd()) {
		notifyAtEnd();
	}
}
 void TrueQuantKernelGenerator::put(std::ostream& stream)  const{
	pushtab();
	stream << "all true instances of: " << nt() << print(_subBddTrueGenerator);
	poptab();
}
