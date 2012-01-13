/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "StringUtils.hpp"

#include <string>
#include <sstream>
#include <locale>
#include <functional>
#include <algorithm>
#include <iostream>

using namespace std;

bool startsWith(const string& s, const string& substring) {
	return s.substr(0, substring.size()) == substring;
}

// trim from start
string &ltrim(string &s) {
	s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
	return s;
}

// trim from end
string &rtrim(string &s) {
	s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
	return s;
}

// trim from both ends
string &trim(string &s) {
	return ltrim(rtrim(s));
}

string trim(const string& s) {
	auto temp = s;
	return ltrim(rtrim(temp));
}

string replaceAllIn(const string& text, const string& find, const string& replacement) {
	auto newtext = text;
	string::size_type position = newtext.find(find); // find first position
	while (position != string::npos) {
		newtext.replace(position, find.length(), replacement);
		position = newtext.find(find, position + 1);
	}
	return newtext;
}

string replaceAllAndTrimEachLine(const string& text, const string& find, const string& replacement) {
	stringstream ss;
	string::size_type prevpos = 0;
	string::size_type position = text.find("\n");
	while (position != string::npos) {
		if (prevpos < position - 1) { // Remove empty lines
			ss << trim(text.substr(prevpos, position - 1)) << "\n";
		}
		prevpos = position;
		position = text.find(find, position + 1);
	}
	ss << trim(text.substr(prevpos));
	return replaceAllIn(ss.str(), find, replacement);
}
