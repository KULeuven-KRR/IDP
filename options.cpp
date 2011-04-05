#include "error.hpp"
#include "options.hpp"

void InfOptions::set(const string& opt, const string& val, ParseInfo* pi) {
	if(InfOptions::isoption(opt)) {
		if(opt == "language") {
			if(val == "txt") _format=OF_TXT;
			else if(val == "idp") _format=OF_IDP;
			else Error::wrongformat(val,pi);
		}
		else if(opt == "modelformat") {
			if(val == "all") _modelformat=MF_ALL;
			else if(val == "twovalued") _modelformat=MF_TWOVAL;
			else if(val == "threevalued") _modelformat=MF_THREEVAL;
			else Error::wrongmodelformat(val,pi);
		}
		else Error::wrongvaluetype(opt,pi);
	}
	else Error::unknopt(opt,pi);
}

void InfOptions::set(const string& opt, double, ParseInfo* pi) {
	if(InfOptions::isoption(opt)) {
		Error::wrongvaluetype(opt,pi);
	}
	else Error::unknopt(opt,pi);
}

void InfOptions::set(const string& opt, bool val, ParseInfo* pi) {
	if(InfOptions::isoption(opt)) {
		if(opt == "printtypes") {
			_printtypes = val;
		}
		else if(opt == "trace") {
			_trace = val;
		}
		else Error::wrongvaluetype(opt,pi);
	}
	else Error::unknopt(opt,pi);
}

void InfOptions::set(const string& opt, int val, ParseInfo* pi) {
	if(InfOptions::isoption(opt)) {
		if(opt == "nrmodels") {
			if(val >= 0) {
				_nrmodels = val;
			}
			else Error::posintexpected(opt,pi);
		}
		else if(opt == "satverbosity") {
			if(val >= 0) {
				_satverbosity = val;
			}
			else Error::posintexpected(opt,pi);
		}
		else Error::wrongvaluetype(opt,pi);
	}
	else Error::unknopt(opt,pi);
}


