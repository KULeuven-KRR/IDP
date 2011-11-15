/************************************
	SimpleFuncGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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

public:
	SimpleFuncGenerator(const FuncTable* ft, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ, const std::vector<uint>& firstocc)
			: _function(ft), _rangevar(vars.back()), _vars(vars) {
		assert(pattern.back()==Pattern::OUTPUT);

		std::vector<SortTable*> outtabs;
		for (unsigned int n = 0; n < pattern.size(); ++n) {
			switch(pattern[n]){
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

		for(uint i = 0; i<_vars.size(); ++i){
			_currenttuple.push_back(_vars[i]->get());
		}
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

			for(uint i = 0; i<_vars.size(); ++i){
				_currenttuple[i] = _vars[i]->get();
			}
			for(uint i=0; i<_inpos.size(); ++i){
				_currenttuple[_inpos[i]] = _invars[i]->get();
			}
		}
		if(_univgen->isAtEnd()){
			notifyAtEnd();
		}

		for(uint i=0; i<_outpos.size(); ++i){
			_currenttuple[_outpos[i]] = _outvars[i]->get();
		}
		_rangevar->operator =(_function->operator [](_currenttuple));

		_univgen->operator ++();
		if(_univgen->isAtEnd()){
			notifyAtEnd();
		}
	}
};

#endif /* SIMPLEFUNCGENERATOR_HPP_ */
