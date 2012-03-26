/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include <sstream>
#include "options.hpp"
#include "errorhandling/error.hpp"
#include <algorithm>
#include "utils/NumericLimits.hpp"
using namespace std;

std::string str(Language choice) {
	switch (choice) {
	//case Language::TXT:
	//	return "txt";
	case Language::IDP:
		return "idp";
	case Language::ECNF:
		return "ecnf";
	case Language::TPTP:
		return "tptp";
	case Language::FLATZINC:
		return "flatzinc";
	case Language::ASP:
		return "asp";
	default:
		return "unknown";
		//case Language::CNF:
		//	return "cnf";
		//case Language::LATEX:
		//	return "latex";
	}
}

std::set<Language> possibleLanguageValues(){
	return {Language::IDP,Language::FLATZINC,Language::TPTP,Language::ECNF,Language::ASP};
}
std::set<std::string> possibleLanguageStringValues(){
	std::set<std::string> s;
	auto values = possibleLanguageValues();
	for(auto i=values.cbegin(); i!=values.cend(); ++i){
		s.insert(str(*i));
	}
	return s;
}

std::string str(Format choice) {
	switch (choice) {
	case Format::TWOVALUED:
		return "twovalued";
	case Format::THREEVALUED:
		return "threevalued";
	case Format::ALL:
		return "all";
	default:
		return "unknown";
	}
}

// TODO add descriptions to options

