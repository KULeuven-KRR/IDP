/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <iostream>
#include <ostream>
#include <limits>
#include <stdlib.h>
#include <sstream>

#include "common.hpp"
#include "GlobalData.hpp"

using namespace std;

std::string getGlobalNamespaceName() {
	return "idpglobal"; // NOTE: this string is also used in idp_intern.idp
}
std::string getInternalNamespaceName() {
	return "idpintern";
}

std::string installdirectorypath = string(IDPINSTALLDIR);
void setInstallDirectoryPath(const std::string& dirpath) {
	installdirectorypath = dirpath;
}
std::string getInstallDirectoryPath() {
	return installdirectorypath;
}

string getPathOfLuaInternals() {
	stringstream ss;
	ss << getInstallDirectoryPath() << INTERNALLIBARYLUA;
	return ss.str();
}
string getPathOfIdpInternals() {
	stringstream ss;
	ss << getInstallDirectoryPath() << INTERNALLIBARYIDP;
	return ss.str();
}
string getPathOfConfigFile() {
	stringstream ss;
	ss << getInstallDirectoryPath() << CONFIGFILENAME;
	return ss.str();
}

std::string tabs() {
	stringstream ss;
	auto nb = GlobalData::instance()->getTabSize();
	for (size_t i = 0; i < nb; ++i) {
		ss << "    ";
	}
	return ss.str();
}

std::string nt() {
	stringstream ss;
	ss << '\n' << tabs();
	return ss.str();
}

void pushtab() {
	GlobalData::instance()->setTabSize(GlobalData::instance()->getTabSize() + 1);
}
void poptab() {
	GlobalData::instance()->resetTabSize();
}

/*void notyetimplemented(const string& message) {
 clog << "WARNING or ERROR: The following feature is not yet implemented:\n" << '\t' << message << '\n'
 << "Please send an e-mail to krr@cs.kuleuven.be if you really need this feature.\n";
 }*/

IdpException notyetimplemented(const string& message) {
	clog << "\n";
	clog << tabs() << "WARNING or ERROR: The following feature is not yet implemented:\n";
	clog << tabs() << '\t' << message << '\n';
	clog << tabs() << "Please send an e-mail to krr@cs.kuleuven.be if you really need this feature.\n";
	return IdpException("Aborting because of not yet implemented feature.");
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
	if (not (i >> n)) {
		return 0;
	} else {
		return n;
	}
}

double toDouble(const string& s) {
	stringstream i(s);
	double d;
	if (not (i >> d)) {
		return 0;
	} else {
		return d;
	}
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
		d = getMaxElem<double>();
		for (size_t n = 0; n < args.size(); ++n) {
			d = (d <= args[n] ? d : args[n]);
		}
		break;
	case AggFunction::MAX:
		d = getMinElem<double>();
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

TsType invertImplication(TsType type) {
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
	Context result;
	switch (t) {
	case Context::BOTH:
		result = Context::BOTH;
		break;
	case Context::POSITIVE:
		result = Context::NEGATIVE;
		break;
	case Context::NEGATIVE:
		result = Context::POSITIVE;
		break;
	}
	return result;
}
Context operator~(Context t) {
	return not t;
}

bool isConj(SIGN sign, bool conj) {
	return (sign == SIGN::POS && conj) || (sign == SIGN::NEG && ~conj);
}

template<>
std::string toString(const CompType& type) {
	std::stringstream output;
	switch (type) {
	case CompType::EQ:
		output << " = ";
		break;
	case CompType::NEQ:
		output << " ~= ";
		break;
	case CompType::LT:
		output << " < ";
		break;
	case CompType::GT:
		output << " > ";
		break;
	case CompType::LEQ:
		output << " =< ";
		break;
	case CompType::GEQ:
		output << " >= ";
		break;
	}
	return output.str();
}

template<>
std::string toString(const TsType& type) {
	std::stringstream output;
	switch (type) {
	case TsType::RIMPL:
		output << " <= ";
		break;
	case TsType::IMPL:
		output << " => ";
		break;
	case TsType::RULE:
		output << " <- ";
		break;
	case TsType::EQ:
		output << " <=> ";
		break;
	}
	return output.str();
}

template<>
std::string toString(const AggFunction& type) {
	std::stringstream output;
	switch (type) {
	case AggFunction::CARD:
		output << "card";
		break;
	case AggFunction::SUM:
		output << "sum";
		break;
	case AggFunction::PROD:
		output << "prod";
		break;
	case AggFunction::MAX:
		output << "max";
		break;
	case AggFunction::MIN:
		output << "min";
		break;
	}
	return output.str();
}

/*********************
 Shared strings
 *********************/

#include <unordered_map>
typedef std::unordered_map<std::string, std::string*> MSSP;
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
	CompType result;
	switch (comp) {
	case CompType::EQ:
		result = comp;
		break;
	case CompType::NEQ:
		result = comp;
		break;
	case CompType::LT:
		result = CompType::GT;
		break;
	case CompType::GT:
		result = CompType::LT;
		break;
	case CompType::LEQ:
		result = CompType::GEQ;
		break;
	case CompType::GEQ:
		result = CompType::LEQ;
		break;
	}
	return result;
}

CompType negateComp(CompType comp) {
	CompType result;
	switch (comp) {
	case CompType::EQ:
		result = CompType::NEQ;
		break;
	case CompType::NEQ:
		result = CompType::EQ;
		break;
	case CompType::LT:
		result = CompType::GEQ;
		break;
	case CompType::GT:
		result = CompType::LEQ;
		break;
	case CompType::LEQ:
		result = CompType::GT;
		break;
	case CompType::GEQ:
		result = CompType::LT;
		break;
	}
	return result;
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
