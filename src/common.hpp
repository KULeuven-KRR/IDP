/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <ostream>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include "commontypes.hpp"
#include <utility>
#include <unordered_map>
#include <memory>

#include "errorhandling/IdpException.hpp"

#include "loki/NullType.h"
#include "loki/TypeTraits.h"
#include "loki/static_check.h"
#include <typeinfo>

#include "Assert.hpp"

typedef unsigned int uint;

std::string getGlobalNamespaceName();
std::string getInternalNamespaceName();

void setInstallDirectoryPath(const std::string& dirpath);
std::string getInstallDirectoryPath();

std::string getPathOfLuaInternals();
std::string getPathOfIdpInternals();
std::string getPathOfConfigFile();

/*
    *** Printing stuff ***

    Refactored the toString method for efficiency:
    a print method returning a ToStream object
    a templated method operator<<(stream, ToStream<T>)
    with a default implementation using a put for objects and pointers (except when they are fundamental, then writing them directly).
    which can be overridden for specialized behavior without a put method.

    Advantages: no intermediate string construction + more logical naming

    a toString method is still available which does generate the string first but so should be used only when the string itself is important!
*/

template<bool isPointer, bool isFundamental, typename Type, typename Stream>
struct PutInStream {
	void operator()(const Type& object, Stream& ss) {
		if (object == NULL) {
			ss << "NULL";
		} else {
			ss << print(*object);
		}
	}
};

template<typename Type, typename Stream>
struct PutInStream<false, false, Type, Stream> {
	void operator()(const Type& object, Stream& ss) {
		object.put(ss);
	}
};

template<typename Type, typename Stream>
struct PutInStream<false, true, Type, Stream> {
	void operator()(const Type& object, Stream& ss) {
		ss << object;
	}
};

template<typename Type>
std::ostream& print(std::ostream& output, const Type& object) {
	PutInStream<Loki::TypeTraits<Type>::isPointer, Loki::TypeTraits<Type>::isFundamental, Type, std::ostream>()(object, output);
	return output;
}

template<class T>
class ToStream{
public:
	const T& object;

	ToStream(const T& object): object(object){

	}

	template<class T2> friend std::ostream& operator<<(std::ostream& stream, const ToStream<T2>& obj);
};

template<class T>
ToStream<const T&> print(const T& object){
	return ToStream<const T&>(object);
}

template<class T>
std::ostream& operator<<(std::ostream& stream, const ToStream<T>& object){
	return print(stream, object.object);
}

template<class T>
std::string toString(const T& object){
	std::stringstream ss;
	auto o = print(object);
	ss << o;
	return ss.str();
}

template<class Head, class Head2, class ... Args>
std::ostream& print(std::ostream& output, const Head& head, const Head2& head2, const Args&... args) {
	print(output, head);
	print(output, head2);
	print(output, args...);
	return output;
}
#define PRINTTOSTREAM(x) template<>\
	std::ostream& operator<<(std::ostream& output, const ToStream<const x&>& o);
#define PRINTTOSTREAMIMPL(x) template<>\
	std::ostream& operator<<(std::ostream& output, const ToStream<const x&>& o) {\
		return output <<o.object;\
	}

#define TEMPLPRINTTOSTREAM(x) template<class... Type>\
	std::ostream& operator<<(std::ostream& output, const ToStream<const x<Type...>&>& o) {\
		return output <<o.object;\
	}

template<class Type>
std::ostream& operator<<(std::ostream& output, const std::vector<Type>& v) {
	output << "(";
	for (auto obj = v.cbegin(); obj != v.cend();) {
		output << print(*obj);
		++obj;
		if (obj != v.cend()) {
			output << ", ";
		}
	}
	output << ")";
	return output;
}
TEMPLPRINTTOSTREAM(std::vector)

template<class Type>
std::ostream& operator<<(std::ostream& output, const std::shared_ptr<Type>& v) {
	if(v.get()==NULL){
		return "empty pointer";
	}
	return toString(*v);
}
TEMPLPRINTTOSTREAM(std::shared_ptr)

template<class... Type>
std::ostream& operator<<(std::ostream& output, const std::pair<Type...>&v) {
	output << "(" <<print(v.first) <<", " <<print(v.second) <<")";
	return output;
}
TEMPLPRINTTOSTREAM(std::pair)

PRINTTOSTREAM(std::string)
PRINTTOSTREAM(char)
PRINTTOSTREAM(char*)

template<typename... Type>
std::ostream& operator<<(std::ostream& output, const std::set<Type...>& v) {
	output << "{";
	for (auto obj = v.cbegin(); obj != v.cend();) {
		output << print(*obj);
		++obj;
		if (obj != v.cend()) {
			output << ", ";
		}
	}
	output << "}";
	return output;
}

