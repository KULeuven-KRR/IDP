/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FOBDSETEXPR_HPP_
#define FOBDSETEXPR_HPP_

#include <set>
#include <vector>
#include <iostream>
#include "common.hpp"


class Sort;
class FOBDD;
class FOBDDTerm;
class FOBDDVisitor;

class FOBDDSetExpr {
private:
	friend class FOBDDManager;
protected:
	std::vector<Sort*> _quantvarsorts; //!< The sorts of the quantified variables of the set expression (in order of quantification (for the DeBruyn indices)
	std::vector<FOBDD*> _subformulas; //!< The direct subformulas of the set expression
	std::vector<FOBDDTerm*> _subterms; //!< The direct subterms of the set expression
	Sort* _sort; //The sort of the expression (needs to be given when creating)

public:
	FOBDDSetExpr(Sort* sort)
			: _sort(sort) {
	}
	Sort* sort() const {
		return _sort;
	}

	int size() const{
		Assert(_subformulas.size()==_subterms.size());
		return _subformulas.size();
	}
	const FOBDD* subformula(int i) const{
		Assert(i<_subformulas.size());
		return _subformulas[i];
	}
	const FOBDDTerm* subterm(int i) const{
			Assert(i<_subterms.size());
			return _subterms[i];
		}
	bool containsDeBruijnIndex(unsigned int i) const;

	virtual void accept(FOBDDVisitor*) const = 0;
	virtual const FOBDDSetExpr* acceptchange(FOBDDVisitor*) const = 0;
	virtual std::ostream& put(std::ostream& output) const;
};

class FOBDDQuantSetExpr: public FOBDDSetExpr {
private:
	friend class FOBDDManager;

public:
	FOBDDQuantSetExpr( Sort* sort)
			: FOBDDSetExpr(sort) {
	}
	FOBDDQuantSetExpr(const std::vector<Sort*>& sorts, FOBDD* formula, FOBDDTerm* term,  Sort* sort)
			: FOBDDSetExpr(sort) {
		_quantvarsorts = sorts;
		_subformulas = {formula};
		_subterms = {term};
	}
	void accept(FOBDDVisitor*) const ;
	const FOBDDSetExpr* acceptchange(FOBDDVisitor*) const;
};

class FOBDDEnumSetExpr: public FOBDDSetExpr {
private:
	friend class FOBDDManager;

public:
	FOBDDEnumSetExpr( Sort* sort)
			: FOBDDSetExpr(sort) {
	}
	FOBDDEnumSetExpr(const std::vector<FOBDD*>& s, const std::vector<FOBDDTerm*>& t,  Sort* sort)
			: FOBDDSetExpr(sort) {
		_subformulas = s;
		_subterms = t;
	}

	void accept(FOBDDVisitor*) const ;
	const FOBDDSetExpr* acceptchange(FOBDDVisitor*) const;

};

#endif /* FOBDSETEXPR_HPP_ */
