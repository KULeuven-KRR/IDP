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

#include "inferences/grounding/GroundUtils.hpp"
#include <iostream>
#include <typeinfo>
#include "structure/TableSize.hpp"

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
	tablesize _maxsize;

public:
	Grounder(AbstractGroundTheory* gt, const GroundingContext& context);
	virtual ~Grounder() {
	}

	void toplevelRun() const; // Guaranteed toplevel run.
	Lit groundAndReturnLit() const; // Explicitly request one literal equisat with subgrounding. NOTE: interprets returnvalue as if in conjunction (false is unsat, true is sat)

	void wrapRun(ConjOrDisj& formula) const;

	// NOTE: it is IMPERATIVE to set the type of the formula within run!
	// NOTE: only call directly in wrapRun!
	virtual void run(ConjOrDisj& formula) const = 0;

	AbstractGroundTheory* getGrounding() const {
		return _grounding;
	}

	GroundTranslator* getTranslator() const;

	const GroundingContext& context() const {
		return _context;
	}

	virtual void put(std::ostream&) const = 0;

	void setMaxGroundSize(const tablesize& maxsize);

	static int _groundedatoms;
	static tablesize _fullgroundsize;
	static int groundedAtoms() {
		return _groundedatoms;
	}
	static void notifyGroundedAtom(){
		_groundedatoms++;
	}
	static const tablesize& getFullGroundSize(){
		return _fullgroundsize;
	}
	static void addToFullGroundSize(const tablesize& size){
		_fullgroundsize = _fullgroundsize + size;
	}
	tablesize getMaxGroundSize() const {
		return _maxsize;
	}

	int verbosity() const;
};

void addToGrounding(AbstractGroundTheory* gt, ConjOrDisj& formula);

#endif /* GROUNDER_HPP_ */
