/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include "inferences/grounding/GroundUtils.hpp"
#include <iostream>
#include <typeinfo>
#include "structure/TableSize.hpp"

enum class Conn {
	DISJ, CONJ
};

Conn negateConn(Conn c);

template<class T>
void deleteDeep(T& object) {
	object->recursiveDelete();
	object = NULL;
}

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
class Formula;

typedef std::set<const DomElemContainer*> containerset;

/**
 * Class representing a request on what to ground of the constraint at hand.
 * A grounder is always allowed to ignore this information
 */
struct LazyGroundingRequest {
	bool groundersdone;
	containerset instantiation; // NOTE: variables which are already instantiated and should NOT be overwritten within the grounder (e.g. during quantification)

	LazyGroundingRequest(const containerset& instantantiation)
			: groundersdone(true), instantiation(instantantiation) {
	}
};

class Grounder {
private:
	AbstractGroundTheory* _grounding;
	GroundingContext _context;
	tablesize _maxsize;

protected:
	// NOTE: it is IMPERATIVE to set the type of the formula within run!
	// NOTE: only call directly in wrapRun!
	// NOTE: formula passed as reference argument to prevent vector copying during grounding
	virtual void internalRun(ConjOrDisj& formula, LazyGroundingRequest& request) = 0;

public:
	Grounder(AbstractGroundTheory* gt, const GroundingContext& context);
	virtual ~Grounder();

	bool toplevelRun() { // Guaranteed toplevel run.
		auto lgr = LazyGroundingRequest( { });
		return toplevelRun(lgr);
	}
	bool toplevelRun(LazyGroundingRequest& request); // Guaranteed toplevel run.
	Lit groundAndReturnLit(LazyGroundingRequest& request); // Explicitly request one literal equisat with subgrounding. NOTE: interprets returnvalue as if in conjunction (false is unsat, true is sat)

	void run(ConjOrDisj& formula, LazyGroundingRequest& request);

	AbstractGroundTheory* getGrounding() const {
		return _grounding;
	}

	GroundTranslator* translator() const;

	void setConjUntilRoot(bool value){
		_context._conjunctivePathFromRoot = value;
	}
	const GroundingContext& getContext() const {
		return _context;
	}

	virtual void put(std::ostream&) const = 0;

	void setMaxGroundSize(const tablesize& maxsize);

	static int _groundedatoms;
	static tablesize _fullgroundsize;
	static int groundedAtoms() {
		return _groundedatoms;
	}
	static void notifyGroundedAtom() {
		_groundedatoms++;
	}
	static const tablesize& getFullGroundingSize() {
		return _fullgroundsize;
	}
	static void addToFullGroundingSize(const tablesize& size) {
		_fullgroundsize = _fullgroundsize + size;
	}
	tablesize getMaxGroundSize() const {
		return _maxsize;
	}

	int verbosity() const;
};

class TheoryGrounder: public Grounder {
private:
	std::vector<Grounder*> subgrounders;
protected:
	virtual void internalRun(ConjOrDisj& formula, LazyGroundingRequest& request) {
		formula.setType(Conn::CONJ);
		for (auto sg : subgrounders) {
			sg->toplevelRun(request);
		}
	}
public:
	TheoryGrounder(AbstractGroundTheory* gt, const GroundingContext& context, const std::vector<Grounder*>& subgrounders)
			: Grounder(gt, context), subgrounders(subgrounders) {

	}

	const std::vector<Grounder*>& getSubGrounders() const {
		return subgrounders;
	}

	virtual void put(std::ostream& stream) const {
		for (auto sg : subgrounders) {
			stream << toString(sg) << "\n";
		}
	}
};

void addToGrounding(AbstractGroundTheory* gt, ConjOrDisj& formula);
