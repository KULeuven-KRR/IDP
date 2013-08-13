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

#ifndef PROPAGATORCOMM_HPP_
#define PROPAGATORCOMM_HPP_

#include <map>
#include <memory>

class PredForm;
class Variable;
class FOBDDManager;
class FOPropBDDDomain;

/**
 * Direction of propagation
 *	UP = from a subformula to its parents
 *	DOWN = from a formula to its subformulas
 */
enum FOPropDirection {
	UP, DOWN
};

enum InitBoundType {
	IBT_TWOVAL, IBT_BOTH, IBT_CT, IBT_CF, IBT_NONE
};

/**
 * Struct to represent the connection between atomic formulas over the same symbol
 */
template<class DomainType>
struct LeafConnectData {
	PredForm* _connector;
	DomainType* _equalities;
	std::map<Variable*, Variable*> _leaftoconnector;
	std::map<Variable*, Variable*> _connectortoleaf;
};

template<class DomainType>
class AdmissibleBoundChecker {
public:
	virtual ~AdmissibleBoundChecker() {
	}
	virtual bool check(DomainType*, DomainType*) const = 0;
};

class LongestBranchChecker: public AdmissibleBoundChecker<FOPropBDDDomain> {
private:
	int _treshhold;
	std::shared_ptr<FOBDDManager> _manager;
public:
	LongestBranchChecker(std::shared_ptr<FOBDDManager> m, int th)
			: _treshhold(th), _manager(m) {
	}
	bool check(FOPropBDDDomain*, FOPropBDDDomain*) const;
};

#endif //PROPAGATORCOMM_HPP_
