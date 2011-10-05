/************************************
	options.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <sstream>
#include <limits>
#include "options.hpp"
#include "error.hpp"
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

std::string str(Format choice){
	switch(choice){
		case Format::TWOVALUED: return "twovalued";
		case Format::THREEVALUED: return "threevalued";
		case Format::ALL: return "all";
	}
}

// TODO add descriptions to options

Options::Options(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) {
	std::set<bool> boolvalues{true, false};
	createOption(OptionType::PRINTTYPES, "printtypes", boolvalues, true);
	createOption(OptionType::CPSUPPORT, "cpsupport", boolvalues, false);
	createOption(OptionType::TRACE, "trace", boolvalues, false);
	createOption(OptionType::AUTOCOMPLETE, "autocomplete", boolvalues, true);
	createOption(OptionType::LONGNAMES, "longnames", boolvalues, false);
	createOption(OptionType::RELATIVEPROPAGATIONSTEPS, "relativepropsteps", boolvalues, true);
	createOption(OptionType::CREATETRANSLATION, "createtranslation", boolvalues, false);

	createOption(OptionType::SATVERBOSITY, "satverbosity", 0,numeric_limits<int>::max(),0);
	createOption(OptionType::GROUNDVERBOSITY, "groundverbosity", 0,numeric_limits<int>::max(),0);
	createOption(OptionType::PROPAGATEVERBOSITY, "propagateverbosity", 0,numeric_limits<int>::max(),0);
	createOption(OptionType::NRMODELS, "nrmodels", 0,numeric_limits<int>::max(),1);
	createOption(OptionType::NRPROPSTEPS, "nrpropsteps", 0,numeric_limits<int>::max(),4);
	createOption(OptionType::LONGESTBRANCH, "longestbranch", 0,numeric_limits<int>::max(),8);
	createOption(OptionType::SYMMETRY, "symmetry", 0,numeric_limits<int>::max(),0);
	createOption(OptionType::PROVERTIMEOUT, "provertimeout", 0,numeric_limits<int>::max(),numeric_limits<int>::max());

	createOption<std::string>(OptionType::LANGUAGE,
							"language",
							std::set<std::string>{str(Language::TXT), str(Language::IDP),
												str(Language::LATEX), str(Language::ECNF),
												str(Language::ASP), str(Language::TPTP),
												str(Language::ASP)},
							str(Language::IDP));
	createOption<std::string>(OptionType::MODELFORMAT,
							"modelformat",
							std::set<std::string>{str(Format::TWOVALUED),str(Format::THREEVALUED),str(Format::ALL)},
							str(Format::THREEVALUED));
}

Options::~Options() {
	for(auto i=_booloptions.begin(); i!=_booloptions.end(); ++i) {
		delete(*i);
	}
	for(auto i=_stringoptions.begin(); i!=_stringoptions.end(); ++i) {
		delete(*i);
	}
	for(auto i=_intoptions.begin(); i!=_intoptions.end(); ++i) {
	delete(*i);
	}
}

template<class ConcreteType>
void Options::createOption(OptionType type, const std::string& name, const ConcreteType& lowerbound, const ConcreteType& upperbound, const ConcreteType& defaultValue){
	_name2optionType[name] = type;
	auto newoption = new RangeOption<ConcreteType>(name, lowerbound, upperbound);
	newoption->setValue(defaultValue);
	vector<TypedOption<ConcreteType>*>& options = getOptions(defaultValue);
	if(options.size()<=type){
		options.resize(type+1, NULL);
		_option2name.resize(type+1, "");
		_option2name[type] = name;
	}
	options[type] =  newoption;
}
template<class ConcreteType>
void Options::createOption(OptionType type, const std::string& name, const std::set<ConcreteType>& values, const ConcreteType& defaultValue){
	_name2optionType[name] = type;
	auto newoption = new EnumeratedOption<ConcreteType>(name, values);
	newoption->setValue(defaultValue);
	vector<TypedOption<ConcreteType>*>& options = getOptions(defaultValue);
	if(options.size()<=type){
		options.resize(type+1, NULL);
		_option2name.resize(type+1, "");
		_option2name[type] = name;
	}
	options[type] = newoption;
}

std::string Options::printAllowedValues(const std::string& name) const{
	OptionType option = _name2optionType.at(name);
	if(hasOption(_booloptions, option)){
		return _booloptions.at(option)->printOption();
	}else if(hasOption(_intoptions, option)){
		return _intoptions.at(option)->printOption();
	}else if(hasOption(_stringoptions, option)){
		return _stringoptions.at(option)->printOption();
	}else{
		return "";
	}
}

bool Options::isOption(const string& optname) const {
	auto it = _name2optionType.find(optname);
	if(it==_name2optionType.end()){
		return false;
	}
	OptionType option = it->second;
	return hasOption(_stringoptions, option) || hasOption(_intoptions, option) || hasOption(_booloptions, option);
}

bool Options::isStringOption(const std::string& optname) const{
	auto it = _name2optionType.find(optname);
	if(it==_name2optionType.end()){
		return false;
	}
	OptionType option = it->second;
	return hasOption(_stringoptions, option);
}
bool Options::isBoolOption(const std::string& optname) const{
	auto it = _name2optionType.find(optname);
	if(it==_name2optionType.end()){
		return false;
	}
	OptionType option = it->second;
	return hasOption(_booloptions, option);
}
bool Options::isIntOption(const std::string& optname) const{
	auto it = _name2optionType.find(optname);
	if(it==_name2optionType.end()){
		return false;
	}
	OptionType option = it->second;
	return hasOption(_intoptions, option);
}

int	Options::getIntValue(const std::string& name) const{
	assert(isIntOption(name));
	return _intoptions.at(_name2optionType.at(name))->getValue();
}
bool Options::getBoolValue(const std::string& name) const{
	assert(isBoolOption(name));
	return _booloptions.at(_name2optionType.at(name))->getValue();
}
std::string Options::getStringValue(const std::string& name) const{
	assert(isStringOption(name));
	return _stringoptions.at(_name2optionType.at(name))->getValue();
}

void Options::copyValues(Options* opts) {
	for(auto it = opts->_stringoptions.begin(); it != opts->_stringoptions.end(); ++it) {
		setValue((*it)->getName(),(*it)->getValue());
	}
	for(auto it = opts->_intoptions.begin(); it != opts->_intoptions.end(); ++it) {
		setValue((*it)->getName(),(*it)->getValue());
	}
	for(auto it = opts->_booloptions.begin(); it != opts->_booloptions.end(); ++it) {
		setValue((*it)->getName(),(*it)->getValue());
	}
}

template<>
std::vector<TypedOption<std::string>* >& Options::getOptions(const std::string& value) { return _stringoptions; }
template<>
std::vector<TypedOption<bool>* >& Options::getOptions(const bool& value) { return _booloptions; }
template<>
std::vector<TypedOption<int>* >& Options::getOptions(const int& value) { return _intoptions; }
template<>
std::vector<TypedOption<double>* >& Options::getOptions(const double& value) { assert(false); }

Language Options::language() const {
	const std::string& value = _stringoptions.at(OptionType::LANGUAGE)->getValue();
	if(value.compare(str(Language::TXT))==0){
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

template<class OptionList, class StringList>
void getStringFromOption(const OptionList& list, StringList& newlist){
	for(auto it = list.begin(); it != list.end(); ++it) {
		stringstream ss;
		ss << (*it)->getName() << " = " << (*it)->getValue();
		newlist.push_back(ss.str());
	}
}

ostream& Options::put(ostream& output) const {
	vector<string> optionslines;
	getStringFromOption(_stringoptions, optionslines);
	getStringFromOption(_intoptions, optionslines);
	for(auto it = _booloptions.begin(); it != _booloptions.end(); ++it) {
		output << (*it)->getName() << " = " << ((*it)->getValue() ? "true" : "false") << "\n";
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
 
