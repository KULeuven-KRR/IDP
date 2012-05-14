/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include "common.hpp"
#include "parseinfo.hpp"

// TODO enum class does not yet support comparison operators in 4.4.3

enum Language {/* TXT,*/
	IDP,
	ECNF, /*LATEX, CNF, */
	TPTP,
	FLATZINC,
	ASP,
	FIRST = IDP,
	LAST = ASP
};
enum Format {
	THREEVALUED,
	ALL,
	TWOVALUED
};

enum StringType {
	LANGUAGE,
	SYMMETRYBREAKING
};

enum IntType {
	SATVERBOSITY,
	GROUNDVERBOSITY,
	PROPAGATEVERBOSITY,
	NBMODELS,
	NRPROPSTEPS,
	LONGESTBRANCH,
	PROVERTIMEOUT,
	TIMEOUT,
	RANDOMSEED
};

enum BoolType {
	SHOWWARNINGS, // TODO Temporary solution to be able to disable warnings in tests
	TRACE,
	AUTOCOMPLETE,
	LONGNAMES,
	RELATIVEPROPAGATIONSTEPS,
	CREATETRANSLATION,
	MXRANDOMPOLARITYCHOICE,
	GROUNDLAZILY,
	TSEITINDELAY,
	SATISFIABILITYDELAY,
	GROUNDWITHBOUNDS,
	CPSUPPORT
};

enum DoubleType {

};

enum class SymmetryBreaking {
	NONE,
	STATIC,
	DYNAMIC,
	FIRST = NONE,
	LAST = DYNAMIC
};

enum class PrintBehaviour {
	PRINT,
	DONOTPRINT
};

template<class EnumType, class ConcreteType>
class TypedOption {
private:
	EnumType type;
	const std::string name;
	PrintBehaviour _visible;
	ConcreteType chosenvalue_;

public:
	TypedOption(EnumType type, const std::string& name, PrintBehaviour visible)
			: 	type(type),
				name(name),
				_visible(visible) {
	}
	virtual ~TypedOption() {
	}

	const std::string& getName() const {
		return name;
	}
	bool shouldPrint() const {
		return _visible == PrintBehaviour::PRINT;
	}

	EnumType getType() const {
		return type;
	}

	virtual bool isAllowedValue(const ConcreteType& value) = 0;
	virtual std::string printOption() const = 0;

	const ConcreteType& getValue() const {
		return chosenvalue_;
	}
	void setValue(const ConcreteType& chosenvalue) {
		Assert(isAllowedValue(chosenvalue));
		chosenvalue_ = chosenvalue;
	}
};

template<class EnumType, class ConcreteType>
class RangeOption: public TypedOption<EnumType, ConcreteType> {
private:
	ConcreteType lowerbound_, upperbound_;

	const ConcreteType& lower() const {
		return lowerbound_;
	}
	const ConcreteType& upper() const {
		return upperbound_;
	}

public:
	RangeOption(EnumType type, const std::string& name, const ConcreteType& lowerbound, const ConcreteType& upperbound, PrintBehaviour visible)
			: 	TypedOption<EnumType, ConcreteType>(type, name, visible),
				lowerbound_(lowerbound),
				upperbound_(upperbound) {
	}

	bool isAllowedValue(const ConcreteType& value) {
		return value >= lower() && value <= upper();
	}

	virtual std::string printOption() const;
};

template<class EnumType, class ConcreteType>
class EnumeratedOption: public TypedOption<EnumType, ConcreteType> {
private:
	std::set<ConcreteType> allowedvalues_;
	const std::set<ConcreteType>& getAllowedValues() const {
		return allowedvalues_;
	}

public:
	EnumeratedOption(EnumType type, const std::string& name, const std::set<ConcreteType>& allowedvalues, PrintBehaviour visible)
			: 	TypedOption<EnumType, ConcreteType>(type, name, visible),
				allowedvalues_(allowedvalues) {
	}

	bool isAllowedValue(const ConcreteType& value) {
		return getAllowedValues().find(value) != getAllowedValues().cend();
	}

	virtual std::string printOption() const;
};

class Options;

