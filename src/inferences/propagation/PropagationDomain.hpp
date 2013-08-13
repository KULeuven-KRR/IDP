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

#ifndef PROPAGATEDOM_HPP
#define PROPAGATEDOM_HPP

#include <vector>

#include "PropagationCommon.hpp"
#include "IncludeComponents.hpp"
#include "fobdds/FoBddUtils.hpp"
#include <set>
#include <map>
#include <iostream>

class FOBDD;
class Variable;
class PredTable;

template<typename PropagatorDomain>
class FOPropDomainFactory;

/**
 * 	Class representing domains for formulas.
 */
class FOPropDomain {
protected:
	std::vector<Variable*> _vars;
public:
	FOPropDomain(const std::vector<Variable*>& vars);
	virtual ~FOPropDomain();
	const std::vector<Variable*>& vars() const;
	virtual FOPropDomain* clone() const = 0; //!< Take a deep copy of the domain

	// A formula is valid iff _vars is a subset of the free variables of the formula
	bool isValidFor(const Formula* f) const;
};

/**
 * A domain represented by a first-order BDD
 */
class FOPropBDDDomain: public FOPropDomain {
private:
	const FOBDD* _bdd;
public:
	FOPropBDDDomain(const FOBDD* bdd, const std::vector<Variable*>& vars);
	FOPropBDDDomain* clone() const;
	const FOBDD* bdd() const;

	// Checks that the free vars of the bdd are contained in vars.
	bool checkConsistency(std::shared_ptr<FOBDDManager> ) const;

	// Additional check (wrt the same method in FOPropDomain): that also internal consistency is achieved.
	bool isValidFor(const Formula* , std::shared_ptr<FOBDDManager> ) const;

	void put(std::ostream& stream) const;
};

/**
 * A domain represented by a predicate table
 */
class FOPropTableDomain: public FOPropDomain {
private:
	PredTable* _table;
public:
	FOPropTableDomain(PredTable* t, const std::vector<Variable*>& v);
	FOPropTableDomain* clone() const;
	PredTable* table() const;
};

/**
 * 	A domain is split in a certainly true and a certainly false part.
 */
template<class DomainType>
struct ThreeValuedDomain {
	DomainType* _ctdomain;
	DomainType* _cfdomain;
	bool _twovalued;
	//ThreeValuedDomain() : _ctdomain(NULL), _cfdomain(NULL), _twovalued(false) { }

	//If ctdom == true, then this represents the domain where f is always true
	//If cfdom == true, this represents a domain where f is always false
	//If ctdom==cfdom==false, this represents a domain where f is unknown
	//If ctdom==cfdom==true, this represents a domain where f is inconsistent (shouldn't happen!)
	//TODO: rename the booleans!!!
	ThreeValuedDomain(const FOPropDomainFactory<DomainType>* factory, bool ctdom, bool cfdom, const Formula* f) {
		_ctdomain = ctdom ? factory->trueDomain(f) : factory->falseDomain(f);
		_cfdomain = cfdom ? factory->trueDomain(f) : factory->falseDomain(f);
		_twovalued = ctdom || cfdom;
		Assert(not (ctdom && cfdom));
		Assert(_ctdomain!=NULL);
		Assert(_cfdomain!=NULL);
	}

	/**
	 * Creates a domain represented by a formula. _ctdomain are all tuples (over free vars of f) where f is true,
	 * _cfdomain are all tuples where f is false
	 */
	ThreeValuedDomain(const FOPropDomainFactory<DomainType>* factory, const Formula* f) {
		_ctdomain = factory->formuladomain(f);
		Formula* negf = f->clone();
		negf->negate();
		_cfdomain = factory->formuladomain(negf);
		_twovalued = true;
		negf->recursiveDelete();
		Assert(_ctdomain!=NULL);
		Assert(_cfdomain!=NULL);
	}

	ThreeValuedDomain(const FOPropDomainFactory<DomainType>* factory, const PredForm* pf, InitBoundType ibt) {
		Assert(ibt!=IBT_TWOVAL && ibt!=IBT_NONE);
		// TODO what with these cases?
		_twovalued = false;
		switch (ibt) {
		case IBT_CT:
			_ctdomain = factory->ctDomain(pf);
			_cfdomain = factory->falseDomain(pf);
			break;
		case IBT_CF:
			_ctdomain = factory->falseDomain(pf);
			_cfdomain = factory->cfDomain(pf);
			break;
		case IBT_BOTH:
			_ctdomain = factory->ctDomain(pf);
			_cfdomain = factory->cfDomain(pf);
			break;
		case IBT_TWOVAL:
		case IBT_NONE:
			break;
		}
	}

	std::ostream& put(std::ostream& stream) const{
		stream << "ct:";
		pushtab();
		stream << nt() << print(_ctdomain);
		poptab();
		stream << nt() << "cf:";
		pushtab();
		stream << nt() << print(_cfdomain);
		poptab();
		return stream;
	}
};

#endif //PROPAGATEDOM_HPP
