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

#include "fobdds/FoBddTerm.hpp"

class FOBDDManager;

class FOBDDDeBruijnIndex: public FOBDDTerm {
private:
	friend class FOBDDManager;

	Sort* _sort;
	unsigned int _index;

	FOBDDDeBruijnIndex(uint id, Sort* sort, unsigned int index)
			: FOBDDTerm(id), _sort(sort), _index(index) {
	}

public:
	bool containsDeBruijnIndex(unsigned int index) const {
		return _index == index;
	}

	Sort* sort() const {
		return _sort;
	}
	unsigned int index() const {
		return _index;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDTerm* acceptchange(FOBDDVisitor*) const;
	virtual std::ostream& put(std::ostream& output) const;

};

struct CompareBDDIndices {
	bool operator()(const FOBDDDeBruijnIndex* lhs, const FOBDDDeBruijnIndex* rhs) const;
};

typedef std::set<const FOBDDDeBruijnIndex*, CompareBDDIndices> fobddindexset;
