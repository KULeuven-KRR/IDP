/************************************
	data.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>
#include <map>
#include <tr1/unordered_map>
#include <tr1/memory>

#include "common.hpp"

typedef std::tr1::unordered_map<std::string,std::string*>	MSSP;
typedef std::map<std::vector<TypedElement>,compound*>		MVTC;
typedef std::map<Function*,MVTC>							MFMVTC;

class CLOptions;
extern CLOptions _cloptions;

/******************************************
	Shared pointers for domain elements
******************************************/

class DomainData {
	private:
		static DomainData*		_instance;
		MSSP					_sharedstrings;			// map a string to its shared pointer
		MFMVTC					_sharedcompounds;		// map a compound to its shared pointer
		std::vector<compound*>	_sharedintcompounds;
		DomainData(unsigned int n = 1001): _sharedintcompounds(n,0) { }
	public:
		static DomainData*	instance();
		~DomainData();
		std::string*	stringpointer(const std::string&);								// get the shared pointer of a string
		compound*		compoundpointer(Function*,const std::vector<TypedElement>&);	// get the shared pointer of a compound
};

#endif
