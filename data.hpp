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
		MSSP	_sharedstrings;		// map a string to its shared pointer
		MFMVTC	_sharedcompounds;	// map a compund to its shared pointer
	public:
		string*		stringpointer(const string&);							// get the shared pointer of a string
		compound*	compoundpointer(Function*,const vector<TypedElement>&);	// get the shared pointer of a compound
};

/****************************
	Built-in vocabularies
****************************/

class Vocabulary;
Vocabulary*	stdbuiltin();	// Returns the standard built-in vocabulary


#endif
