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
#include "utils/ListUtils.hpp"


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
		CHECKTERMINATION;
		auto elements = getDomainElements();
		if (not contains(_alreadySeen, elements)) {
			_alreadySeen.insert(elements);
			return;
		}

		// Check whether we already have all possible tuples in _alreadySeen given the outvars.
		// NOTE: currentlty, the domain of outvars is unknown, so we can only check this for the empty tuple
		if(_outvars.size()==0 && _alreadySeen.size()>0){
			break;
		}
	}
	if (_subBddTrueGenerator->isAtEnd() || (_outvars.size()==0 && _alreadySeen.size()>0)) {
		notifyAtEnd();
	}
}
 void TrueQuantKernelGenerator::put(std::ostream& stream)  const{
	pushtab();
	stream << "all true instances of: " << nt() << print(_subBddTrueGenerator);
	poptab();
}
