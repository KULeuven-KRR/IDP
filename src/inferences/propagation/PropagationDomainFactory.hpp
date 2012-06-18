/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef PROPAGATEDOMFACT_HPP
#define PROPAGATEDOMFACT_HPP

#include "PropagationCommon.hpp"
#include "PropagationDomain.hpp"
#include <set>
#include <vector>
#include <map>

class FOBDD;
class FOBDDManager;
class Formula;
class PredForm;
class Variable;
class PredInter;
class FOPropBDDDomain;
class AbstractStructure;
class FOPropTableDomain;



template<class DomainType> struct ThreeValuedDomain;

/**
 * Class to create and managed domains for formulas
 */
template<class PropagatorDomain>
class FOPropDomainFactory {
public:
	virtual ~FOPropDomainFactory() {
	}
	virtual PropagatorDomain* trueDomain(const Formula*) const = 0; //!< Create a domain containing all tuples
	virtual PropagatorDomain* falseDomain(const Formula*) const = 0; //!< Create an empty domain
	virtual PropagatorDomain* formuladomain(const Formula*) const = 0;
	virtual PropagatorDomain* ctDomain(const PredForm*) const = 0;
	virtual PropagatorDomain* cfDomain(const PredForm*) const = 0;
	virtual PropagatorDomain* exists(PropagatorDomain*, const std::set<Variable*>&) const = 0;
	virtual PropagatorDomain* forall(PropagatorDomain*, const std::set<Variable*>&) const = 0;
	virtual PropagatorDomain* conjunction(PropagatorDomain*, PropagatorDomain*) const = 0;
	virtual PropagatorDomain* disjunction(PropagatorDomain*, PropagatorDomain*) const = 0;
	virtual PropagatorDomain* substitute(PropagatorDomain*, const std::map<Variable*, Variable*>&) const = 0;
	virtual bool approxequals(PropagatorDomain*, PropagatorDomain*) const = 0; //!< Checks if two domains are equal
	virtual PredInter* inter(const std::vector<Variable*>&, const ThreeValuedDomain<PropagatorDomain>&, AbstractStructure*) const = 0;
	virtual std::ostream& put(std::ostream&, PropagatorDomain*) const = 0;
};

class FOPropBDDDomainFactory: public FOPropDomainFactory<FOPropBDDDomain> {
private:
	FOBDDManager* _manager;
public:
	FOPropBDDDomainFactory();
	~FOPropBDDDomainFactory();
	FOBDDManager* manager() const {
		return _manager;
	}
	FOPropBDDDomain* trueDomain(const Formula*) const;
	FOPropBDDDomain* falseDomain(const Formula*) const;
	FOPropBDDDomain* formuladomain(const Formula*) const;
	FOPropBDDDomain* ctDomain(const PredForm*) const;
	FOPropBDDDomain* cfDomain(const PredForm*) const;
	FOPropBDDDomain* exists(FOPropBDDDomain*, const std::set<Variable*>&) const;
	FOPropBDDDomain* forall(FOPropBDDDomain*, const std::set<Variable*>&) const;
	FOPropBDDDomain* conjunction(FOPropBDDDomain*, FOPropBDDDomain*) const;
	FOPropBDDDomain* disjunction(FOPropBDDDomain*, FOPropBDDDomain*) const;
	FOPropBDDDomain* substitute(FOPropBDDDomain*, const std::map<Variable*, Variable*>&) const;
	bool approxequals(FOPropBDDDomain*, FOPropBDDDomain*) const;
	PredInter* inter(const std::vector<Variable*>&, const ThreeValuedDomain<FOPropBDDDomain>&, AbstractStructure*) const;
	std::ostream& put(std::ostream&, FOPropBDDDomain*) const;
};

class FOPropTableDomainFactory: public FOPropDomainFactory<FOPropTableDomain> {
private:
	AbstractStructure* _structure;
public:
	FOPropTableDomainFactory(AbstractStructure*);
	FOPropTableDomain* trueDomain(const Formula*) const;
	FOPropTableDomain* falseDomain(const Formula*) const;
	FOPropTableDomain* formuladomain(const Formula*) const;
	FOPropTableDomain* ctDomain(const PredForm*) const;
	FOPropTableDomain* cfDomain(const PredForm*) const;
	FOPropTableDomain* exists(FOPropTableDomain*, const std::set<Variable*>&) const;
	FOPropTableDomain* forall(FOPropTableDomain*, const std::set<Variable*>&) const;
	FOPropTableDomain* conjunction(FOPropTableDomain*, FOPropTableDomain*) const;
	FOPropTableDomain* disjunction(FOPropTableDomain*, FOPropTableDomain*) const;
	FOPropTableDomain* substitute(FOPropTableDomain*, const std::map<Variable*, Variable*>&) const;
	bool equals(FOPropTableDomain*, FOPropTableDomain*) const;
	bool approxequals(FOPropTableDomain*, FOPropTableDomain*) const;
	PredInter* inter(const std::vector<Variable*>&, const ThreeValuedDomain<FOPropTableDomain>&, AbstractStructure*) const;
	std::ostream& put(std::ostream&, FOPropTableDomain*) const;
};




#endif //PROPAGATEDOMFACT_HPP
