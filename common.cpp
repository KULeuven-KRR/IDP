#include <iostream>
#include <ostream>
#include <limits>
#include <stdlib.h>
#include <sstream>

#include "common.hpp"
#include "IdpException.hpp"
#include "GlobalData.hpp"

using namespace std;

string getTablenameForInternals() {
	return "idp_intern";
}

string getPathOfLuaInternals() {
	stringstream ss;
	ss << DATADIR << INTERNALLIBARYLUA;
	return ss.str();
}
string getPathOfIdpInternals() {
	stringstream ss;
	ss << DATADIR << INTERNALLIBARYIDP;
	return ss.str();
}
string getPathOfConfigFile() {
	stringstream ss;
	ss << RCDIR << CONFIGFILENAME;
	return ss.str();
}

std::string tabs(){
	stringstream ss;
	unsigned int nb = GlobalData::instance()->getTabSize();
	for(unsigned int i=0; i<nb; ++i){
		ss <<"    ";
	}
	return ss.str();
}

void pushtab(){
	GlobalData::instance()->setTabSize(GlobalData::instance()->getTabSize()+1);
}
void poptab(){
	GlobalData::instance()->resetTabSize();
}

void notyetimplemented(const string& message) {
	cerr << "WARNING or ERROR: The following feature is not yet implemented:\n" << '\t' << message << '\n'
			<< "Please send an e-mail to krr@cs.kuleuven.be if you really need this feature.\n";
}

void thrownotyetimplemented(const string& message) {
	cerr << "WARNING or ERROR: The following feature is not yet implemented:\n" << '\t' << message << '\n'
			<< "Please send an e-mail to krr@cs.kuleuven.be if you really need this feature.\n";
	throw IdpException("Aborting because of not yet implemented feature.");
}

bool isInt(double d) {
	return (double(int(d)) == d);
}

bool isInt(const string& s) {
	stringstream i(s);
	int n;
	return (i >> n);
}

bool isDouble(const string& s) {
	stringstream i(s);
	double d;
	return (i >> d);
}

int toInt(const string& s) {
	stringstream i(s);
	int n;
	if (!(i >> n))
		return 0;
	else
		return n;
}

double toDouble(const string& s) {
	stringstream i(s);
	double d;
	if (!(i >> d))
		return 0;
	else
		return d;
}



double applyAgg(const AggFunction& agg, const vector<double>& args) {
	double d;
	switch (agg) {
	case AggFunction::CARD:
		d = double(args.size());
		break;
	case AggFunction::SUM:
		d = 0;
		for (size_t n = 0; n < args.size(); ++n) {
			d += args[n];
		}
		break;
	case AggFunction::PROD:
		d = 1;
		for (size_t n = 0; n < args.size(); ++n) {
			d = d * args[n];
		}
		break;
	case AggFunction::MIN:
		d = numeric_limits<double>::max();
		for (size_t n = 0; n < args.size(); ++n) {
			d = (d <= args[n] ? d : args[n]);
		}
		break;
	case AggFunction::MAX:
		d = - numeric_limits<double>::max();
		for (size_t n = 0; n < args.size(); ++n) {
			d = (d >= args[n] ? d : args[n]);
		}
		break;
	}
	return d;
}

// TODO remove when all are using gcc 4.5
bool operator==(CompType left, CompType right) {
	return (int) left == (int) right;
}
bool operator>(CompType left, CompType right) {
	return (int) left > (int) right;
}

bool operator<(CompType left, CompType right) {
	return not (left == right || left > right);
}



TsType reverseImplication(TsType type) {
	if (type == TsType::IMPL) {
		return TsType::RIMPL;
	}
	if (type == TsType::RIMPL) {
		return TsType::IMPL;
	}
	return type;
}

bool isPos(SIGN s) {
	return s == SIGN::POS;
}
bool isNeg(SIGN s) {
	return s == SIGN::NEG;
}

SIGN operator not(SIGN rhs) {
	return rhs == SIGN::POS ? SIGN::NEG : SIGN::POS;
}

SIGN operator~(SIGN rhs) {
	return not rhs;
}

QUANT operator not(QUANT t) {
	return t == QUANT::UNIV ? QUANT::EXIST : QUANT::UNIV;
}

Context operator not(Context t) {
	switch (t) {
	case Context::BOTH:
		return Context::BOTH;
	case Context::POSITIVE:
		return Context::NEGATIVE;
	case Context::NEGATIVE:
		return Context::POSITIVE;
	}
}
Context operator~(Context t) {
	return not t;
}

bool isConj(SIGN sign, bool conj) {
	return (sign == SIGN::POS && conj) || (sign == SIGN::NEG && ~conj);
}

/*********************
 Shared strings
 *********************/

#include <tr1/unordered_map>
typedef std::tr1::unordered_map<std::string, std::string*> MSSP;
class StringPointers {
private:
	MSSP _sharedstrings; //!< map a string to its shared pointer
public:
	~StringPointers();
	string* stringpointer(const std::string&); //!< get the shared pointer of a string
};

StringPointers::~StringPointers() {
	for (auto it = _sharedstrings.begin(); it != _sharedstrings.end(); ++it) {
		delete (it->second);
	}
}

string* StringPointers::stringpointer(const string& s) {
	MSSP::iterator it = _sharedstrings.find(s);
	if (it != _sharedstrings.end()) {
		return it->second;
	} else {
		string* sp = new string(s);
		_sharedstrings[s] = sp;
		return sp;
	}
}

StringPointers sharedstrings;

string* StringPointer(const char* str) {
	return sharedstrings.stringpointer(string(str));
}

string* StringPointer(const string& str) {
	return sharedstrings.stringpointer(str);
}

CompType invertComp(CompType comp) {
	switch (comp) {
	case CompType::EQ:
		return comp;
	case CompType::NEQ:
		return comp;
	case CompType::LT:
		return CompType::GT;
	case CompType::GT:
		return CompType::LT;
	case CompType::LEQ:
		return CompType::GEQ;
	case CompType::GEQ:
		return CompType::LEQ;
	}
}

CompType negateComp(CompType comp) {
	switch (comp) {
	case CompType::EQ:
		return CompType::NEQ;
	case CompType::NEQ:
		return CompType::EQ;
	case CompType::LT:
		return CompType::GEQ;
	case CompType::GT:
		return CompType::LEQ;
	case CompType::LEQ:
		return CompType::GT;
	case CompType::GEQ:
		return CompType::LT;
	}
}

ostream& operator<<(ostream& out, AggFunction aggtype) {
	switch (aggtype) {
	case AggFunction::CARD:
		out << "#";
		break;
	case AggFunction::PROD:
		out << "prod";
		break;
	case AggFunction::SUM:
		out << "sum";
		break;
	case AggFunction::MIN:
		out << "min";
		break;
	case AggFunction::MAX:
		out << "max";
		break;
	}
	return out;
}

ostream& operator<<(ostream& out, TsType tstype) {
	switch (tstype) {
	case TsType::RULE:
		out << "<-";
		break;
	case TsType::IMPL:
		out << "=>";
		break;
	case TsType::RIMPL:
		out << "<=";
		break;
	case TsType::EQ:
		out << "<=>";
		break;
	}
	return out;
}
