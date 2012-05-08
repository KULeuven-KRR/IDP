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
	if(rhs._type==TableSizeType::TST_UNKNOWN || _type==TableSizeType::TST_UNKNOWN || rhs._type==TableSizeType::TST_INFINITE || _type==TableSizeType::TST_INFINITE){
		return rhs;
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, rhs._size+_size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, rhs._size+_size);
	}
}
tablesize tablesize::operator-(const tablesize& rhs) const{
	if(rhs._type==TableSizeType::TST_UNKNOWN || _type==TableSizeType::TST_UNKNOWN || rhs._type==TableSizeType::TST_INFINITE || _type==TableSizeType::TST_INFINITE){
		return rhs;
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, _size-rhs._size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, _size-rhs._size);
	}
}
tablesize tablesize::operator*(const tablesize& rhs) const{
	if(rhs._type==TableSizeType::TST_UNKNOWN || _type==TableSizeType::TST_UNKNOWN || rhs._type==TableSizeType::TST_INFINITE || _type==TableSizeType::TST_INFINITE){
		return rhs;
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, _size*rhs._size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, _size*rhs._size);
	}
}

tablesize operator*(int lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)+rhs;
}
tablesize operator*(const tablesize& lhs, int rhs){
	return rhs*lhs;
}

tablesize operator-(const tablesize& lhs, int rhs){
	return lhs - tablesize(TableSizeType::TST_EXACT, rhs);
}
tablesize operator-(int lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)-rhs;
}

tablesize operator+(int lhs, const tablesize& rhs){
	return tablesize(TableSizeType::TST_EXACT, lhs)+rhs;
}
tablesize operator+(const tablesize& lhs, int rhs){
	return rhs+lhs;
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
