/************************************
	common.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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
			d = MAX_DOUBLE;
			for(unsigned int n = 0; n < args.size(); ++n) d = (d <= args[n] ? d : args[n]);
			break;
		case AGG_MAX:
			d = MIN_DOUBLE;
			for(unsigned int n = 0; n < args.size(); ++n) d = (d >= args[n] ? d : args[n]);
			break;
	}
	return d;
}


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

// Return a string of n spaces
string tabstring(unsigned int n) {
	string tab;
	for(unsigned int m = 0; m < n; ++m) 
		tab = tab + ' ';
	return tab;
}
