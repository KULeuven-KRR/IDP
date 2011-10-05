/************************************
	options.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>
#include <vector>
#include <string>
#include <cassert>
#include <map>
#include <sstream>
#include <set>
#include "parseinfo.hpp"

// TODO enum class does not yet support comparison operators in 4.4.3

enum Language { TXT, IDP, ECNF, LATEX, ASP, CNF, TPTP };
enum Format { THREEVALUED, ALL, TWOVALUED};

enum StringType{
	LANGUAGE, MODELFORMAT
};
enum IntType{
	SATVERBOSITY, GROUNDVERBOSITY, PROPAGATEVERBOSITY, NRMODELS, NRPROPSTEPS, LONGESTBRANCH, SYMMETRY, PROVERTIMEOUT
};
enum BoolType{
	PRINTTYPES, CPSUPPORT, TRACE, AUTOCOMPLETE, LONGNAMES, RELATIVEPROPAGATIONSTEPS, CREATETRANSLATION
};
enum DoubleType{

};

template<class EnumType, class ConcreteType>
class TypedOption{
private:
	EnumType type;
	const std::string name;
	ConcreteType chosenvalue_;

public:
	TypedOption(EnumType type, const std::string& name): type(type), name(name){}
	~TypedOption(){}

	const std::string& getName() const { return name; }
	EnumType getType() const { return type; }

	virtual bool 		isAllowedValue(const ConcreteType& value) = 0;
	virtual std::string printOption() const = 0;

	const ConcreteType&	getValue() const { return chosenvalue_; }
	void setValue(const ConcreteType& chosenvalue){
		assert(isAllowedValue(chosenvalue));
		chosenvalue_ = chosenvalue;
	}
};

template<class EnumType, class ConcreteType>
class RangeOption: public TypedOption<EnumType, ConcreteType> {
private:
	ConcreteType lowerbound_, upperbound_;

	const ConcreteType&	lower() const { return lowerbound_;  }
	const ConcreteType&	upper() const { return upperbound_;  }

public:
	RangeOption(EnumType type, const std::string& name, const ConcreteType& lowerbound, const ConcreteType& upperbound)
		: TypedOption<EnumType, ConcreteType>(type, name), lowerbound_(lowerbound), upperbound_(upperbound) { }

	bool isAllowedValue(const ConcreteType& value){
		return value >= lower() && value <= upper();
	}

	virtual std::string printOption() const;
};

template<class EnumType, class ConcreteType>
class EnumeratedOption: public TypedOption<EnumType, ConcreteType> {
private:
	std::set<ConcreteType>	allowedvalues_;
	const std::set<ConcreteType>& getAllowedValues() const { return allowedvalues_; }

public:
	EnumeratedOption(EnumType type, const std::string& name, const std::set<ConcreteType>& allowedvalues)
		: TypedOption<EnumType, ConcreteType>(type, name), allowedvalues_(allowedvalues) { }

	bool isAllowedValue(const ConcreteType& value){
		return getAllowedValues().find(value)!=getAllowedValues().end();
	}

	virtual std::string printOption() const;
};

class Options;

template<class EnumType, class ValueType>
class OptionPolicy{
private:
	std::vector<TypedOption<EnumType, ValueType>* > _options;
	std::map<std::string, EnumType> _name2type;
protected:
	void createOption(EnumType type, const std::string& name, const ValueType& lowerbound, const ValueType& upperbound, const ValueType& defaultValue, std::vector<std::string>& option2name);
	void createOption(EnumType type, const std::string& name, const std::set<ValueType>& values, const ValueType& defaultValue, std::vector<std::string>& option2name);
public:
	~OptionPolicy(){
		for(auto i=_options.begin(); i!=_options.end(); ++i) {
			delete(*i);
		}
	}
	bool isOption(const std::string& name) const{
		return _name2type.find(name)!=_name2type.end();
	}
	ValueType getValue(const std::string& name) const{
		assert(isOption(name));
		return _options.at(_name2type.at(name))->getValue();
	}
	ValueType getValue(EnumType option) const{
		return _options.at(option)->getValue();
	}
	void setStrValue(const std::string& name, const ValueType& value){
		assert(isOption(name));
		_options.at(_name2type.at(name))->setValue(value);
	}
	void setValue(EnumType type, const ValueType& value){
		_options.at(type)->setValue(value);
	}
	bool isAllowedValue(const std::string& name, const ValueType& value) const{
		return isOption(name) && _options.at(_name2type.at(name))->isAllowedValue(value);
	}
	std::string printOption(const std::string& name) const{
		return _options.at(_name2type.at(name))->printOption();
	}
	void addOptionStrings(std::vector<std::string>& optionlines) const {
		for(auto i=_options.begin(); i<_options.end(); ++i){
			optionlines.push_back((*i)->printOption());
		}
	}

	void copyValues(Options* opts);
};

typedef OptionPolicy<IntType, int> IntPol;
typedef OptionPolicy<BoolType, bool> BoolPol;
typedef OptionPolicy<DoubleType, double> DoublePol;
typedef OptionPolicy<StringType, std::string> StringPol;

/**
 * Class to represent a block of options
 */
