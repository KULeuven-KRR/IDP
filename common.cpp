/************************************
	common.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <string>
#include <limits>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdlib.h>
using namespace std;

// Extreme integers
int MIN_INT = numeric_limits<int>::min();
int MAX_INT = numeric_limits<int>::max();

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
   if(n == 0) return "0";
   string s;
   int a = abs(n);
   while(a > 0) {
	  int temp = a % 10;
	  char c = '0' + temp;
	  s = c + s;
	  a = (a - temp) / 10;
   }
   if(n < 0) return ('-' + s);
   else return s;
}

// Convert string to integer 
// (returns 0 when the string is not an integer)
int stoi(const string& s) {
	int i = 0;
	unsigned int n = 0;
	bool inv = false;
	if(s.size() && s[0] == '-') { 
		inv = true;
		++n;
	}
	for( ; n < s.size(); ++n) {
		if(s[n] < '0' || s[n] > '9') return 0;
		i = i * 10 + (s[n] - '0');
	}
	if(inv) return -i;
	else return i;
}

// Return a string of n spaces
string tabstring(unsigned int n) {
	string tab;
	for(unsigned int m = 0; m < n; ++m) 
		tab = tab + ' ';
	return tab;
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
