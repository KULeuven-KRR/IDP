/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "TableSize.hpp"

// FIXME check overflows (which result in going to INFINITY!)

tablesize tablesize::operator+(const tablesize& rhs) const{
	if(rhs.isInfinite() || isInfinite()){
		return rhs;
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, rhs._size+_size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, rhs._size+_size);
	}
}
tablesize tablesize::operator-(const tablesize& rhs) const{
	if(rhs.isInfinite() ){
		return rhs;
	}
	if(isInfinite()){
		return *this;
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, _size-rhs._size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, _size-rhs._size);
	}
}
tablesize tablesize::operator*(const tablesize& rhs) const{
	auto t = tablesize(_type, _size);
	t *= rhs;
	return t;
}
void tablesize::operator*=(const tablesize& rhs){
	if(rhs.isInfinite() || isInfinite()){
		return;
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		_type = TableSizeType::TST_APPROXIMATED;
		_size = _size*rhs._size;
	}else{
		_type = TableSizeType::TST_EXACT;
		_size = _size*rhs._size;
	}
}
tablesize tablesize::operator/(const tablesize& rhs) const{
	if(rhs._type==TableSizeType::TST_UNKNOWN|| _type==TableSizeType::TST_UNKNOWN || rhs._size==0){
		return tablesize(TST_UNKNOWN, 0);
	}
	if(rhs._type==TableSizeType::TST_INFINITE|| _type==TableSizeType::TST_INFINITE){
		return rhs;
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, _size/rhs._size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, _size/rhs._size);
	}
}

tablesize natlog(const tablesize& val){
	if(val.isInfinite()){
		return val;
	}
	return tablesize(val._type, log(val._size));
}

double toDouble(const tablesize& val){
	if(val.isInfinite()){
		return getMaxElem<double>();
	}
	return val._size;
}

template<>
std::string toString(const tablesize& obj){
	std::stringstream ss;
	if(obj._type!=TableSizeType::TST_EXACT){
		ss <<"not exactly known";
	}else{
		ss <<obj._size;
	}
	return ss.str();
}
