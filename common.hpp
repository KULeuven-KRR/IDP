/************************************
	common.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <ostream>
#include <sstream>
#include <vector>
#include "commontypes.hpp"

std::string getLibraryName();
std::string getLuaLibraryFilename();
std::string getIDPLibraryFilename();
std::string getConfigFilename();

void notyetimplemented(const std::string&);

bool isInt(double);					//!< true iff the given double is an integer
bool isInt(const std::string&);		//!< true iff the given string is an integer
bool isDouble(const std::string&);	//!< true iff the given string is a double

template<typename T>
std::string	toString(T element){
	std::stringstream ss;
	ss << element;
	return ss.str();
}
int			toInt(const std::string&);	//!< convert string to int
double		toDouble(const std::string&);	//!< convert string to double

void	printtabs(std::ostream&,unsigned int tabs);	//!< write a given number of tabs

double applyAgg(const AggFunction&,const std::vector<double>& args);	//!< apply an aggregate function to arguments

CompType invertct(CompType ct);
CompType invertcomp(CompType);	//!< Invert a comparison operator
CompType negatecomp(CompType);	//!< Negate a comparison operator

TsType reverseImplication(TsType type);

// Negate a context
Context swapcontext(Context ct);

bool isPos(SIGN s);
bool isNeg(SIGN s);

SIGN operator not(SIGN rhs);

SIGN operator~(SIGN rhs);

QUANT operator not (QUANT t);

template<class Stream>
Stream& operator<<(Stream& out, AggFunction aggtype) {
	switch(aggtype) {
		case AggFunction::CARD: out << "#"; break;
		case AggFunction::SUM:	out << "sum"; break;
		case AggFunction::PROD:	out << "prod"; break;
		case AggFunction::MIN:	out << "min"; break;
		case AggFunction::MAX:	out << "max"; break;
	}
	return out;
}
template<class Stream>
Stream& operator<<(Stream& out, TsType tstype) {
	switch(tstype) {
		case TsType::EQ: out << "<=>"; break;
		case TsType::IMPL:	out << "=>"; break;
		case TsType::RULE:	out << "<-"; break;
		case TsType::RIMPL:	out << "<="; break;
	}
	return out;
}

template<class Stream>
Stream& operator<<(Stream& output, CompType type){
	switch(type) {
		case CompType::EQ: output << " = "; break;
		case CompType::NEQ: output << " ~= "; break;
		case CompType::LT: output << " < "; break;
		case CompType::GT: output << " > "; break;
		case CompType::LEQ: output << " =< "; break;
		case CompType::GEQ: output << " >= "; break;
	}
	return output;
}

std::string* StringPointer(const char* str);			//!< Returns a shared pointer to the given string
std::string* StringPointer(const std::string& str);	//!< Returns a shared pointer to the given string


/*#include "loki/NullType.h"
#include "loki/TypeTraits.h"
#include "loki/static_check.h"
#include <typeinfo>

class CannotCompareTypeIDofPointers{};

template<class T, class T2>
bool safetypeid(const T& object){
	LOKI_STATIC_CHECK(not Loki::TypeTraits<T>::isPointer, InvalidTypeIDCheck);
	LOKI_STATIC_CHECK(not Loki::TypeTraits<T2>::isPointer, InvalidTypeIDCheck);
	return typeid(object)==typeid(T2);
}*/

#endif
