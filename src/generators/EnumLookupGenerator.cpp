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

#include "EnumLookupGenerator.hpp"
#include <unordered_map>
#include "structure/MainStructureComponents.hpp"

typedef std::unordered_map<ElementTuple, std::vector<ElementTuple>, HashTuple> LookupTable;

EnumLookupGenerator::EnumLookupGenerator(std::shared_ptr<const LookupTable> t, const std::vector<const DomElemContainer*>& in, const std::vector<const DomElemContainer*>& out)
		: _table(t), _currpos(_table->cend()), _invars(in), _outvars(out), _reset(true), _currargs(_invars.size()) {
#ifdef DEBUG
	for(auto i=_table->cbegin(); i!=_table->cend(); ++i) {
		for(auto j=(*i).second.cbegin(); j<(*i).second.cend(); ++j) {
			Assert((*j).size()==out.size());
		}
	}
	for(auto i=in.cbegin(); i<in.cend(); ++i) {
		Assert(*i != NULL);
	}
	for(auto i=out.cbegin(); i<out.cend(); ++i) {
		Assert(*i != NULL);
	}
#endif
}

EnumLookupGenerator* EnumLookupGenerator::clone() const {
	auto g = new EnumLookupGenerator(*this);
	if(_currpos==_table->cend()){
		g->_currpos = g->_table->cend();
	}else{
		g->_currpos = g->_table->find(g->_currargs);
		Assert(g->_currpos!=g->_table->cend());
		g->_iter = g->_currpos->second.cbegin();
	}
	return g;
}

void EnumLookupGenerator::reset() {
	_reset = true;
}

// Increment is done AFTER returning a tuple!
void EnumLookupGenerator::next() {
	if (_reset) {
		_reset = false;
		for (unsigned int i = 0; i < _invars.size(); ++i) {
			_currargs[i] = _invars[i]->get();
		}
		_currpos = _table->find(_currargs);
		if (_currpos == _table->cend() || _currpos->second.size() == 0) {
			notifyAtEnd();
			return;
		}
		_iter = _currpos->second.cbegin();
	} else {
		++_iter;
		if (_iter == _currpos->second.cend()) {
			notifyAtEnd();
			return;
		}
	}
	Assert(_iter != _currpos->second.cend());
	for (unsigned int n = 0; n < _outvars.size(); ++n) {
		*(_outvars[n]) = (*_iter)[n];
	}
}

void EnumLookupGenerator::internalSetVarsAgain() {
	if(_currpos==_table->cend()){
		return;
	}
	if (_iter != _currpos->second.cend()) {
		for (unsigned int i = 0; i < _invars.size(); ++i) {
			*(_invars[i]) = _currargs[i];
		}
		for (unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = (*_iter)[n];
		}
	}
}

  void EnumLookupGenerator::put(std::ostream& stream) const{
	stream <<"enumerating table";
	if(_table->size()<10){
		stream <<print(_table);
	}else{
		stream <<"(too large to print)";
	}
}

