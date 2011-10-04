/************************************
	options.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <sstream>
#include <limits>
#include "options.hpp"
#include "error.hpp"
#include "internalargument.hpp"
#include <algorithm>
using namespace std;

std::string str(Language choice){
	switch(choice){
		case Language::TXT: return "txt";
		case Language::IDP: return "idp";
		case Language::ECNF: return "ecnf";
		case Language::TPTP: return "tptp";
		case Language::CNF: return "cnf";
		case Language::ASP: return "asp";
		case Language::LATEX: return "latex";
	}
}

// TODO add descriptions to options

Options::Options(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) {
	createOption(OptionType::PRINTTYPES, "printtypes", {true, false}, true);
	createOption(OptionType::CPSUPPORT, "cpsupport", {true, false}, false);
	createOption(OptionType::TRACE, "trace", {true, false}, false);
	createOption(OptionType::AUTOCOMPLETE, "autocomplete", {true, false}, true);
	createOption(OptionType::LONGNAMES, "longnames", {true, false}, false);
	createOption(OptionType::RELATIVEPROPAGATIONSTEPS, "relativepropsteps", {true, false}, true);
	createOption(OptionType::CREATETRANSLATION, "createtranslation", {true, false}, false);

	createOption(OptionType::SATVERBOSITY, "satverbosity", 0,numeric_limits<int>::max(),0);
	createOption(OptionType::GROUNDVERBOSITY, "groundverbosity", 0,numeric_limits<int>::max(),0);
	createOption(OptionType::PROPAGATEVERBOSITY, "propagateverbosity", 0,numeric_limits<int>::max(),0);
	createOption(OptionType::NRMODELS, "nrmodels", 0,numeric_limits<int>::max(),1);
	createOption(OptionType::NRPROPSTEPS, "nrpropsteps", 0,numeric_limits<int>::max(),4);
	createOption(OptionType::LONGESTBRANCH, "longestbranch", 0,numeric_limits<int>::max(),8);
	createOption(OptionType::SYMMETRY, "symmetry", 0,numeric_limits<int>::max(),0);
	createOption(OptionType::PROVERTIMEOUT, "provertimeout", 0,numeric_limits<int>::max(),numeric_limits<int>::max());

	// FIXME replace all by getstring method for the enums!
	createOption<std::string>(OptionType::LANGUAGE, "language", std::set<std::string>{str(TXT), str(IDP), str(LATEX), str(ECNF), str(ASP), str(TPTP), str(ASP)}, str(IDP));

	createOption<std::string>(OptionType::MODELFORMAT, "modelformat", std::set<std::string>{"all", "threevalued", "2valued"}, "threevalued");
}

Options::~Options() {
	for(auto i=_booloptions.begin(); i!=_booloptions.end(); ++i) {
		delete((*i).second);
	}
	for(auto i=_stringoptions.begin(); i!=_stringoptions.end(); ++i) {
		delete((*i).second);
	}
	for(auto i=_intoptions.begin(); i!=_intoptions.end(); ++i) {
	delete((*i).second);
	}
}

InternalArgument Options::getValue(const std::string& name) const{
	return getValue(_name2optionType.at(name));
}

InternalArgument Options::getValue(OptionType option) const{
	InternalArgument a;
	if(_booloptions.find(option)!=_booloptions.end()){
		a = InternalArgument(_booloptions.at(option)->getValue());
	}else if(_intoptions.find(option)!=_intoptions.end()){
		a = InternalArgument(_intoptions.at(option)->getValue());
	}else if(_stringoptions.find(option)!=_stringoptions.end()){
		a = InternalArgument(new std::string(_stringoptions.at(option)->getValue()));
	}else{
		assert(false);
		// FIXME error
	}
	return a;
}

std::string Options::printAllowedValues(const std::string& name) const{
	OptionType option = _name2optionType.at(name);
	if(_booloptions.find(option)!=_booloptions.end()){
		return _booloptions.at(option)->printOption();
	}else if(_intoptions.find(option)!=_intoptions.end()){
		return _intoptions.at(option)->printOption();
	}else if(_stringoptions.find(option)!=_stringoptions.end()){
		return _stringoptions.at(option)->printOption();
	}
	return "";
}

bool Options::isOption(const string& optname) const {
	auto it = _name2optionType.find(optname);
	if(it==_name2optionType.end()){
		return false;
	}
	OptionType option = it->second;
	return _stringoptions.find(option) != _stringoptions.end() || _booloptions.find(option) != _booloptions.end() || _intoptions.find(option) != _intoptions.end();
}

void Options::copyValues(Options* opts) {
	for(auto it = opts->_stringoptions.begin(); it != opts->_stringoptions.end(); ++it) {
		setValue(_optionType2name[it->first],it->second->getValue());
	}
	for(auto it = opts->_intoptions.begin(); it != opts->_intoptions.end(); ++it) {
		setValue(_optionType2name[it->first],it->second->getValue());
	}
	for(auto it = opts->_booloptions.begin(); it != opts->_booloptions.end(); ++it) {
		setValue(_optionType2name[it->first],it->second->getValue());
	}
}

template<>
std::map<OptionType,TypedOption<OptionType, std::string>* >& Options::getOptions(const std::string& value) { return _stringoptions; }
template<>
std::map<OptionType,TypedOption<OptionType, bool>* >& Options::getOptions(const bool& value) { return _booloptions; }
template<>
std::map<OptionType,TypedOption<OptionType, int>* >& Options::getOptions(const int& value) { return _intoptions; }
template<>
std::map<OptionType,TypedOption<OptionType, double>* >& Options::getOptions(const double& value) { assert(false); }

Language Options::language() const {
	const std::string& value = _stringoptions.at(OptionType::LANGUAGE)->getValue();
	if(value.compare("txt")==0){
		return Language::TXT;
	}else if(value.compare("tptp")==0){
		return Language::TPTP;
	}else if(value.compare("idp")==0){
		return Language::IDP;
	}else if(value.compare("ecnf")==0){
		return Language::ECNF;
	}else if(value.compare("asp")==0){
		return Language::ASP;
	}else{
		assert(value.compare("latex")==0);
		return Language::LATEX;
	}
}

bool Options::printtypes() const {
	return _booloptions.at(OptionType::PRINTTYPES)->getValue();
}

int Options::nrmodels() const {
	return _intoptions.at(OptionType::NRMODELS)->getValue();
}

int Options::satverbosity() const {
	return _intoptions.at(OptionType::SATVERBOSITY)->getValue();
}

int Options::groundverbosity() const {
	return _intoptions.at(OptionType::GROUNDVERBOSITY)->getValue();
}

int Options::propagateverbosity() const {
	return _intoptions.at(OptionType::PROPAGATEVERBOSITY)->getValue();
}

int Options::nrpropsteps() const {
	return _intoptions.at(OptionType::NRPROPSTEPS)->getValue();
}

int Options::longestbranch() const {
	return _intoptions.at(OptionType::LONGESTBRANCH)->getValue();
}

int Options::symmetry() const {
	return _intoptions.at(OptionType::SYMMETRY)->getValue();
}

int Options::provertimeout() const {
	return _intoptions.at(OptionType::PROVERTIMEOUT)->getValue();
}

bool Options::cpsupport() const {
	return _booloptions.at(OptionType::CPSUPPORT)->getValue();
}

bool Options::autocomplete() const {
	return _booloptions.at(OptionType::AUTOCOMPLETE)->getValue();
}

bool Options::trace() const {
	return _booloptions.at(OptionType::TRACE)->getValue();
}

bool Options::longnames() const {
	return _booloptions.at(OptionType::LONGNAMES)->getValue();
}

bool Options::relativepropsteps() const {
	return _booloptions.at(OptionType::RELATIVEPROPAGATIONSTEPS)->getValue();
}

bool Options::writeTranslation() const {
	return _booloptions.at(OptionType::CREATETRANSLATION)->getValue();
}

template<class Option2NameList, class OptionList, class StringList>
void getStringFromOption(const Option2NameList& namelist, const OptionList& list, StringList& newlist){
	for(auto it = list.begin(); it != list.end(); ++it) {
		stringstream ss;
		ss << namelist.at(it->first) << " = " << it->second->getValue();
		newlist.push_back(ss.str());
	}
}

ostream& Options::put(ostream& output) const {
	vector<string> optionslines;
	getStringFromOption(_optionType2name, _stringoptions, optionslines);
	getStringFromOption(_optionType2name, _intoptions, optionslines);
	for(auto it = _booloptions.begin(); it != _booloptions.end(); ++it) {
		output << _optionType2name.at(it->first) << " = " << (it->second->getValue() ? "true" : "false") << "\n";
	}

	sort(optionslines.begin(), optionslines.end());
	for(auto i=optionslines.begin(); i<optionslines.end(); ++i){
		output <<*i <<"\n";
	}

	return output;
}

string Options::to_string() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}
 
