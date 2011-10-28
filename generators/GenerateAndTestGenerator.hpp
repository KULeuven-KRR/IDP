/************************************
 GenerateAndTestGenerator.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef GENERATEANDTESTGENERATOR_HPP_
#define GENERATEANDTESTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class GenerateAndTestGenerator: public InstGenerator {
private:
	const PredTable* _table;
	PredTable* _full;
	std::vector<const DomElemContainer*> _invars;
	std::vector<const DomElemContainer*> _outvars;
	std::vector<unsigned int> _inposs;
	std::vector<unsigned int> _outposs;
	std::vector<unsigned int> _firstocc;
	mutable TableIterator _currpos;
	mutable ElementTuple _currtuple;
public:
	GenerateAndTestGenerator(const PredTable*, const std::vector<bool>&,
			const std::vector<const DomElemContainer*>&, const std::vector<unsigned int>&, const Universe& univ) {
		vector<SortTable*> outuniv;
		for (unsigned int n = 0; n < pattern.size(); ++n) {
			if (pattern[n]) {
				_invars.push_back(vars[n]);
				_inposs.push_back(n);
			} else {
				_outposs.push_back(n);
				if (firstocc[n] == n) {
					_outvars.push_back(vars[n]);
					outuniv.push_back(univ.tables()[n]);
				}
			}
		}
		_full = new PredTable(new FullInternalPredTable(), Universe(outuniv));
	}
	bool first() const {
		_currpos = _full->begin();
		if (_currpos.hasNext()) {
			const ElementTuple& tuple = *_currpos;
			for (unsigned int n = 0; n < _inposs.size(); ++n) {
				_currtuple[_inposs[n]] = _invars[n]->get();
			}
			for (unsigned int n = 0; n < _outposs.size(); ++n) {
				_currtuple[_outposs[n]] = tuple[_firstocc[_outposs[n]]];
			}
			if (_table->contains(_currtuple)) {
				for (unsigned int n = 0; n < tuple.size(); ++n) {
					*(_outvars[n]) = tuple[n];
				}
				return true;
			} else
				return next();
		} else
			return false;
	}
	bool next() const {
		++_currpos;
		while (_currpos.hasNext()) {
			const ElementTuple& tuple = *_currpos;
			for (unsigned int n = 0; n < _outposs.size(); ++n) {
				_currtuple[_outposs[n]] = tuple[_firstocc[_outposs[n]]];
			}
			if (_table->contains(_currtuple)) {
				for (unsigned int n = 0; n < tuple.size(); ++n) {
					*(_outvars[n]) = tuple[n];
				}
				return true;
			}
			++_currpos;
		}
		return false;
	}
};

#endif /* GENERATEANDTESTGENERATOR_HPP_ */
