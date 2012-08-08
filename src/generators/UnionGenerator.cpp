/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "UnionGenerator.hpp"

UnionGenerator::UnionGenerator(std::vector<InstGenerator*>& generators, std::vector<InstGenerator*>& checkers)
		: _generators(generators), _checkers(checkers), _reset(false), _current(0) {
}

UnionGenerator* UnionGenerator::clone() const {
	auto t = new UnionGenerator(*this);
	for(auto gen:_generators){
		t->_generators.push_back(gen->clone());
	}
	for(auto check:_checkers){
		t->_checkers.push_back(check->clone());
	}
	t->_reset = _reset;
	t->_current = _current;
	return t;
}

void UnionGenerator::setVarsAgain(){
	_generators[_current]->setVarsAgain();
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
				_generators[0]->begin();
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
		stream << "*" << toString(*it);
	}
	poptab();
}

