/************************************
	common.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef IDP_COMMON_H
#define IDP_COMMON_H

#include <string>
#include <ostream>

extern std::string	itos(int);					//!< convert int to string
extern std::string	dtos(double);				//!< convert double to string
extern int			stoi(const std::string&);	//!< convert string to int
extern double		stod(const std::string&);	//!< convert string to double

extern void	printtabs(std::ostream&,unsigned int tabs);	//!< write a given number of tabs

#ifdef OLD
#include <iostream>

#include <vector>
using namespace std;
struct compound;
struct TypedElement;
union Element;
class Function;
enum ElementType { ELINT, ELDOUBLE, ELSTRING, ELCOMPOUND };

/** Memory management **/
// The functions below implement shared pointers to strings and compound elements. All user defined strings should be created by a call to one of these functions. Similarly for all compound elements.
extern string*		IDPointer(char* s);			// return a 'shared' pointer to s;
extern string*		IDPointer(const string& s);	// return a 'shared' pointer to s;
extern compound*	CPPointer(TypedElement e);	// return a 'shared' pointer to compound 0(e);
extern compound*	CPPointer(Element e, ElementType t);	// return a 'shared' pointer to compound 0(e);
extern compound*	CPPointer(Function* f,const vector<TypedElement>& v);	// return a 'shared' pointer to compound f(v);

/** Extreme numbers **/
extern int MAX_INT;			// maximum integer value
extern int MIN_INT;			// minimum integer value
extern double MAX_DOUBLE;	// maximum double value
extern double MIN_DOUBLE;	// minimum double value

/** Number of characters **/
extern int nrOfChars();

/** Next tuple **/
// implements a 'mechanical counter' (or 'carry')
//		when called, the first element in tuple is incremented by 1
//		if now, that element is strictly lower than the first element in limits, stop and return true;
//		else, set the first element to 0 and increase the second element in tuple
//		if now, that element is strictly lower than the second element in limits, stop and return true;
//		etc.
//		if all elements are set to 0, return false.
//	NOTE: this is useful, e.g., when iterating over all values of a tuple of variables.
extern bool nexttuple(vector<unsigned int>& tuple, const vector<unsigned int>& limits);

/** Type checking **/
extern bool isInt(const string&);
extern bool isInt(double);
extern bool isChar(int);
extern bool isChar(double);
extern bool isDouble(const string&);

/** Return a string of n spaces **/
extern string tabstring(unsigned int n);

#endif
#endif
