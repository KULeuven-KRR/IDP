/************************************
	data.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "options.hpp"
#include "builtin.hpp"
#include "namespace.hpp"

/******************************************
	Shared pointers for domain elements
******************************************/

string* DomainData::stringpointer(const string& s) {
	MSSP::iterator it = _sharedstrings.find(s);
	if(it != _sharedstrings.end()) return it->second;
	else {
		string* sp = new string(s);
		_sharedstrings[s] = sp;
		return sp;
	}
}

compound* DomainData::compoundpointer(Function* f, const vector<TypedElement>& vte) {
	MFMVTC::iterator it = _sharedcompounds.find(f);
	if(it != _sharedcompounds.end()) {
		MVTC::iterator jt = (it->second).find(vte);
		if(jt != (it->second).end()) return jt->second;
		else {
			compound* cp = new compound(f,vte);
			(it->second)[vte] = cp;
			return cp;
		}
	}
	else {
		compound* cp = new compound(f,vte);
		_sharedcompounds[f][vte] = cp;
		return cp;
	}
}

DomainData::~DomainData() {
	for(MSSP::iterator it = _sharedstrings.begin(); it != _sharedstrings.end(); ++it) {
		delete(it->second);
	}
	for(MFMVTC::iterator it = _sharedcompounds.begin(); it != _sharedcompounds.end(); ++it) {
		for(MVTC::iterator jt = (it->second).begin(); jt != (it->second).end(); ++jt) {
			delete(jt->second);
		}
	}
}

DomainData* DomainData::_instance = 0;

DomainData* DomainData::instance() {
	if(!_instance) _instance = new DomainData();
	return _instance;
}

string*		IDPointer(char* s)			{ return DomainData::instance()->stringpointer(string(s));						}
string*		IDPointer(const string& s)	{ return DomainData::instance()->stringpointer(s);								}
compound*	CPPointer(TypedElement e)	{ return DomainData::instance()->compoundpointer(0,vector<TypedElement>(1,e));	}
compound*	CPPointer(Function* f, const vector<TypedElement>& vte)	{ 
	return DomainData::instance()->compoundpointer(f,vte);
}

/******************
	Global data	
******************/

CLOptions		_cloptions;			// Options
