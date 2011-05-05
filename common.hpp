/************************************
	common.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <ostream>
#include <vector>
#include "commontypes.hpp"

extern void notyetimplemented(const std::string&);

extern bool	isInt(double);					//!< true iff the given double is an integer
extern bool isInt(const std::string&);		//!< true iff the given string is an integer
extern bool isDouble(const std::string&);	//!< true iff the given string is a double

extern std::string	itos(int);					//!< convert int to string
extern std::string	dtos(double);				//!< convert double to string
extern int			stoi(const std::string&);	//!< convert string to int
extern double		stod(const std::string&);	//!< convert string to double

extern void	printtabs(std::ostream&,unsigned int tabs);	//!< write a given number of tabs

extern double applyAgg(AggFunction,const std::vector<double>& args);	//!< apply an aggregate function to arguments

extern CompType	invertct(CompType);	//!< Invert a comparison operator
extern CompType	negatect(CompType);	//!< Negate a comparison operator

extern std::string* StringPointer(const char* str);			//!< Returns a shared pointer to the given string
extern std::string* StringPointer(const std::string& str);	//!< Returns a shared pointer to the given string

#endif
