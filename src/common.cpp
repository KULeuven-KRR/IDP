/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

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
	stringstream ss;
	ss << "The following feature is not yet implemented: " << message << '\n';
	ss << "Please send an e-mail to krr@cs.kuleuven.be if you really need this feature.";
	return IdpException(ss.str());
}

bool isInt(double d) {
	return (double(int(d)) == d);
}

bool isInt(const string& s) {
	stringstream i(s);
	int n;
	return (i >> n).eof();
}

bool isDouble(const string& s) {
	stringstream i(s);
	double d;
	return (i >> d).eof();
}

int toInt(const string& s) {
	stringstream i(s);
	int n;
	if (not (i >> n).eof()) {
		return 0;
	} else {
		return n;
	}
}

double toDouble(const string& s) {
	stringstream i(s);
	double d;
	if (not (i >> d).eof()) {
		return 0;
	} else {
		return d;
	}
}

double applyAgg(const AggFunction& agg, const vector<double>& args) {
	double d = 0;
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
		d = getMaxElem<int>();
		for (size_t n = 0; n < args.size(); ++n) {
			d = (d <= args[n] ? d : args[n]);
		}
		break;
	case AggFunction::MAX:
		d = getMinElem<int>();
		for (size_t n = 0; n < args.size(); ++n) {
			d = (d >= args[n] ? d : args[n]);
		}
		break;
	}
	return d;
}

bool operator==(VarId left, VarId right){
	return left.id==right.id;
}
bool operator!=(VarId left, VarId right){
	return not (left.id==right.id);
}
bool operator<(VarId left, VarId right){
	return left.id<right.id;
}
bool operator==(DefId left, DefId right){
	return left.id==right.id;
}
bool operator!=(DefId left, DefId right){
	return not (left.id==right.id);
}
bool operator<(DefId left, DefId right){
	return left.id<right.id;
}
bool operator==(SetId left, SetId right){
	return left.id==right.id;
}
bool operator!=(SetId left, SetId right){
	return not (left.id==right.id);
}
bool operator<(SetId left, SetId right){
	return left.id<right.id;
}

std::ostream& operator<<(std::ostream& out, const VarId& id) {
	out <<id.id;
	return out;
}

std::ostream& operator<<(std::ostream& out, const DefId& id) {
	out <<id.id;
	return out;
}

std::ostream& operator<<(std::ostream& out, const SetId& id) {
	out <<id.id;
	return out;
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
	Context result = Context::BOTH;
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

TruthValue operator not(TruthValue t) {
	TruthValue result = TruthValue::Unknown;
	switch (t) {
	case TruthValue::Unknown:
		result = TruthValue::Unknown;
		break;
	case TruthValue::True:
		result = TruthValue::False;
		break;
	case TruthValue::False:
		result = TruthValue::True;
		break;
	}
	return result;
}
TruthValue operator~(TruthValue t) {
	return not t;
}

bool isConj(SIGN sign, bool conj) {
	return (sign == SIGN::POS && conj) || (sign == SIGN::NEG && ~conj);
}

PRINTTOSTREAMIMPL(std::string)
PRINTTOSTREAMIMPL(char)
PRINTTOSTREAMIMPL(char*)
PRINTTOSTREAMIMPL(CompType)
PRINTTOSTREAMIMPL(TruthType)
PRINTTOSTREAMIMPL(TsType)
PRINTTOSTREAMIMPL(AggFunction)
PRINTTOSTREAMIMPL(TruthValue)

std::ostream& operator<<(std::ostream& output, const CompType& type) {
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
	return output;
}

std::ostream& operator<<(std::ostream& output, const TruthType& type){
	switch (type) {
	case TruthType::CERTAIN_FALSE:
		output << " cf ";
		break;
	case TruthType::CERTAIN_TRUE:
		output << " ct ";
		break;
	case TruthType::POSS_FALSE:
		output << " pf ";
		break;
	case TruthType::POSS_TRUE:
		output << " pt ";
		break;
	}
	return output;
}

std::ostream& operator<<(std::ostream& output, const TruthValue& type){
	switch (type) {
	case TruthValue::True:
		output << "true";
		break;
	case TruthValue::False:
		output << "false";
		break;
	case TruthValue::Unknown:
		output << "unknown";
		break;
	}
	return output;
}

std::ostream& operator<<(std::ostream& output, const TsType& type) {
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
	return output;
}

std::ostream& operator<<(std::ostream& output, const AggFunction& type) {
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
	return output;
}

CompType invertComp(CompType comp) {
	CompType result = CompType::EQ;
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
	CompType result = CompType::EQ;
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
