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

#include "TableSize.hpp"
#include "utils/ArithmeticUtils.hpp"

tablesize tablesize::operator+(const tablesize& rhs) const{
	if(rhs.isInfinite() || isInfinite()){
		return tablesize(TST_INFINITE, 0);
	}

	// bool 'safe' indicates whether numerical operation was executed safely
	bool safe = true;
	long long result = 0;
	addition(_size,rhs._size,result,safe);
	if(safe){
		if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
			return tablesize(TableSizeType::TST_APPROXIMATED, result);
		}else{
			return tablesize(TableSizeType::TST_EXACT, result);
		}
	}else{
		return tablesize(TableSizeType::TST_INFINITE,0);
	}
}
tablesize tablesize::operator-(const tablesize& rhs) const{
	if(rhs.isInfinite()){
		throw IdpException("Trying to subtract with an infinite tablesize");
	}

	if(isInfinite()){
		return *this;
	}
	if(_size < rhs._size){
		throw IdpException("Trying to subtract with a bigger tablesize from a smaller one");
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
		_type = TableSizeType::TST_INFINITE;
		return;
	}

	// bool 'safe' indicates whether numerical operation was executed safely
	bool safe = true;
	long long result = 0;
	multiplication(_size,rhs._size,result,safe);
	if(safe) {
		if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
			_type = TableSizeType::TST_APPROXIMATED;
			_size = result;
		}else{
			_type = TableSizeType::TST_EXACT;
			_size = result;
		}
	}else{
		_type = TableSizeType::TST_INFINITE;
		return;
	}
}
tablesize tablesize::operator/(const tablesize& rhs) const{
	Assert(rhs._size!=0);
	if(isInfinite()){
		Assert(not rhs.isInfinite());
		return tablesize(TST_INFINITE, 0);
	}
	if(rhs.isInfinite()){
		return tablesize(TST_APPROXIMATED,0);
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, _size/rhs._size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, _size/rhs._size);
	}
}

double toDouble(const tablesize& val){
	if(val.isInfinite()){
		return getMaxElem<double>();
	}
	return val._size;
}


bool tablesize::operator==(const tablesize& rhs) const{
	if (isInfinite() && rhs.isInfinite()){
		return true; // infinite things are always equal
	}
	if (isInfinite() || rhs.isInfinite()){
		return false; // One of the two is infinite -> false
	}
	// Comparing approx/exact types: Let the stored _size handle it
	return (_size == rhs._size);
//	return (not (this < rhs) &&  not (rhs < this));
}

bool tablesize::operator!=(const tablesize& rhs) const{
	return not (*this==rhs);
}

bool tablesize::operator<(const tablesize& rhs) const{
	if(isInfinite()){
		return false;
	}
	if(rhs.isInfinite()){
		return true;
	}
	// Comparing approx/exact types: Let the stored _size handle it
	return _size < rhs._size;
}

bool tablesize::operator>(const tablesize& rhs) const{
	return rhs<*this;
}

bool tablesize::operator<=(const tablesize& rhs) const{
	return not (rhs<*this);
}

bool tablesize::operator>=(const tablesize& rhs) const{
	return not (*this<rhs);
}

template<>
std::ostream& print(std::ostream& stream, const tablesize& obj){
	switch(obj._type){
	case TableSizeType::TST_APPROXIMATED:
		stream <<"approximated to " <<obj._size;
		break;
	case TableSizeType::TST_EXACT:
		stream <<obj._size;
		break;
	case TableSizeType::TST_INFINITE:
		stream <<"infinite";
		break;
	}
	return stream;
}
