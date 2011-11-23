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
#include <utility>

#include "loki/NullType.h"
#include "loki/TypeTraits.h"
#include "loki/static_check.h"
#include <typeinfo>

std::string getTablenameForInternals();
std::string getPathOfLuaInternals();
std::string getPathOfIdpInternals();
std::string getPathOfConfigFile();

void notyetimplemented(const std::string&);

// Guaranteed to throw (but cannot tell this to gcc unfortunately)
// Counts as a TODO (but cannot tell this to eclipse unfortunately)
void thrownotyetimplemented(const std::string&);

bool isInt(double);					//!< true iff the given double is an integer
bool isInt(const std::string&);		//!< true iff the given string is an integer
bool isDouble(const std::string&);	//!< true iff the given string is a double

template<typename T>
std::string	convertToString(T element) {
	std::stringstream ss;
	ss << element;
	return ss.str();
}
int		toInt(const std::string&);	//!< convert string to int
double	toDouble(const std::string&);	//!< convert string to double

void	printTabs(std::ostream&,unsigned int tabs);	//!< write a given number of tabs

double 	applyAgg(const AggFunction&,const std::vector<double>& args);	//!< apply an aggregate function to arguments

CompType invertComp(CompType);	//!< Invert a comparison operator
CompType negateComp(CompType);	//!< Negate a comparison operator
bool operator==(CompType left, CompType right);
bool operator>(CompType left, CompType right);
bool operator<(CompType left, CompType right);

TsType reverseImplication(TsType type);

bool isPos(SIGN s);
bool isNeg(SIGN s);

SIGN operator not(SIGN rhs);
SIGN operator~(SIGN rhs);

QUANT operator not (QUANT t);

Context operator not (Context t);
Context operator~(Context t);

bool isConj(SIGN sign, bool conj);

template<class Stream>
Stream& operator<<(Stream& out, const AggFunction& aggtype) {
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
Stream& operator<<(Stream& out, const TsType& tstype) {
	switch(tstype) {
		case TsType::EQ: out << "<=>"; break;
		case TsType::IMPL:	out << "=>"; break;
		case TsType::RULE:	out << "<-"; break;
		case TsType::RIMPL:	out << "<="; break;
	}
	return out;
}

template<class Stream>
Stream& operator<<(Stream& output, const CompType& type){
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

std::string* StringPointer(const char* str);		//!< Returns a shared pointer to the given string
std::string* StringPointer(const std::string& str);	//!< Returns a shared pointer to the given string


class CannotCompareTypeIDofPointers{};

template<class T2, class T>
bool sametypeid(const T& object){
	LOKI_STATIC_CHECK(not Loki::TypeTraits<T>::isPointer, InvalidTypeIDCheck);
	LOKI_STATIC_CHECK(not Loki::TypeTraits<T2>::isPointer, InvalidTypeIDCheck);
	return typeid(object)==typeid(T2);
}

#endif
