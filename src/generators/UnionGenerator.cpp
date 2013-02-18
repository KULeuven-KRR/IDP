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

#include "UnionGenerator.hpp"

UnionGenerator::UnionGenerator(const std::vector<InstGenerator*>& generators, const std::vector<InstChecker*>& checkers)
		: _generators(generators), _checkers(checkers), _reset(false), _current(0) {
	Assert(_generators.size()==_checkers.size());
}

UnionGenerator* UnionGenerator::clone() const {
	auto t = new UnionGenerator(*this);
	t->_generators.clear();
	for(auto gen:_generators){
		t->_generators.push_back(gen->clone());
	}
	t->_checkers.clear();
	for(auto check:_checkers){
		t->_checkers.push_back(check->clone());
	}
	return t;
}

void UnionGenerator::internalSetVarsAgain(){
	if(_current<_generators.size()){
		_generators[_current]->setVarsAgain();
	}
}

void UnionGenerator::reset() {
	_reset = true;
	_current = 0;
}

bool UnionGenerator::alreadySeen() {
	for (unsigned int i = 0; i < _current; i++) {
		if (_checkers[i]->check()) {
			return true;
		}
	}
	return false;
}
void UnionGenerator::next() {
	do {
		if (_reset) {
			_reset = false;
			if (_generators.size() == 0) {
				notifyAtEnd();
				return;
			} else {
				_generators[_current]->begin();
			}
		} else {
			_generators[_current]->operator ++();
		}
		while (_generators[_current]->isAtEnd()) {
			_current++;
			if (_current < _generators.size()) {
				_generators[_current]->begin();
			} else {
				notifyAtEnd();
				return;
			}
		}
	} while (alreadySeen());
}
void UnionGenerator::put(std::ostream& stream)  const{
	stream << "Union generator: union of ";
	pushtab();
	stream << nt();
	bool first = true;
	for (auto it = _generators.cbegin(); it != _generators.cend(); ++it) {
		if (not first) {
			stream << nt();
		} else {
			first = false;
		}
		stream << "*" << print(*it);
	}
	poptab();
}