Options::Options() {
	std::set<bool> boolvalues { true, false };
	BoolPol::createOption(BoolType::SHOWWARNINGS, "showwarnings", boolvalues, true, _option2name, PrintBehaviour::DONOTPRINT); // TODO Temporary solution to be able to disable warnings in tests
	//BoolPol::createOption(BoolType::PRINTTYPES, "printtypes", boolvalues, true, _option2name); DOET NIETS!
	BoolPol::createOption(BoolType::CPSUPPORT, "cpsupport", boolvalues, false, _option2name, PrintBehaviour::DONOTPRINT);
	BoolPol::createOption(BoolType::TRACE, "trace", boolvalues, false, _option2name, PrintBehaviour::PRINT);
	BoolPol::createOption(BoolType::AUTOCOMPLETE, "autocomplete", boolvalues, true, _option2name, PrintBehaviour::DONOTPRINT); // TODO is only used before any lua is executed (during parsing) so not useful for user atm!
	BoolPol::createOption(BoolType::LONGNAMES, "longnames", boolvalues, false, _option2name, PrintBehaviour::PRINT);
	BoolPol::createOption(BoolType::RELATIVEPROPAGATIONSTEPS, "relativepropsteps", boolvalues, true, _option2name, PrintBehaviour::DONOTPRINT);
	BoolPol::createOption(BoolType::CREATETRANSLATION, "createtranslation", boolvalues, false, _option2name, PrintBehaviour::DONOTPRINT); //show? bugged: when grounding: write out the information about which string belongs to which cnf number
	BoolPol::createOption(BoolType::MXRANDOMPOLARITYCHOICE, "randomvaluechoice", boolvalues, false, _option2name, PrintBehaviour::DONOTPRINT); //belongs to bdds : something about random seeds
	BoolPol::createOption(BoolType::GROUNDLAZILY, "groundlazily", boolvalues, false, _option2name, PrintBehaviour::DONOTPRINT);
	BoolPol::createOption(BoolType::GROUNDWITHBOUNDS, "groundwithbounds", boolvalues, false, _option2name, PrintBehaviour::PRINT);
	//BoolPol::createOption(BoolType::MODELCOUNTEQUIVALENCE, "nbmodelequivalent", boolvalues, false, _option2name, PrintBehaviour::PRINT); //Verwijderd: is nu automatisch wanneer nbmodels == 1

	IntPol::createOption(IntType::RANDOMSEED, "seed", 1, getMaxElem<int>(), 91648253, _option2name, PrintBehaviour::PRINT); // This is the default minisat random seed to (for consistency)
	IntPol::createOption(IntType::SATVERBOSITY, "satverbosity", 0, getMaxElem<int>(), 0, _option2name, PrintBehaviour::PRINT);
	IntPol::createOption(IntType::GROUNDVERBOSITY, "groundverbosity", 0, getMaxElem<int>(), 0, _option2name, PrintBehaviour::PRINT);
	IntPol::createOption(IntType::PROPAGATEVERBOSITY, "propagateverbosity", 0, getMaxElem<int>(), 0, _option2name, PrintBehaviour::DONOTPRINT);
	IntPol::createOption(IntType::NBMODELS, "nbmodels", 0, getMaxElem<int>(), 1, _option2name, PrintBehaviour::PRINT);
	IntPol::createOption(IntType::NRPROPSTEPS, "nrpropsteps", 0, getMaxElem<int>(), 4, _option2name, PrintBehaviour::DONOTPRINT);
	IntPol::createOption(IntType::LONGESTBRANCH, "longestbranch", 0, getMaxElem<int>(), 8, _option2name, PrintBehaviour::DONOTPRINT);
	IntPol::createOption(IntType::SYMMETRY, "symmetry", 0, getMaxElem<int>(), 0, _option2name, PrintBehaviour::DONOTPRINT); //TODO: testing!

	// NOTE: set this to infinity, so he always starts timing, even when the options have not been read in yet.
	// Afterwards, setting them to 0 stops the timing
	IntPol::createOption(IntType::TIMEOUT, "timeout", 0, getMaxElem<int>(), getMaxElem<int>(), _option2name, PrintBehaviour::PRINT);
	IntPol::createOption(IntType::PROVERTIMEOUT, "provertimeout", 0, getMaxElem<int>(), getMaxElem<int>(), _option2name, PrintBehaviour::DONOTPRINT);

	StringPol::createOption(StringType::LANGUAGE, "language", possibleLanguageStringValues(), str(Language::IDP), _option2name,
			PrintBehaviour::PRINT);
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::createOption(EnumType type, const std::string& name, const ValueType& lowerbound, const ValueType& upperbound,
		const ValueType& defaultValue, std::vector<std::string>& option2name, PrintBehaviour visible) {
	_name2type[name] = type;
	auto newoption = new RangeOption<EnumType, ValueType>(type, name, lowerbound, upperbound, visible);
	newoption->setValue(defaultValue);
	auto& options = _options;
	if (options.size() <= (unsigned int) type) {
		options.resize(type + 1, NULL);
		option2name.resize(type + 1, "");
		option2name[type] = name;
	}
	options[type] = newoption;
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::createOption(EnumType type, const std::string& name, const std::set<ValueType>& values,
		const ValueType& defaultValue, std::vector<std::string>& option2name, PrintBehaviour visible) {
	_name2type[name] = type;
	auto newoption = new EnumeratedOption<EnumType, ValueType>(type, name, values, visible);
	newoption->setValue(defaultValue);
	auto& options = _options;
	if (options.size() <= (unsigned int) type) {
		options.resize(type + 1, NULL);
		option2name.resize(type + 1, "");
		option2name[type] = name;
	}
	options[type] = newoption;
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::copyValues(Options* opts) {
	for (auto i = _options.cbegin(); i < _options.cend(); ++i) {
		(*i)->setValue(opts->getValue((*i)->getType()));
	}
}

template<class EnumType, class ConcreteType>
std::string RangeOption<EnumType, ConcreteType>::printOption() const {
	if (TypedOption<EnumType, ConcreteType>::shouldPrint()) {
		std::stringstream ss; // TODO correct usage
		ss << "\t" << TypedOption<EnumType, ConcreteType>::getName() << " = " << TypedOption<EnumType, ConcreteType>::getValue();
		ss << "\n\t\t => between " << lower() << " and " << upper() << ".\n";
		return ss.str();
	} else {
		return "";
	}
}

template<>
std::string EnumeratedOption<BoolType, bool>::printOption() const {
	if (TypedOption<BoolType, bool>::shouldPrint()) {
		std::stringstream ss;
		ss << "\t" << TypedOption<BoolType, bool>::getName() << " = " << (TypedOption<BoolType, bool>::getValue() ? "true" : "false");
		ss << "\n\t\t => one of [";
		bool begin = true;
		for (auto i = getAllowedValues().cbegin(); i != getAllowedValues().cend(); ++i) {
			if (not begin) {
				ss << ", ";
			}
			begin = false;
			ss << ((*i) ? "true" : "false");
		}
		ss << "]\n";
		return ss.str();
	} else {
		return "";
	}
}

template<class EnumType, class ConcreteType>
std::string EnumeratedOption<EnumType, ConcreteType>::printOption() const {
	if (TypedOption<EnumType, ConcreteType>::shouldPrint()) {
		std::stringstream ss;
		ss << "\t" << TypedOption<EnumType, ConcreteType>::getName() << " = " << TypedOption<EnumType, ConcreteType>::getValue();
		ss << "\n\t\t => one of [";
		bool begin = true;
		for (auto i = getAllowedValues().cbegin(); i != getAllowedValues().cend(); ++i) {
			if (not begin) {
				ss << ", ";
			}
			begin = false;
			ss << *i;
		}
		ss << "]\n";
		return ss.str();
	} else {
		return "";
	}
}

Language Options::language() const {
	auto values = possibleLanguageValues();
	const std::string& value = StringPol::getValue(StringType::LANGUAGE);
	for(auto i=values.cbegin(); i!=values.cend(); ++i) {
		if (value.compare(str(*i)) == 0) {
			return *i;
		}
	}
	Warning::warning("Encountered unsupported language option, assuming ECNF.\n");
	return Language::ECNF;
}

std::string Options::printAllowedValues(const std::string& name) const {
	if (isOptionOfType<int>(name)) {
		return IntPol::printOption(name);
	} else if (isOptionOfType<std::string>(name)) {
		return StringPol::printOption(name);
	} else if (isOptionOfType<double>(name)) {
		return DoublePol::printOption(name);
	} else if (isOptionOfType<bool>(name)) {
		return BoolPol::printOption(name);
	} else {
		return "";
	}
}

bool Options::isOption(const string& optname) const {
	return isOptionOfType<int>(optname) || isOptionOfType<bool>(optname) || isOptionOfType<double>(optname) || isOptionOfType<std::string>(optname);
}

void Options::copyValues(Options* opts) {
	StringPol::copyValues(opts);
	BoolPol::copyValues(opts);
	IntPol::copyValues(opts);
	DoublePol::copyValues(opts);
}

template<class OptionList, class StringList>
void getStringFromOption(const OptionList& list, StringList& newlist) {
	for (auto it = list.cbegin(); it != list.cend(); ++it) {
		stringstream ss;
		ss << (*it)->getName() << " = " << (*it)->getValue();
		newlist.push_back(ss.str());
	}
}

ostream& Options::put(ostream& output) const {
	vector<string> optionslines;
	StringPol::addOptionStrings(optionslines);
	IntPol::addOptionStrings(optionslines);
	BoolPol::addOptionStrings(optionslines);
	DoublePol::addOptionStrings(optionslines);

	sort(optionslines.begin(), optionslines.end());
	for (auto i = optionslines.cbegin(); i < optionslines.cend(); ++i) {
		output << *i;
	}

	return output;
}