TEMPLPRINTTOSTREAM(std::set)

template<typename... Type>
std::ostream& operator<<(std::ostream& output, const std::list<Type...>& v) {
	output << "{";
	for (auto obj = v.cbegin(); obj != v.cend();) {
		output << print(*obj);
		++obj;
		if (obj != v.cend()) {
			output << ", ";
		}
	}
	output << "}";
	return output;
}

TEMPLPRINTTOSTREAM(std::list)

template<typename Map>
std::ostream& printMap(std::ostream& output, const Map& map){
	output << "(";
	for (auto obj = map.cbegin(); obj != map.cend();) {
		output << print((*obj).first);
		output << "->";
		output << print((*obj).second);
		++obj;
		if (obj != map.cend()) {
			output << "; ";
		}
	}
	output << ")";
	return output;
}

template<typename... Type>
std::ostream& operator<<(std::ostream& output, const std::map<Type...>& map) {
	return printMap(output, map);
}
template<typename... Type>
std::ostream& operator<<(std::ostream& output, const std::unordered_map<Type...>& map) {
	return printMap(output, map);
}
TEMPLPRINTTOSTREAM(std::map)
TEMPLPRINTTOSTREAM(std::unordered_map)

std::ostream& operator<<(std::ostream& output, const CompType& type);
std::ostream& operator<<(std::ostream& output, const TruthType& type);
std::ostream& operator<<(std::ostream& output, const TsType& type);
std::ostream& operator<<(std::ostream& output, const AggFunction& type);
std::ostream& operator<<(std::ostream& output, const TruthValue& type);
PRINTTOSTREAM(CompType)
PRINTTOSTREAM(TruthType)
PRINTTOSTREAM(TsType)
PRINTTOSTREAM(AggFunction)
PRINTTOSTREAM(TruthValue)

/*#if __GNUC__ < 4 || \
              (__GNUC__ == 4 && __GNUC_MINOR__ < 6)

 template<typename Stream>
 Stream& operator<<(Stream& out, CompType ct) {
 out << print(ct);
 return out;
 }
 #endif*/

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
std::string nt();
void pushtab();
void poptab();

//void notyetimplemented(const std::string&);
IdpException notyetimplemented(const std::string&);

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

double applyAgg(const AggFunction&, const std::vector<double>& args); //!< apply an aggregate function to arguments

CompType invertComp(CompType); //!< Invert a comparison operator
CompType negateComp(CompType); //!< Negate a comparison operator

bool operator==(CompType left, CompType right);
bool operator>(CompType left, CompType right);
bool operator<(CompType left, CompType right);

bool operator==(VarId left, VarId right);
bool operator!=(VarId left, VarId right);
bool operator<(VarId left, VarId right);
bool operator==(DefId left, DefId right);
bool operator!=(DefId left, DefId right);
bool operator<(DefId left, DefId right);
bool operator==(SetId left, SetId right);
bool operator!=(SetId left, SetId right);
bool operator<(SetId left, SetId right);

template<typename NumberType, typename NumberType2>
bool compare(NumberType a, CompType comp, NumberType2 b) {
	switch (comp) {
	case CompType::EQ:
		return a == b;
	case CompType::NEQ:
		return a != b;
	case CompType::LEQ:
		return a <= b;
	case CompType::GEQ:
		return a >= b;
	case CompType::LT:
		return a < b;
	case CompType::GT:
		return a > b;
	}Assert(false);
	return true;
}

TsType invertImplication(TsType type);

bool isPos(SIGN s);
bool isNeg(SIGN s);

SIGN operator not(SIGN rhs);
SIGN operator~(SIGN rhs);

QUANT operator not(QUANT t);

Context operator not(Context t);
Context operator~(Context t);

TruthValue operator not(TruthValue t);
TruthValue operator~(TruthValue t);

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

std::ostream& operator<<(std::ostream& out, const VarId& id);
std::ostream& operator<<(std::ostream& out, const DefId& id);
std::ostream& operator<<(std::ostream& out, const SetId& id);

std::string* StringPointer(const char* str); //!< Returns a shared pointer to the given string
std::string* StringPointer(const std::string& str); //!< Returns a shared pointer to the given string

template<class T2, class T>
bool isa(const T& object) {
	LOKI_STATIC_CHECK(not Loki::TypeTraits<T>::isPointer, CannotCompareTypeIDofPointers);
	LOKI_STATIC_CHECK(not Loki::TypeTraits<T2>::isPointer, CannotCompareTypeIDofPointers);
	return (dynamic_cast<const T2*>(&object) != NULL);
}

#endif /* COMMON_HPP */
