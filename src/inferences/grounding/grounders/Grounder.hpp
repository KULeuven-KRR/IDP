/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUNDER_HPP_
#define GROUNDER_HPP_

#include "GroundUtils.hpp"
#include <iostream>
#include <typeinfo>

enum class Conn {
	DISJ, CONJ
};

Conn negateConn(Conn c);

struct ConjOrDisj {
	litlist literals;
private:
	Conn _type;
	bool _settype; // NOTE: true if the type has been set externally. Allows to check whether grounder have forgotten to set the value (only runtime check)
public:
	Conn getType() const {
		Assert(_settype);
		return _type;
	}
	void setType(Conn c) {
		_settype = true;
		_type = c;
	}
	void negate();

	void put(std::ostream& stream) const;

	ConjOrDisj()
			: _type(Conn::DISJ), _settype(false) {
	}
};

class AbstractGroundTheory;
class GroundTranslator;

class Grounder {
private:
	AbstractGroundTheory* _grounding;
	GroundingContext _context;

public:
	Grounder(AbstractGroundTheory* gt, const GroundingContext& context)
			: _grounding(gt), _context(context) {
	}
	virtual ~Grounder() {
	}

	void toplevelRun() const; // Guaranteed toplevel run.
	Lit groundAndReturnLit() const; // Explicitly requesting one literal equisat with subgrounding. NOTE: interprete returnvalue as if in conjunction (false is unsat, true is sat)
	virtual void run(ConjOrDisj& formula) const = 0;

	AbstractGroundTheory* getGrounding() const {
		return _grounding;
	}

	GroundTranslator* getTranslator() const;

	const GroundingContext& context() const {
		return _context;
	}
};

#endif /* GROUNDER_HPP_ */
