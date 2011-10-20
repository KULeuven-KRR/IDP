/************************************
	InvUnaGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INVUNAGENERATOR_HPP_
#define INVUNAGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class InvUNAGenerator : public InstGenerator {
	private:
		std::vector<const DomElemContainer*>	_outvars;
		std::vector<unsigned int>			_outpos;
		Universe						_universe;
		const DomElemContainer*			_resvar;
	public:
		InvUNAGenerator(const std::vector<bool>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ) {
			_universe = univ;
			for(unsigned int n = 0; n < pattern.size(); ++n) {
				if(!pattern[n]) {
					_outvars.push_back(vars[n]);
					_outpos.push_back(n);
				}
			}
			_resvar = vars.back();
		}
		bool first() const {
			assert(_resvar->get()->type() == DET_COMPOUND);
			const Compound* c = _resvar->get()->value()._compound;
			unsigned int n = 0;
			for(; n < _outpos.size(); ++n) {
				if(_universe.tables()[_outpos[n]]->contains(c->arg(_outpos[n]))) {
					*(_outvars[n]) = c->arg(_outpos[n]);
				}
				else { break; }
			}
			return (n == _outpos.size());
		}
		bool next() const {
			return false;
		}
};

#endif /* INVUNAGENERATOR_HPP_ */