template<class EnumType, class ValueType>
class OptionPolicy {
private:
	std::vector<TypedOption<EnumType, ValueType>*> _options;
	std::map<std::string, EnumType> _name2type;
protected:
	void createOption(EnumType type, const std::string& name, const ValueType& lowerbound, const ValueType& upperbound, const ValueType& defaultValue,
			std::vector<std::string>& option2name, PrintBehaviour visible);
	void createOption(EnumType type, const std::string& name, const std::set<ValueType>& values, const ValueType& defaultValue,
			std::vector<std::string>& option2name, PrintBehaviour visible);
public:
	~OptionPolicy() {
		for (auto i = _options.cbegin(); i != _options.cend(); ++i) {
			delete (*i);
		}
	}
	bool isOption(const std::string& name) const {
		return _name2type.find(name) != _name2type.cend();
	}
	ValueType getValue(const std::string& name) const {
		Assert(isOption(name));
		return _options.at(_name2type.at(name))->getValue();
	}
	ValueType getValue(EnumType option) const {
		return _options.at(option)->getValue();
	}
	void setStrValue(const std::string& name, const ValueType& value) {
		Assert(isOption(name));
		//_options.at(_name2type.at(name))->setValue(value);
		setValue(_name2type.at(name), value);
	}
	void setValue(EnumType type, const ValueType& value) {
		_options.at(type)->setValue(value);
	}
	bool isAllowedValue(const std::string& name, const ValueType& value) const {
		return isOption(name) && _options.at(_name2type.at(name))->isAllowedValue(value);
	}
	std::string printOption(const std::string& name) const {
		return _options.at(_name2type.at(name))->printOption();
	}
	void addOptionStrings(std::vector<std::string>& optionlines) const {
		for (auto i = _options.cbegin(); i < _options.cend(); ++i) {
			optionlines.push_back((*i)->printOption());
		}
	}

	void copyValues(Options* opts);
};

// Note: Makes sure users cannot set the CPSUPPORT option when there is no cp solver available.
#ifndef WITHCP
template<>
inline void OptionPolicy<BoolType, bool>::setValue(BoolType type, const bool& value) {
	Assert(_options.size()>(uint)type);
	if (type == BoolType::CPSUPPORT and value != false) {
		std::clog <<"WARNING: CP support is not available. Option cpsupport is ignored.\n";
	} else {
		_options.at(type)->setValue(value);
	}
}
#endif

typedef OptionPolicy<IntType, int> IntPol;
typedef OptionPolicy<BoolType, bool> BoolPol;
typedef OptionPolicy<DoubleType, double> DoublePol;
typedef OptionPolicy<StringType, std::string> StringPol;

template<class T>
struct OptionValueTraits;

template<>
struct OptionValueTraits<int> {
	typedef IntType EnumType;
};

template<>
struct OptionValueTraits<double> {
	typedef DoubleType EnumType;
};

template<>
struct OptionValueTraits<bool> {
	typedef BoolType EnumType;
};

template<>
struct OptionValueTraits<std::string> {
	typedef StringType EnumType;
};

template<class T>
struct OptionTypeTraits;

template<>
struct OptionTypeTraits<IntType> {
	typedef int ValueType;
};

template<>
struct OptionTypeTraits<DoubleType> {
	typedef double ValueType;
};

template<>
struct OptionTypeTraits<BoolType> {
	typedef bool ValueType;
};

template<>
struct OptionTypeTraits<StringType> {
	typedef std::string ValueType;
};

/**
 * Class to represent a block of options
 */
class Options: public IntPol, public BoolPol, public DoublePol, public StringPol {
private:
	std::vector<std::string> _option2name;

public:
	Options();
	~Options() {
	}

	bool isOption(const std::string&) const;

	template<class ValueType>
	bool isOptionOfType(const std::string& optname) const {
		return OptionPolicy<typename OptionValueTraits<ValueType>::EnumType, ValueType>::isOption(optname);
	}
	template<class ValueType>
	ValueType getValueOfType(const std::string& optname) const {
		return OptionPolicy<typename OptionValueTraits<ValueType>::EnumType, ValueType>::getValue(optname);
	}

	template<class EnumType>
	typename OptionTypeTraits<EnumType>::ValueType getValue(EnumType type) {
		return OptionPolicy<EnumType, typename OptionTypeTraits<EnumType>::ValueType>::getValue(type);
	}

	template<class EnumType, class ValueType>
	void setValue(EnumType type, const ValueType& value) {
		OptionPolicy < EnumType, ValueType > ::setValue(type, value);
	}

	void copyValues(Options*);

	std::string printAllowedValues(const std::string& option) const;
	std::ostream& put(std::ostream&) const;

	Language language() const;
	SymmetryBreaking symmetryBreaking() const;

	// NOTE: do NOT call this code outside luaconnection or other user interface methods.
	template<class ValueType>
	void setValue(const std::string& name, const ValueType& value) {
		OptionPolicy<typename OptionValueTraits<ValueType>::EnumType, ValueType>::setStrValue(name, value);
	}

	template<class ValueType>
	bool isAllowedValue(const std::string& name, const ValueType& value) {
		return OptionPolicy<typename OptionValueTraits<ValueType>::EnumType, ValueType>::isAllowedValue(name, value);
	}
};

#endif
