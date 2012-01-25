/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GENERATEANDTESTGENERATOR_HPP_
#define GENERATEANDTESTGENERATOR_HPP_

#include "InstGenerator.hpp"

class TableGenerator: public InstGenerator {
private:
	const PredTable* _fulltable;
	PredTable* _outputtable;
	std::vector<const DomElemContainer*> _allvars;
	std::vector<const DomElemContainer*> _outvars;
	std::vector<unsigned int> _outvaroccurence, _uniqueoutvarindex;
	ElementTuple _currenttuple;
	bool _reset;
	TableIterator _current;
public:
	TableGenerator(const PredTable* table, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars,
			const std::vector<unsigned int>& firstocc, const Universe& univ)
			: _fulltable(table), _allvars(vars), _reset(true) {
		std::vector<SortTable*> outuniv;
		for (unsigned int n = 0; n < pattern.size(); ++n) {
			if (pattern[n] == Pattern::OUTPUT) {
				_outvars.push_back(vars[n]);
				_outvaroccurence.push_back(n);
				_uniqueoutvarindex.push_back(_outvars.size() - 1);
				if (firstocc[n] == n) {
					outuniv.push_back(univ.tables()[n]);
				}
			}
		}
		_outputtable = new PredTable(new FullInternalPredTable(), Universe(outuniv));
	}

	~TableGenerator(){
		delete(_outputtable);
	}

	TableGenerator* clone() const {
		return new TableGenerator(*this);
	}

	void reset() {
		_reset = true;
	}

	void next() {
		if (_reset) {
			_reset = false;
			for (auto i = _allvars.begin(); i < _allvars.end(); ++i) {
				_currenttuple.push_back((*i)->get());
			}
			_current = _outputtable->begin();
			while (not _current.isAtEnd() && not inFullTable()) {
				++_current;
			}
			if (_current.isAtEnd()) {
				notifyAtEnd();
				return;
			}
		} else {
			++_current;

		}
	}

private:
	bool inFullTable() {
		const auto& values = *_current;
		for (unsigned int i = 0; i < values.size(); ++i) {
			_currenttuple[_outvaroccurence[i]] = values[_uniqueoutvarindex[i]];
		}
		return _fulltable->contains(_currenttuple);
	}
};

#endif /* GENERATEANDTESTGENERATOR_HPP_ */
