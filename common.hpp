/************************************
	common.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <iostream>
#include <vector>
#include <string>
using namespace std;

/** Memory management **/
struct compound;
struct TypedElement;

extern string*		IDPointer(char* s);			// return a 'shared' pointer to s;
extern string*		IDPointer(const string& s);	// return a 'shared' pointer to s;
extern compound*	CPPointer(TypedElement e);	// return a 'shared' pointer to compound 0(e);

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

/** Conversions **/
extern string	itos(int);				// int to string
extern string	dtos(double);			// double to string
extern int		stoi(const string&);	// string to int
extern double	stod(const string&);	// string to double

/** Type checking **/
extern bool isInt(const string&);
extern bool isInt(double);
extern bool isChar(int);
extern bool isChar(double);
extern bool isDouble(const string&);

/** Return a string of n spaces **/
extern string tabstring(unsigned int n);