class Options: public IntPol, public BoolPol, public DoublePol, public StringPol{
private:
	std::string		_name;	//!< The name of the options block
	ParseInfo		_pi;	//!< The place where the options were parsed

	std::vector<std::string> _option2name;

public:
	Options(const std::string& name, const ParseInfo& pi);
	~Options(){}

	const std::string&	name()	const	{ return _name;	}
	const ParseInfo&	pi()	const	{ return _pi;	}

	bool			isOption(const std::string&) const;

	bool			isStringOption(const std::string&) const;
	bool			isBoolOption(const std::string&) const;
	bool			isIntOption(const std::string&) const;
	bool			isDoubleOption(const std::string&) const;

	int				getIntValue(const std::string&) const;
	bool			getBoolValue(const std::string&) const;
	std::string 	getStringValue(const std::string&) const;
	double 			getDoubleValue(const std::string&) const;

	int getValue(IntType type){
		return OptionPolicy<IntType, int>::getValue(type);
	}
	double getValue(DoubleType type){
		return OptionPolicy<DoubleType, double>::getValue(type);
	}
	bool getValue(BoolType type){
		return OptionPolicy<BoolType, bool>::getValue(type);
	}
	std::string getValue(StringType type){
		return OptionPolicy<StringType, std::string>::getValue(type);
	}

	void setValue(IntType type, const int& value){
		OptionPolicy<IntType, int>::setValue(type, value);
	}
	void setValue(DoubleType type, const double& value){
		OptionPolicy<DoubleType, double>::setValue(type, value);
	}
	void setValue(BoolType type, const bool& value){
		OptionPolicy<BoolType, bool>::setValue(type, value);
	}
	void setValue(StringType type, const std::string& value){
		OptionPolicy<StringType, std::string>::setValue(type, value);
	}

	void			copyValues(Options*);

	std::string 	printAllowedValues	(const std::string& option) const;
	std::ostream&	put					(std::ostream&)	const;
	std::string		to_string			()			const;

	Language	language() const;

	// NOTE: do NOT call this code outside luaconnection or other user interface methods.
	void setValue(const std::string& name, const int& value){
		OptionPolicy<IntType, int>::setStrValue(name, value);
	}
	void setValue(const std::string& name, const double& value){
		OptionPolicy<DoubleType, double>::setStrValue(name, value);
	}
	void setValue(const std::string& name, const bool& value){
		OptionPolicy<BoolType, bool>::setStrValue(name, value);
	}
	void setValue(const std::string& name, const std::string& value){
		OptionPolicy<StringType, std::string>::setStrValue(name, value);
	}
	bool isAllowedValue(const std::string& name, const int& value){
		return OptionPolicy<IntType, int>::isAllowedValue(name, value);
	}
	bool isAllowedValue(const std::string& name, const double& value){
		return OptionPolicy<DoubleType, double>::isAllowedValue(name, value);
	}
	bool isAllowedValue(const std::string& name, const bool& value){
		return OptionPolicy<BoolType, bool>::isAllowedValue(name, value);
	}
	bool isAllowedValue(const std::string& name, const std::string& value){
		return OptionPolicy<StringType, std::string>::isAllowedValue(name, value);
	}
};

#endif
