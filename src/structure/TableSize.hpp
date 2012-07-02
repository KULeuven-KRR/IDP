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
#include <cmath>
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
	void operator*=(const tablesize& rhs);
	tablesize operator/(const tablesize& rhs) const;

	bool isInfinite() const{
		return _type==TST_INFINITE || _type==TST_UNKNOWN;
	}
};

tablesize natlog(const tablesize& val);
double toDouble(const tablesize& val);

template<class Num>
tablesize operator*(Num lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)*rhs;
}
template<class Num>
tablesize operator*(const tablesize& lhs, Num rhs){
	return rhs*lhs;
}
template<class Num>
tablesize operator/(Num lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)/rhs;
}
template<class Num>
tablesize operator/(const tablesize& lhs, Num rhs){
	return lhs/tablesize(TableSizeType::TST_EXACT, rhs);
}
template<class Num>
tablesize operator-(const tablesize& lhs, Num rhs){
	return lhs - tablesize(TableSizeType::TST_EXACT, rhs);
}
template<class Num>
tablesize operator-(Num lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)-rhs;
}
template<class Num>
tablesize operator+(Num lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)+rhs;
}
template<class Num>
tablesize operator+(const tablesize& lhs, Num rhs){
	return rhs+lhs;
}
template<class Num>
Num& operator+=(Num& val, const tablesize& rhs){
	auto n= val+rhs;
	if(n.isInfinite()){
		val = getMaxElem<Num>();
	}else{
		val = n._size;
	}
	return val;
}
template<class Num>
Num& operator*=(Num& val, const tablesize& rhs){
	auto n= val*rhs;
	if(n.isInfinite()){ //Voor unknown werkt dit niet goed
		val = getMaxElem<Num>();
	}else{
		val = n._size;
	}
	return val;
}

template<>
std::string toString(const tablesize& obj);

bool isFinite(const tablesize& tsize);

#endif /* TABLESIZE_HPP_ */
