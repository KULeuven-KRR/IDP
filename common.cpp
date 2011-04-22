/************************************
	common.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <iostream>
#include <vector>
#include <limits>
#include <cassert>
#include <stdlib.h>
#include <string>
#include <sstream>

#include "commontypes.hpp"

using namespace std;

string itos(int n) {
	stringstream sst;
	sst << n;
	return sst.str();
}

int stoi(const string& s) {
	stringstream i(s);
	int n;
	if(!(i >> n)) return 0;
	else return n;
}

string dtos(double d) {
	stringstream s;
	s << d;
	return s.str();
}

double stod(const string& s) {
	stringstream i(s);
	double d;
	if(!(i >> d)) return 0;
	else return d;
}

bool isInt(double d) {
	return (double(int(d)) == d);
}

bool isInt(const string& s) {
	stringstream i(s);
	int n;
	return (i >> n);
}

bool isDouble(const string& s) {
	stringstream i(s);
	double d;
	return (i >> d);
}

void notyetimplemented(const string& message) {
	cerr << "ERROR: The following feature is not yet implemented:\n"
		 << '\t' << message << '\n'
		 << "Please send an e-mail to krr@cs.kuleuven.be if you really need this feature.\n";
}

double applyAgg(AggFunction agg, const vector<double>& args) {
	double d;
	switch(agg) {
		case AGG_CARD:
			d = double(args.size());
			break;
		case AGG_SUM:
			d = 0;
			for(unsigned int n = 0; n < args.size(); ++n) d += args[n];
			break;
		case AGG_PROD:
			d = 1;
			for(unsigned int n = 0; n < args.size(); ++n) d = d * args[n];
			break;
		case AGG_MIN:
			d = numeric_limits<double>::max();
			for(unsigned int n = 0; n < args.size(); ++n) d = (d <= args[n] ? d : args[n]);
			break;
		case AGG_MAX:
			d = numeric_limits<double>::min();
			for(unsigned int n = 0; n < args.size(); ++n) d = (d >= args[n] ? d : args[n]);
			break;
	}
	return d;
}

void printtabs(ostream& output, unsigned int tabs) {
	for(unsigned int n = 0; n < tabs; ++n) 
		output << ' ';
}

/*********************
	Shared strings
*********************/

#include <tr1/unordered_map>
typedef std::tr1::unordered_map<std::string,std::string*>	MSSP;
class StringPointers {
	private:
		MSSP	_sharedstrings;		//!< map a string to its shared pointer
	public:
		~StringPointers();
		string*	stringpointer(const std::string&);	//!< get the shared pointer of a string
};

StringPointers::~StringPointers() {
	for(MSSP::iterator it = _sharedstrings.begin(); it != _sharedstrings.end(); ++it) {
		delete(it->second);
	}
}

string* StringPointers::stringpointer(const string& s) {
	MSSP::iterator it = _sharedstrings.find(s);
	if(it != _sharedstrings.end()) return it->second;
	else {
		string* sp = new string(s);
		_sharedstrings[s] = sp;
		return sp;
	}
}

StringPointers sharedstrings;

string* StringPointer(const char* str) {
	return sharedstrings.stringpointer(string(str));
}

string* StringPointer(const string& str) {
	return sharedstrings.stringpointer(str);
}

CompType invertct(CompType ct) {
	switch(ct) {
		case CT_EQ: case CT_NEQ: return ct;
		case CT_LT: return CT_GT;
		case CT_GT: return CT_LT;
		case CT_LEQ: return CT_GEQ;
		case CT_GEQ: return CT_LEQ;
		default:
			assert(false);
			return CT_EQ;
	}
}

CompType negatect(CompType ct) {
	switch(ct) {
		case CT_EQ: return CT_NEQ;
		case CT_NEQ: return CT_EQ;
		case CT_LT: return CT_GEQ;
		case CT_GT: return CT_LEQ;
		case CT_LEQ: return CT_GT;
		case CT_GEQ: return CT_LT;
		default:
			assert(false);
			return CT_EQ;
	}
}

string AggTypeNames[5] = { "#", "sum", "prod", "min", "max" };
ostream& operator<<(ostream& out, const AggType& aggtype) {
	return out << AggTypeNames[aggtype];
}

string ElementTypeNames[4] = { "int", "double", "string", "compound" };
ostream& operator<<(ostream& out, const ElementType& elementtype) {
	return out << ElementTypeNames[elementtype];
}

string TsTypeNames[4] = { "<=>", "<-", "=>", "<=" };
ostream& operator<<(ostream& out, const TsType& tstype) {
	return out << TsTypeNames[tstype];
}







#ifdef OLD
#include <string>
#include <limits>
#include <sstream>
#include <stdlib.h>

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

// Convert string to integer 
// (returns 0 when the string is not an integer)
// Convert double to string 

// Convert string to double 
// (returns 0 if the input string is not a double)
// Test if something is a char, double or int

bool isChar(int n) {
	return (0 <= n && n < 10);
}

bool isChar(double d) {
	if(isInt(d)) return isChar(int(d));
	else return false;
}

#endif
