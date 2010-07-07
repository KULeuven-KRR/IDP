/************************************
	data.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef DATA_H
#define DATA_H

#include <string>
#include <tr1/unordered_map>
#include <map>
using namespace std;
using namespace std::tr1;

typedef unordered_map<string,string*>	HMSSP;

/******************************************
	Shared pointers for domain elements
******************************************/

class DomainData {
	private:
		HMSSP	_sharedstrings;
	public:
		string*	stringpointer(const string&);
};

#endif
