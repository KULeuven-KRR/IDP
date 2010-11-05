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
		~DomainData();
		string*		stringpointer(const string&);							// get the shared pointer of a string
		compound*	compoundpointer(Function*,const vector<TypedElement>&);	// get the shared pointer of a compound
};

/******************
	Global data
******************/

class StdBuiltin;
struct Options;
class Namespace;

extern StdBuiltin	_stdbuiltin;		// Standard built-in vocabularium
extern DomainData	_domaindata;		// Shared domain element pointers
extern Options		_options;			// Options
extern Namespace	_globalnamespace;	// The global namespace

#endif
