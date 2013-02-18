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

#ifndef FOBDSETEXPR_HPP_
#define FOBDSETEXPR_HPP_

#include <set>
#include <vector>
#include <iostream>
#include "vocabulary/vocabulary.hpp"

class Sort;
class FOBDD;
class FOBDDTerm;
class FOBDDVisitor;

class FOBDDSetExpr {
private:
	friend class FOBDDManager;
protected:
	Sort* _sort; //The sort of the expression (needs to be given when creating)

public:
	FOBDDSetExpr(Sort* sort)
			: _sort(sort) {
	}
	Sort* sort() const {
		return _sort;
	}





	virtual bool containsDeBruijnIndex(unsigned int i) const = 0;

	virtual void accept(FOBDDVisitor*) const = 0;
	virtual const FOBDDSetExpr* acceptchange(FOBDDVisitor*) const = 0;
	virtual std::ostream& put(std::ostream& output) const = 0;
};

class FOBDDQuantSetExpr: public FOBDDSetExpr {
private:
	friend class FOBDDManager;
	std::vector<Sort*> _quantvarsorts; //!< The sorts of the quantified variables of the set expression (in order of quantification (for the DeBruyn indices)
	const FOBDD* _subformula; //!< The direct subformulas of the set expression
	const FOBDDTerm* _subterm; //!< The direct subterms of the set expression
	FOBDDQuantSetExpr(Sort* sort)
			: FOBDDSetExpr(sort) {
	}
	FOBDDQuantSetExpr(const std::vector<Sort*>& sorts, const FOBDD* formula, const FOBDDTerm* term, Sort* sort)
			: FOBDDSetExpr(sort), _quantvarsorts (sorts), _subformula (formula),_subterm (term){
	}
public:
	void accept(FOBDDVisitor*) const;
	const FOBDDQuantSetExpr* acceptchange(FOBDDVisitor*) const;
	virtual std::ostream& put(std::ostream& output) const;
	const std::vector<Sort*>& quantvarsorts() const {
		return _quantvarsorts;
	}
	const FOBDD* subformula() const {
		return _subformula;
	}
	const FOBDDTerm* subterm() const {
		return _subterm;
	}
	virtual bool containsDeBruijnIndex(unsigned int i) const ;
};

class FOBDDEnumSetExpr: public FOBDDSetExpr {
private:
	friend class FOBDDManager;
	std::vector<const FOBDDQuantSetExpr* > _subsets;

	FOBDDEnumSetExpr(Sort* sort)
			: FOBDDSetExpr(sort) {
	}
	FOBDDEnumSetExpr(const std::vector<const FOBDDQuantSetExpr*>& subsets, Sort* sort)
			: FOBDDSetExpr(sort), _subsets(subsets) {
	}
public:
	int size() const {
		return _subsets.size();
	}
	const std::vector<const FOBDDQuantSetExpr* >& subsets() const{
		return _subsets;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDEnumSetExpr* acceptchange(FOBDDVisitor*) const;
	virtual std::ostream& put(std::ostream& output) const;
	virtual bool containsDeBruijnIndex(unsigned int i) const ;


};

#endif /* FOBDSETEXPR_HPP_ */
