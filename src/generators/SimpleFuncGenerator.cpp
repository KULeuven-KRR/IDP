/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
#include "SimpleFuncGenerator.hpp"
#include "GeneratorFactory.hpp"
#include "structure/MainStructureComponents.hpp"

// NOTE: DOES NOT take ownership of table
SimpleFuncGenerator::SimpleFuncGenerator(const FuncTable* ft, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars,
		const Universe& univ, const std::vector<unsigned int>& firstocc)
		: _functable(ft), _rangevar(vars.back()), _vars(vars), _universe(univ) {
	Assert(pattern.back() == Pattern::OUTPUT);
	auto domainpattern = pattern;
	domainpattern.pop_back();

	std::vector<SortTable*> outtabs;
	for (unsigned int n = 0; n < domainpattern.size(); ++n) {
		switch (domainpattern[n]) {
		case Pattern::OUTPUT:
			if (firstocc[n] == n) {
				_outvars.push_back(vars[n]);
				outtabs.push_back(univ.tables()[n]);
				_outpos.push_back(n);
			}
			break;
		case Pattern::INPUT:
			_invars.push_back(vars[n]);
			_inpos.push_back(n);
			break;
		}
	}

	_univgen = GeneratorFactory::create(_outvars, outtabs);

	for (unsigned int i = 0; i < domainpattern.size(); ++i) {
		_currenttuple.push_back(_vars[i]->get());
	}
}

SimpleFuncGenerator::~SimpleFuncGenerator() {
	delete (_univgen);
}

// FIXME reimplement
SimpleFuncGenerator* SimpleFuncGenerator::clone() const {
	auto gen = new SimpleFuncGenerator(*this);
	gen->_univgen = _univgen->clone();
	return gen;
}

void SimpleFuncGenerator::setVarsAgain() {
	auto result = _functable->operator [](_currenttuple);
	if (result != NULL) {
		if (_universe.tables().back()->contains(result)) {
			//TODO: this is not guaranteed by the functable, since the universes may differ! Should be fixed!
			_rangevar->operator =(result);
			return;
		}
	}
	_univgen->setVarsAgain();
}

void SimpleFuncGenerator::reset() {
	_reset = true;
}

void SimpleFuncGenerator::next() {
	if (_reset) {
		_reset = false;
		_univgen->begin();

		for (unsigned int i = 0; i < _vars.size() - 1; ++i) {
			Assert(_currenttuple.size()>i);
			_currenttuple[i] = _vars[i]->get();
		}
		for (unsigned int i = 0; i < _inpos.size(); ++i) {
			Assert(_currenttuple.size()>_inpos[i]);
			Assert(_invars.size()>i);
			_currenttuple[_inpos[i]] = _invars[i]->get();
		}
	} else {
		_univgen->operator ++();
	}

	while (not _univgen->isAtEnd()) {
		for (unsigned int i = 0; i < _outpos.size(); ++i) {
			Assert(_currenttuple.size()>_outpos[i]);
			Assert(_outvars.size()>i);
			Assert(_outvars[i]!=NULL);
			_currenttuple[_outpos[i]] = _outvars[i]->get();
		}
		auto result = _functable->operator [](_currenttuple);
		if (result != NULL) {
			if (_universe.tables().back()->contains(result)) {
				//TODO: this is not guaranteed by the functable, since the universes may differ! Should be fixed!
				_rangevar->operator =(result);
				return;
			}
		}
		_univgen->operator ++();
	}

	//No valid result found
	notifyAtEnd();
}

void SimpleFuncGenerator::put(std::ostream& stream) const {
	stream << toString(_functable) << "(";
	bool begin = true;
	for (size_t n = 0; n < _vars.size() - 1; ++n) {
		if (not begin) {
			stream << ", ";
		}
		begin = false;
		stream << toString(_vars[n]);
		stream << toString(_universe.tables()[n]);
		for (auto i = _outpos.begin(); i < _outpos.end(); ++i) {
			if (n == *i) {
				stream << "(out)";
			}
		}
		for (auto i = _inpos.begin(); i < _inpos.end(); ++i) {
			if (n == *i) {
				stream << "(in)";
			}
		}
	}
	stream << "):" << toString(_rangevar) << toString(_universe.tables().back()) << "(out)";
}

