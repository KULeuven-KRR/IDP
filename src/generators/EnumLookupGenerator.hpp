/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ENUMLOOKUPGENERATOR_HPP_
#define ENUMLOOKUPGENERATOR_HPP_

#include <map>
#include "InstGenerator.hpp"

typedef std::map<ElementTuple, std::vector<ElementTuple>, Compare<ElementTuple> > LookupTable;

/**
 * Given a map from tuples to a list of tuples, with given input variables and output variables, go over the list of tuples of the corresponding input tuple.
 */
class EnumLookupGenerator: public InstGenerator {
private:
	LookupTable _table;
	LookupTable::const_iterator _currpos;
	std::vector<std::vector<const DomainElement*> >::const_iterator _iter;
	std::vector<const DomElemContainer*> _invars, _outvars;
	bool _reset;
	mutable std::vector<const DomainElement*> _currargs;
public:
	EnumLookupGenerator(const LookupTable& t, const std::vector<const DomElemContainer*>& in, const std::vector<const DomElemContainer*>& out)
			: _table(t), _invars(in), _outvars(out), _reset(true), _currargs(_invars.size()) {
#ifdef DEBUG
		for(auto i=_table.cbegin(); i!=_table.cend(); ++i) {
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

	// TODO quite expensive
	EnumLookupGenerator* clone() const {
		auto g = new EnumLookupGenerator(*this);
		g->_outvars = _outvars;
		g->_invars = _invars;
		g->_table = _table;
		g->_reset = _reset;
		g->_currargs = _currargs;
		g->_currpos = g->_table.find(g->_currargs);
		if(g->_currpos!=g->_table.cend()){
			g->_iter = g->_currpos->second.cbegin();
		}
		return g;
	}

	void reset() {
		_reset = true;
	}

	// Increment is done AFTER returning a tuple!
	void next() {
		if (_reset) {
			_reset = false;
			for (unsigned int i=0; i < _invars.size(); ++i) {
				_currargs[i]=_invars[i]->get();
			}
			_currpos = _table.find(_currargs);
			if (_currpos == _table.cend() || _currpos->second.size() == 0) {
				notifyAtEnd();
				return;
			}
			_iter = _currpos->second.cbegin();
		} else {
			if (_iter == _currpos->second.cend()) {
				notifyAtEnd();
				return;
			}
		}
		Assert(_iter != _currpos->second.cend());
		for (unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = (*_iter)[n];
		}
		++_iter;
	}

	void setVarsAgain(){
		for (unsigned int i=0; i < _invars.size(); ++i) {
			*(_invars[i]) = _currargs[i];
		}
		for (unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = (*_iter)[n];
		}
	}

	virtual void put(std::ostream& stream){
		stream << toString(_table);
	}

};

#endif /* ENUMLOOKUPGENERATOR_HPP_ */
