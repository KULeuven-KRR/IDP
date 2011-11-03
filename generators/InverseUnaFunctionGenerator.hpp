#ifndef INVUNAGENERATOR_HPP_
#define INVUNAGENERATOR_HPP_

#include "generators/InstGenerator.hpp"
#include <cassert>

class InverseUNAFuncGenerator: public InstGenerator {
private:
	std::vector<const DomElemContainer*> _outvars;
	std::vector<unsigned int> _outpos;
	Universe _universe;
	const DomElemContainer* _resvar;
	bool _reset;

#ifdef DEBUG
	std::vector<const DomElemContainer*> _invars;
	std::vector<unsigned int> _inpos;
#endif

public:
	InverseUNAFuncGenerator(const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ)
			: _reset(true){
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

	void reset(){
		_reset = true;
	}

	void next(){
		if(_reset){
			_reset = false;
			assert(_resvar->get()->type() == DET_COMPOUND);
			const Compound* c = _resvar->get()->value()._compound;
#ifdef DEBUG
			for (uint n = 0; n < _inpos.size(); ++n) {
				assert(_invars[n]->get()==c->arg(_inpos[n]));
			}
#endif
			for (uint n = 0; n < _outpos.size(); ++n) {
				if (_universe.tables()[_outpos[n]]->contains(c->arg(_outpos[n]))) {
					*(_outvars[n]) = c->arg(_outpos[n]);
				} else {
					notifyAtEnd();
					break;
				}
			}
		}else{
			notifyAtEnd();
		}
	}
};

#endif /* INVUNAGENERATOR_HPP_ */
