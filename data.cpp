/************************************
	data.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "builtin.hpp"

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

shared_ptr<DomainData> _domaindata(new DomainData());

string*		IDPointer(char* s)			{ return _domaindata->stringpointer(string(s));						}
string*		IDPointer(const string& s)	{ return _domaindata->stringpointer(s);								}
compound*	CPPointer(TypedElement e)	{ return _domaindata->compoundpointer(0,vector<TypedElement>(1,e));	}

/****************************
	Built-in vocabularies
****************************/

shared_ptr<StdBuiltin> _stdbuiltin(new StdBuiltin());

Vocabulary*	stdbuiltin() { return _stdbuiltin.get();	}
