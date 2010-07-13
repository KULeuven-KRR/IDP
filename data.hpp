/************************************
	data.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef DATA_H
#define DATA_H

#include <string>
#include <tr1/unordered_map>
#include <tr1/memory>
#include <map>
#include "vocabulary.hpp"
using namespace std;
using namespace std::tr1;

typedef unordered_map<string,string*>			MSSP;
typedef map<vector<TypedElement>,compound*>		MVTC;
typedef map<Function*,MVTC>						MFMVTC;

/******************************************
	Shared pointers for domain elements
******************************************/

class DomainData {
	private:
		MSSP	_sharedstrings;
		MFMVTC	_sharedcompounds;
	public:
		string*		stringpointer(const string&);
		compound*	compoundpointer(Function*,const vector<TypedElement>&);
};

/****************************
	Built-in vocabularies
****************************/

class Vocabulary;
Vocabulary*	stdbuiltin();	// Returns the standard built-in vocabulary


#endif
