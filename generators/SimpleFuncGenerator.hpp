/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef SIMPLEFUNCGENERATOR_HPP_
#define SIMPLEFUNCGENERATOR_HPP_

#include <vector>
#include "generators/InstGenerator.hpp"

class FuncTable;
class DomElemContainer;
class DomainElement;
class Universe;

/**
 * Generator for a function when the range is output and there is an input/output pattern for the domain.
 */
class SimpleFuncGenerator : public InstGenerator {
private:
	const FuncTable* _function;
	InstGenerator* _univgen;
private:
	std::vector<const DomElemContainer*> _outvars;
	std::vector<unsigned int> _outpos;
	const DomElemContainer* _rangevar;
	bool _reset;

	std::vector<const DomElemContainer*> _vars;
	ElementTuple _currenttuple;

	std::vector<const DomElemContainer*> _invars;
	std::vector<unsigned int> _inpos;

	Universe _universe;

public:
	SimpleFuncGenerator(const FuncTable* ft, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ, const std::vector<unsigned int>& firstocc)
			: _function(ft), _rangevar(vars.back()), _vars(vars), _universe(univ) {
		Assert(pattern.back()==Pattern::OUTPUT);
		auto domainpattern = pattern;
		domainpattern.pop_back();

		std::vector<SortTable*> outtabs;
		for (unsigned int n = 0; n < domainpattern.size(); ++n) {
			switch(domainpattern[n]){
			case Pattern::OUTPUT:
				if(firstocc[n]==n){
					_outvars.push_back(vars[n]);
					outtabs.push_back(univ.tables()[n]);
				}
				_outpos.push_back(n);

				break;
			case Pattern::INPUT:
				_invars.push_back(vars[n]);
				_inpos.push_back(n);
				break;
			}
		}

		_univgen = GeneratorFactory::create(_outvars,outtabs);

		for(unsigned int i = 0; i<domainpattern.size(); ++i){
			_currenttuple.push_back(_vars[i]->get());
		}
		_currenttuple.push_back(_rangevar->get());
	}

	// FIXME reimplement (clone generator)
	SimpleFuncGenerator* clone() const{
		return new SimpleFuncGenerator(*this);
	}

	void reset(){
		_reset = true;
	}

	void next(){
		if(_reset){
			_reset = false;
			_univgen->begin();

			for(unsigned int i = 0; i<_vars.size(); ++i){
				_currenttuple[i] = _vars[i]->get();
			}
			for(unsigned int i=0; i<_inpos.size(); ++i){
				_currenttuple[_inpos[i]] = _invars[i]->get();
			}
		}
		if(_univgen->isAtEnd()){
			notifyAtEnd();
		}

		for(unsigned int i=0; i<_outpos.size(); ++i){
			_currenttuple[_outpos[i]] = _outvars[i]->get();
		}
		_rangevar->operator =(_function->operator [](_currenttuple));

		_univgen->operator ++();
		if(_univgen->isAtEnd()){
			notifyAtEnd();
		}
	}

	virtual void put(std::ostream& stream){
		stream <<toString(_function) <<"(";
		bool begin = true;
		for(unsigned int n = 0; n<_vars.size()-1; ++n){
			if(not begin){
				stream <<", ";
			}
			begin = false;
			stream <<_vars[n];
			stream <<toString(_universe.tables()[n]);
			for(auto i=_outpos.begin(); i<_outpos.end(); ++i){
				if(n==*i){
					stream <<"(out)";
				}
			}
			for(auto i=_inpos.begin(); i<_inpos.end(); ++i){
				if(n==*i){
					stream <<"(in)";
				}
			}
		}
		stream <<"):" <<_rangevar <<toString(_universe.tables().back()) <<"(out)";
	}
};

#endif /* SIMPLEFUNCGENERATOR_HPP_ */
