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

#include "InverseUnaFunctionGenerator.hpp"
#include "common.hpp"
#include "vocabulary/vocabulary.hpp"
#include "structure/MainStructureComponents.hpp"

InverseUNAFuncGenerator::InverseUNAFuncGenerator(Function* function, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ)
		: _function(function), _reset(true) {
	_universe = univ;
	/*
	 * Note: if pattern.back()!=Pattern::OUTPUT, then a SimpleFuncGenerator should have been constructed instead.
	 * See the method "void GeneratorFactory::visit(const FuncTable* ft)" for more information.
	 */
	if(pattern.back()==Pattern::OUTPUT){
		throw IdpException("Invalid code path in InverseUNAFuncGenerator");
	}
	for (unsigned int n = 0; n < pattern.size(); ++n) {
		if (pattern[n] == Pattern::OUTPUT) {
			_outvars.push_back(vars[n]);
			_outpos.push_back(n);
		}else{
			_invars.push_back(vars[n]);
			_inpos.push_back(n);
		}
	}
	_resvar = vars.back();

	Assert(_inpos.size()+_outpos.size()==pattern.size());
	Assert(_invars.size()+_outvars.size()==pattern.size());
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
		auto c = _resvar->get()->value()._compound;
		if(c->function()!=_function){
			notifyAtEnd();
			return;
		}
		// checking whether the input variables correspond to the arguments of the input compound constructor co-domain element
		// if not, notifyAtEnd
		for (unsigned int n = 0; n < _inpos.size(); ++n) {
			if(_inpos[n]<c->args().size()){
				if(_invars[n]->get()!=c->arg(_inpos[n])){
					notifyAtEnd();
					return;
				}
			}else{
				if(_invars[n]->get()->value()._compound!=c){
					notifyAtEnd();
					return;
				}
			}
		}
		for (unsigned int n = 0; n < _outpos.size(); ++n) {
			if (_universe.tables()[_outpos[n]]->contains(c->arg(_outpos[n]))) {
				*(_outvars[n]) = c->arg(_outpos[n]);
			} else {
				notifyAtEnd();
				return;
			}
		}
	} else {
		notifyAtEnd();
	}
}

void InverseUNAFuncGenerator::put(std::ostream& stream) const{
	stream <<"inverse UNA generator for " <<print(_function) <<" indices_in = " <<print(_inpos) <<", indices_out = " <<print(_outpos) <<"\n";
}
