/************************************
	common.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <limits>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <cassert>
#include "commontypes.hpp"

using namespace std;

// Extreme numbers
int		MIN_INT		= numeric_limits<int>::min();
int		MAX_INT		= numeric_limits<int>::max();
double	MIN_DOUBLE	= numeric_limits<double>::min(); 
double	MAX_DOUBLE	= numeric_limits<double>::max(); 

// Number of chars
int nrOfChars() {
	return numeric_limits<char>::max() - numeric_limits<char>::min();
}

// Next tuple
bool nexttuple(vector<unsigned int>& tuple, const vector<unsigned int>& limits) {
	for(unsigned int n = 0; n < tuple.size(); ++n) {
		++tuple[n]; 
		if(tuple[n] == limits[n]) tuple[n] = 0;
		else return true;
	}
	return false;
}

// Convert integer to string
string itos(int n) {
	stringstream sst;
	sst << n;
	return sst.str();
}

// Convert string to integer 
// (returns 0 when the string is not an integer)
int stoi(const string& s) {
	stringstream i(s);
	int n;
	if(!(i >> n)) return 0;
	else return n;
}

// Convert double to string 
string dtos(double d) {
	stringstream s;
	s << d;
	return s.str();
}

// Convert string to double 
// (returns 0 if the input string is not a double)
double stod(const string& s) {
	stringstream i(s);
	double d;
	if(!(i >> d)) return 0;
	else return d;
}

// Test if something is a char, double or int

bool isDouble(const string& s) {
	stringstream i(s);
	double d;
	return (i >> d);
}

bool isInt(const string& s) {
	stringstream i(s);
	int n;
	return (i >> n);
}

bool isInt(double d) {
	return (double(int(d)) == d);
}

bool isChar(int n) {
	return (0 <= n && n < 10);
}

bool isChar(double d) {
	if(isInt(d)) return isChar(int(d));
	else return false;
}

// String of n spaces
string tabstring(unsigned int n) {
	string tab;
	for(unsigned int m = 0; m < n; ++m) 
		tab = tab + ' ';
	return tab;
}

// Invert comparison type
CompType invertcomp(const CompType& ct) {
	switch(ct) {
		case CT_EQ: return CT_EQ;
		case CT_NEQ: return CT_NEQ;
		case CT_LEQ: return CT_GEQ;
		case CT_GEQ: return CT_LEQ;
		case CT_LT: return CT_GT;
		case CT_GT: return CT_LT;
		default: assert(false);
	}
}

// Put enum types on output stream
ostream& operator<<(ostream& out, const AggType& aggtype) {
	string AggTypeStrings[5] = { "#", "sum", "prod", "min", "max" };
	return out << AggTypeStrings[aggtype];
}

ostream& operator<<(ostream& out, const ElementType& elementtype) {
	string ElementTypeStrings[4] = { "int", "double", "string", "compound" };
	return out << ElementTypeStrings[elementtype];
}

ostream& operator<<(ostream& out, const TsType& tstype) {
	string TsTypeStrings[4] = { "<=>", "<-", "=>", "<=" };
	return out << TsTypeStrings[tstype];
}

ostream& operator<<(ostream& out, const CompType& comp) {
	string CompTypeStrings[6] = { "=", "~=", "=<", ">=", "<", ">" };
	return out << CompTypeStrings[comp];
}
