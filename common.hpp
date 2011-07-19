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
std::string	convertToString(T element) {
	std::stringstream ss;
	ss << element;
	return ss.str();
}
int		toInt(const std::string&);	//!< convert string to int
double	toDouble(const std::string&);	//!< convert string to double

void	printtabs(std::ostream&,unsigned int tabs);	//!< write a given number of tabs

double applyAgg(const AggFunction&,const std::vector<double>& args);	//!< apply an aggregate function to arguments

CompType invertct(CompType ct);
CompType invertcomp(CompType);	//!< Invert a comparison operator
CompType negatecomp(CompType);	//!< Negate a comparison operator

std::ostream& operator<<(std::ostream&, const AggFunction&);	//!< Put an aggregate type on the given output stream
std::ostream& operator<<(std::ostream&, const TsType&);		//!< Put a tseitin type on the given output stream
std::ostream& operator<<(std::ostream&, const CompType&);	//!< Put a comparator type on the given output stream

PosContext swapcontext(PosContext);	//!< Negate a context

std::string* StringPointer(const char* str);			//!< Returns a shared pointer to the given string
std::string* StringPointer(const std::string& str);	//!< Returns a shared pointer to the given string

#endif
