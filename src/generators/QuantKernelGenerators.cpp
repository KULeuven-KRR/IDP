/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

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
	t->_alreadySeen = _alreadySeen;
	t->_outvars = _outvars;
	t->_subBddTrueGenerator = _subBddTrueGenerator->clone();
	t->_reset = _reset;
	return t;
}
void TrueQuantKernelGenerator::setVarsAgain(){
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
	stream << "all true instances of: " << nt() << toString(_subBddTrueGenerator);
	poptab();
}


FalseQuantKernelGenerator::FalseQuantKernelGenerator(InstGenerator* universegenerator, InstChecker* bddtruechecker)
		: _universeGenerator(universegenerator), _quantKernelTrueChecker(bddtruechecker), _reset(true) {
}

FalseQuantKernelGenerator* FalseQuantKernelGenerator::clone() const {
	auto t = new FalseQuantKernelGenerator(*this);
	t->_quantKernelTrueChecker = _quantKernelTrueChecker->clone();
	t->_universeGenerator = _universeGenerator->clone();
	t->_reset = _reset;
	return t;
}
void FalseQuantKernelGenerator::setVarsAgain(){
	_universeGenerator->setVarsAgain();
}

void FalseQuantKernelGenerator::reset() {
	_reset = true;
	_universeGenerator->begin();
	if (_universeGenerator->isAtEnd()) {
		notifyAtEnd();
	}
}

void FalseQuantKernelGenerator::next() {
	if (_reset) {
		_reset = false;
	} else {
		_universeGenerator->operator ++();
	}

	for (; not _universeGenerator->isAtEnd() && _quantKernelTrueChecker->check(); _universeGenerator->operator ++()) {
	}
	if (_universeGenerator->isAtEnd()) {
		notifyAtEnd();
	}
}
 void FalseQuantKernelGenerator::put(std::ostream& stream)  const{
	pushtab();
	stream << "FalseQuantKernelGenerator with kernelchecker: " << nt() << toString(_quantKernelTrueChecker);
	poptab();
}


