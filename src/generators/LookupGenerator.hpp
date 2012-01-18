/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef SIMPLELOOKUPGENERATOR_HPP_
#define SIMPLELOOKUPGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

/**
 * A generator which checks whether a fully instantiated list of variables is a valid tuple for a certain predicate
 */
class LookupGenerator: public InstGenerator {
private:
	const PredTable* _table;
	std::vector<const DomElemContainer*> _vars;
	Universe _universe; // FIXME waarvoor is dit universe nu juist nodig? => om het juiste supertype mee te geven, je moet niet perse genereren over het type van de tabel

	bool _reset;

public:
	// NOTE: takes ownership of the table
	LookupGenerator(const PredTable* t, const std::vector<const DomElemContainer*>& vars, const Universe& univ)
			: _table(t), _vars(vars), _universe(univ), _reset(true) {
		Assert(t->arity() == vars.size());
	}

	LookupGenerator* clone() const {
		return new LookupGenerator(*this);
	}

	~LookupGenerator(){
		delete(_table);
	}

	void reset() {
		_reset = true;
	}

	void next() {
		if (_reset) {
			_reset = false;
			std::vector<const DomainElement*> _currargs;
			for (auto i = _vars.begin(); i < _vars.end(); ++i) {
				_currargs.push_back((*i)->get());
			}
			bool allowedvalue = (_table->contains(_currargs) && _universe.contains(_currargs));
			if (not allowedvalue) {
				notifyAtEnd();
			}
		} else {
			notifyAtEnd();
		}
	}

	virtual void put(std::ostream& stream) {
		stream << toString(_table) << "(";
		bool begin = true;
		for (unsigned int n = 0; n < _vars.size(); ++n) {
			if (not begin) {
				stream << ", ";
			}
			begin = false;
			stream << _vars[n] << "(in)";
		}
		stream << ")";
	}
};

#endif /* SIMPLELOOKUPGENERATOR_HPP_ */
