/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef TABLESIZE_HPP_
#define TABLESIZE_HPP_

#include <cstddef>
#include <iostream>
#include "common.hpp"

enum TableSizeType {
	TST_APPROXIMATED, TST_INFINITE, TST_EXACT, TST_UNKNOWN
};

struct tablesize {
	TableSizeType _type;
	size_t _size;
	tablesize(TableSizeType tp, size_t sz)
			: _type(tp), _size(sz) {
	}
	tablesize()
			: _type(TST_UNKNOWN), _size(0) {
	}

	tablesize operator+(const tablesize& rhs) const;
	tablesize operator-(const tablesize& rhs) const;
	tablesize operator*(const tablesize& rhs) const;
};

tablesize operator*(int lhs, const tablesize& rhs);
tablesize operator*(const tablesize& lhs, int rhs);

tablesize operator-(const tablesize& lhs, int rhs);
tablesize operator-(int lhs, const tablesize& rhs);

tablesize operator+(int lhs, const tablesize& rhs);
tablesize operator+(const tablesize& lhs, int rhs);

template<>
std::string toString(const tablesize& obj);

bool isFinite(const tablesize& tsize);

#endif /* TABLESIZE_HPP_ */
