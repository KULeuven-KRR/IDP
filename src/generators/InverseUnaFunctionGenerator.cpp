/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "InverseUnaFunctionGenerator.hpp"
#include "common.hpp"
#include "structure/MainStructureComponents.hpp"

InverseUNAFuncGenerator::InverseUNAFuncGenerator(const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ)
		: _reset(true) {
	_universe = univ;
	for (unsigned int n = 0; n < pattern.size(); ++n) {
		if (pattern[n] == Pattern::OUTPUT) {
			_outvars.push_back(vars[n]);
			_outpos.push_back(n);
		}
#ifdef DEBUG
		if (pattern[n] == Pattern::INPUT) {
			_invars.push_back(vars[n]);
			_inpos.push_back(n);
		}
#endif
	}
	_resvar = vars.back();
}

InverseUNAFuncGenerator* InverseUNAFuncGenerator::clone() const {
	return new InverseUNAFuncGenerator(*this);
}

void InverseUNAFuncGenerator::reset() {
	_reset = true;
}

void InverseUNAFuncGenerator::next() {
	if (_reset) {
		_reset = false;
		Assert(_resvar->get()->type() == DET_COMPOUND);
		const Compound* c = _resvar->get()->value()._compound;
#ifdef DEBUG
		for (unsigned int n = 0; n < _inpos.size(); ++n) {
			Assert(_invars[n]->get()==c->arg(_inpos[n]));
		}
#endif
		for (unsigned int n = 0; n < _outpos.size(); ++n) {
			if (_universe.tables()[_outpos[n]]->contains(c->arg(_outpos[n]))) {
				*(_outvars[n]) = c->arg(_outpos[n]);
			} else {
				notifyAtEnd();
				break;
			}
		}
	} else {
		notifyAtEnd();
	}
}

