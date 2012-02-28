/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef INVERSEINSTGENERATOR_HPP_
#define INVERSEINSTGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "LookupGenerator.hpp"
#include "GeneratorFactory.hpp"

/**
 * Given a predicate table, generate tuples which, given the input, are not in the predicate table
 */
// TODO optimize generators if we can guarantee that sorts are always traversed from smallest to largest. This leads to problems as it might be easier to iterate
// over N as 0 1 -1 2 -2 to avoid having to start at -infinity
// TODO Code below is correct, but can be optimized in case there are output variables that occur more than once in 'vars'
// TODO can probably be optimized a lot if with the order in which we run over it.
class InverseInstGenerator: public InstGenerator {
private:
	InstGenerator *_universegen;
	InstChecker *_predchecker;
	bool _reset;

public:
	// NOTE: takes ownership of table
	InverseInstGenerator(PredTable* table, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars)
			: _reset(true) {
		std::vector<const DomElemContainer*> outvars;
		std::vector<SortTable*> temptables;
		for (unsigned int i = 0; i < pattern.size(); ++i) {
			if (pattern[i] == Pattern::OUTPUT) {
				outvars.push_back(vars[i]);
				temptables.push_back(table->universe().tables()[i]);
			}
		}
		Universe universe(temptables);
		PredTable temp(new FullInternalPredTable(), universe);
		_universegen = GeneratorFactory::create(&temp, std::vector<Pattern>(outvars.size(), Pattern::OUTPUT), outvars, universe);
		_predchecker = new LookupGenerator(table, vars, table->universe());
	}

	// FIXME reimplement clone
	InverseInstGenerator* clone() const {
		throw notyetimplemented("Cloning generators.");
	}

	void reset() {
		_reset = true;
	}

	void next() {
		if (_reset) {
			_reset = false;
			_universegen->begin();
		} else {
			_universegen->operator ++();
		}

		for (; not _universegen->isAtEnd(); _universegen->operator ++()) {
			if (not _predchecker->check()) { // It is NOT a tuple in the table
				return;
			}
		}
		if (_universegen->isAtEnd()) {
			notifyAtEnd();
		}
	}
	virtual void put(std::ostream& stream) {
		stream << "Inverse instance generater: inverse of\n"<< tabs()<< toString(_predchecker) ;
	}

};

#endif /* INVERSEINSTGENERATOR_HPP_ */
