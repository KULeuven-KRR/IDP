/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FOBDAGGTERM_HPP_
#define FOBDAGGTERM_HPP_

#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddSetExpr.hpp"

class DomainElement;
class FOBDDManager;

class FOBDDAggTerm: public FOBDDTerm {
private:
	friend class FOBDDManager;

	AggFunction _agg;
	const FOBDDSetExpr* _setexpr;

	FOBDDAggTerm(AggFunction agg, const FOBDDSetExpr* setexpr)
			: _agg(agg), _setexpr(setexpr) {
	}

public:
	bool containsDeBruijnIndex(unsigned int i) const {
		return _setexpr->containsDeBruijnIndex(i);
	}

		AggFunction aggfunction() const {
		return _agg;
	}
	const FOBDDSetExpr* setexpr() const {
		return _setexpr;
	}

	virtual Sort* sort() const;


	void accept(FOBDDVisitor*) const;
	const FOBDDTerm* acceptchange(FOBDDVisitor*) const;
	virtual std::ostream& put(std::ostream& output) const;

};

#endif /* FOBDAGGTERM_HPP_ */
