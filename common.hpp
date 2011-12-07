#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <ostream>
#include <iostream>
#include <sstream>
#include <vector>
#include "commontypes.hpp"
#include <utility>

#include "IdpException.hpp"

#include "loki/NullType.h"
#include "loki/TypeTraits.h"
#include "loki/static_check.h"
#include <typeinfo>

class AssertionException: public std::exception{

};

#ifdef DEBUG
#define Assert(condition) { if(!(condition)){ std::cerr << "ASSERT FAILED: " << #condition << " @ " << __FILE__ << " (" << __LINE__ << ")" << std::endl; throw AssertionException();} }
#else
#define Assert(x) do {} while(0)
#endif //DEBUG


std::string getTablenameForInternals();
std::string getPathOfLuaInternals();
std::string getPathOfIdpInternals();
std::string getPathOfConfigFile();

template<bool condition, typename Type, typename Stream>
struct PutInStream{
	void operator() (const Type& object, Stream& ss) {
		object->put(ss);
	}
};


template<typename Type, typename Stream>
struct PutInStream<false, Type, Stream>{
	void operator() (const Type& object, Stream& ss) {
		object.put(ss);
	}
};

template<typename Type>
std::string toString(const Type& object) {
	std::stringstream ss;
	PutInStream<Loki::TypeTraits<Type>::isPointer, Type, std::stringstream>()(object, ss);
	return ss.str();
}

/**
 * HOW TO PRINT INFORMATION CONSISTENCTLY!
 *
 * Consider a put (parent) which calls a put on another object (child)
 * The parent decides in which portion of the screen the child can print and puts the cursor at the start of that section.
 * When the child starts, it is at the correct location.
 * When the child newlines, it has to tab until the correct location.
 * The parent cannot know whether the child newlines at the end or not, so CONVENTION: the child never ends with a newline (allows compact printing).
 *
 * So in brief, to implement a put method:
 * 		before your first newline, do NOT put tabs().
 * 		after each newline you put yourself, put tabs().
 * 		when you have done printing, the cursor should be at the END of the last INFORMATION you printed. No newlines or tabs!
 */
std::string tabs();
void pushtab();
void poptab();

void notyetimplemented(const std::string&);
// Guaranteed to throw (but cannot tell this to gcc unfortunately)
// Counts as a TODO (but cannot tell this to eclipse unfortunately)
void thrownotyetimplemented(const std::string&);

bool isInt(double); //!< true iff the given double is an integer
bool isInt(const std::string&); //!< true iff the given string is an integer
bool isDouble(const std::string&); //!< true iff the given string is a double

template<typename T>
std::string convertToString(T element) {
	std::stringstream ss;
	ss << element;
	return ss.str();
}
int toInt(const std::string&); //!< convert string to int
double toDouble(const std::string&); //!< convert string to double

void printTabs(std::ostream&, unsigned int tabs); //!< write a given number of tabs

double applyAgg(const AggFunction&, const std::vector<double>& args); //!< apply an aggregate function to arguments

CompType invertComp(CompType); //!< Invert a comparison operator
CompType negateComp(CompType); //!< Negate a comparison operator
bool operator==(CompType left, CompType right);
bool operator>(CompType left, CompType right);
bool operator<(CompType left, CompType right);
template<typename NumberType, typename NumberType2>
bool compare(NumberType a, CompType comp, NumberType2 b){
	switch(comp){
	case CompType::EQ:
		return a == b;
	case CompType::NEQ:
		return a != b;
	case CompType::LEQ:
		return a <= b;
	case CompType::GEQ:
		return a >= b;
	case CompType::LT:
		return a<b;
	case CompType::GT:
		return a>b;
	}
	Assert(false);
	return true;
}


TsType reverseImplication(TsType type);

bool isPos(SIGN s);
bool isNeg(SIGN s);

SIGN operator not(SIGN rhs);
SIGN operator~(SIGN rhs);

QUANT operator not(QUANT t);

Context operator not(Context t);
Context operator~(Context t);

bool isConj(SIGN sign, bool conj);

template<class Stream>
Stream& operator<<(Stream& out, const AggFunction& aggtype) {
	switch (aggtype) {
	case AggFunction::CARD:
		out << "#";
		break;
	case AggFunction::SUM:
		out << "sum";
		break;
	case AggFunction::PROD:
		out << "prod";
		break;
	case AggFunction::MIN:
		out << "min";
		break;
	case AggFunction::MAX:
		out << "max";
		break;
	}
	return out;
}
template<class Stream>
Stream& operator<<(Stream& out, const TsType& tstype) {
	switch (tstype) {
	case TsType::EQ:
		out << "<=>";
		break;
	case TsType::IMPL:
		out << "=>";
		break;
	case TsType::RULE:
		out << "<-";
		break;
	case TsType::RIMPL:
		out << "<=";
		break;
	}
	return out;
}

template<class Stream>
Stream& operator<<(Stream& output, const CompType& type) {
	switch (type) {
	case CompType::EQ:
		output << " = ";
		break;
	case CompType::NEQ:
		output << " ~= ";
		break;
	case CompType::LT:
		output << " < ";
		break;
	case CompType::GT:
		output << " > ";
		break;
	case CompType::LEQ:
		output << " =< ";
		break;
	case CompType::GEQ:
		output << " >= ";
		break;
	}
	return output;
}

std::string* StringPointer(const char* str); //!< Returns a shared pointer to the given string
std::string* StringPointer(const std::string& str); //!< Returns a shared pointer to the given string

template<class T2, class T>
bool sametypeid(const T& object) {
	LOKI_STATIC_CHECK(not Loki::TypeTraits<T>::isPointer, CannotCompareTypeIDofPointers);
	LOKI_STATIC_CHECK(not Loki::TypeTraits<T2>::isPointer, CannotCompareTypeIDofPointers);
	return typeid(object) == typeid(T2);
}

#endif //COMMON_HPP

