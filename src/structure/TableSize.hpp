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

#ifndef TABLESIZE_HPP_
#define TABLESIZE_HPP_

#include <cstddef>
#include <iostream>
#include <cmath>
#include "common.hpp"

enum TableSizeType {
	TST_APPROXIMATED, TST_INFINITE, TST_EXACT
};

struct tablesize {
	TableSizeType _type;
	long long _size;

	// If tp==TST_INFINITE, the size is set to 0.
	tablesize(TableSizeType tp, long long sz)
			: _type(tp), _size(tp==TST_INFINITE?0:sz) {
	}
	tablesize()
			: _type(TST_EXACT), _size(0) { // Initial tablesize is empty
	}

	tablesize operator+(const tablesize& rhs) const;
	tablesize operator-(const tablesize& rhs) const;
	tablesize operator*(const tablesize& rhs) const;
	void operator*=(const tablesize& rhs);
	// Division operator executes a C++ int division, not returning a float
	tablesize operator/(const tablesize& rhs) const;

	//Compares tablesizes. NOTE: Approximate values are handled as if they were exact
	bool operator==(const tablesize& rhs) const;
	//Compares tablesizes. NOTE: Approximate values are handled as if they were exact
	bool operator!=(const tablesize& rhs) const;
	//Compares tablesizes. NOTE: Approximate values are handled as if they were exact
	bool operator<(const tablesize& rhs) const;
	//Compares tablesizes. NOTE: Approximate values are handled as if they were exact
	bool operator>(const tablesize& rhs) const;
	//Compares tablesizes. NOTE: Approximate values are handled as if they were exact
	bool operator<=(const tablesize& rhs) const;
	//Compares tablesizes. NOTE: Approximate values are handled as if they were exact
	bool operator>=(const tablesize& rhs) const;

	bool isInfinite() const{
		return _type==TST_INFINITE;
	}
};

double toDouble(const tablesize& val);



/**
 * Numerical operators
 */
template<typename Num>
tablesize operator*(Num lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)*rhs;
}
template<typename Num>
tablesize operator*(const tablesize& lhs, Num rhs){
	return rhs*lhs;
}
template<typename Num>
tablesize operator/(Num lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)/rhs;
}
template<typename Num>
tablesize operator/(const tablesize& lhs, Num rhs){
	return lhs/tablesize(TableSizeType::TST_EXACT, rhs);
}
template<typename Num>
tablesize operator-(const tablesize& lhs, Num rhs){
	return lhs - tablesize(TableSizeType::TST_EXACT, rhs);
}
template<typename Num>
tablesize operator-(Num lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)-rhs;
}
template<typename Num>
tablesize operator+(Num lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)+rhs;
}
template<typename Num>
tablesize operator+(const tablesize& lhs, Num rhs){
	return rhs+lhs;
}
template<typename Num>
Num& operator+=(Num& val, const tablesize& rhs){
	auto n= val+rhs;
	if(n.isInfinite()){
		val = getMaxElem<Num>();
	}else{
		val = n._size;
	}
	return val;
}
template<typename Num>
Num& operator*=(Num& val, const tablesize& rhs){
	auto n= val*rhs;
	if(n.isInfinite()){ //Voor unknown werkt dit niet goed
		val = getMaxElem<Num>();
	}else{
		val = n._size;
	}
	return val;
}

/**
 * Comparison operators
 */
template<typename Num>
bool operator== ( const tablesize& lhs, const Num& rhs){
	return lhs == tablesize(TableSizeType::TST_EXACT, rhs);
}
template<typename Num>
bool operator== ( const Num& lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs) == rhs;
}


template<typename Num>
bool operator!= ( const tablesize& lhs, const Num& rhs){
	return lhs != tablesize(TableSizeType::TST_EXACT, rhs);
}
template<typename Num>
bool operator!= ( const Num& lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs) != rhs;
}

template<typename Num>
bool operator< ( const tablesize& lhs, const Num& rhs){
	return lhs < tablesize(TableSizeType::TST_EXACT, rhs);
}
template<typename Num>
bool operator< ( const Num& lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs) < rhs;
}

template<typename Num>
bool operator> ( const tablesize& lhs, const Num& rhs){
	return lhs > tablesize(TableSizeType::TST_EXACT, rhs);
}
template<typename Num>
bool operator> ( const Num& lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs) > rhs;
}

template<typename Num>
bool operator<= ( const tablesize& lhs, const Num& rhs){
	return lhs <= tablesize(TableSizeType::TST_EXACT, rhs);
}
template<typename Num>
bool operator<= ( const Num& lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs) <= rhs;
}

template<typename Num>
bool operator>= ( const tablesize& lhs, const Num& rhs){
	return lhs >= tablesize(TableSizeType::TST_EXACT, rhs);
}
template<typename Num>
bool operator>= ( const Num& lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs) >= rhs;
}


template<>
std::ostream& print(std::ostream& stream, const tablesize& obj);

bool isFinite(const tablesize& tsize);

#endif /* TABLESIZE_HPP_ */
