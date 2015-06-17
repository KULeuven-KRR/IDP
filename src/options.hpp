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

#pragma once

#include "common.hpp"
#include "errorhandling/error.hpp"
#include "parseinfo.hpp"

// TODO enum class does not yet support comparison operators in 4.4.3

enum Language {/* TXT,*/
	IDP,
	IDP2,
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
	SYMMETRYBREAKING,
	PROVERCOMMAND,
	APPROXDEF,
	SOLVERHEURISTIC
};

enum IntType {
	NBMODELS,
	NRPROPSTEPS,
	LONGESTBRANCH,
	TIMEOUT,
	MXTIMEOUT,
	TIMEOUT_ENTAILMENT,
	MEMORYOUT,
	MXMEMORYOUT,
	RANDOMSEED,
	LAZYSIZETHRESHOLD,
	EXISTSEXPANSIONSTEPS,
	// DO NOT MIX verbosity and non-verbosity options!
	VERBOSE_CREATE_GROUNDERS,
	VERBOSE_GEN_AND_CHECK,
	VERBOSE_GROUNDING,
	VERBOSE_GROUNDING_STATISTICS,
	VERBOSE_SOLVING_STATISTICS,
	VERBOSE_TRANSFORMATIONS,
	VERBOSE_SOLVING,
	VERBOSE_PROPAGATING,
	VERBOSE_CREATE_PROPAGATORS,
	VERBOSE_QUERY,
	VERBOSE_FUNCDETECT,
	VERBOSE_ENTAILMENT,
	VERBOSE_DEFINITIONS,
	VERBOSE_APPROXDEF,
	VERBOSE_SYMMETRY,
	FIRST_VERBOSE = VERBOSE_CREATE_GROUNDERS, //IMPORTANT: this has to be the first of the verbosity options
	LAST_VERBOSE = VERBOSE_SYMMETRY //IMPORTANT: this has to be the last of the verbosity options
};

enum BoolType {
	SHOWWARNINGS,
	ASSUMECONSISTENTINPUT,
	TRACE,
	AUTOCOMPLETE,
	LONGNAMES,
	CPGROUNDATOMS,
	RELATIVEPROPAGATIONSTEPS,
	CREATETRANSLATION,
	MXRANDOMPOLARITYCHOICE,
	EXPANDIMMEDIATELY,
	TSEITINDELAY,
	SATISFIABILITYDELAY,
	GROUNDWITHBOUNDS,
	CPSUPPORT,
	SKOLEMIZE,
	ADDEQUALITYTHEORY,
	EXISTS_ONLYONELEFT_APPROX,
	SHAREDTSEITIN,
	LAZYHEURISTIC,
	WATCHEDRELEVANCE,
	LIFTEDUNITPROPAGATION,
	STABLESEMANTICS,
	REDUCEDGROUNDING,
	POSTPROCESS_DEFS,
	SPLIT_DEFS,
	JOIN_DEFS_FOR_XSB,
	GUARANTEE_NO_REC_NEG, //user guarantees that his theories have no recursion over negation
	PROVER_SUPPORTS_TFA,
	XSB_SHORT_NAMES,
	SHOW_XSB_WARNINGS,
	REFINE_DEFS_WITH_XSB,
	XSB
};

enum OptionType {
	VERBOSITY
	// IMPORTANT: the moment another one is added, OptionPolicy<OptionType, Options*>::copyValues(Options* opts) has to be adapted to iterate over all of them!
};

enum DoubleType {

};

enum class SolverHeuristic {
	CLASSIC,
	VMTF,
	FIRST = CLASSIC,
	LAST = VMTF
};


enum class SymmetryBreaking {
	NONE,
	STATIC,
	FIRST = NONE,
	LAST = STATIC
};

enum class ApproxDef {
	NONE,
	COMPLETE,				// TRUE DOWN and FALSE UP
	CHEAP_RULES_ONLY,		// TRUE DOWN and FALSE UP, without "expensive" rules (= rules with forall quantor in the body)
	STRATIFIED,				// First calculate CHEAP_RULES_ONLY, then calculate TRUE DOWN and FALSE UP for only the "expensive" rules
	ALL_POSSIBLE_RULES,		// TRUE DOWN, TRUE UP, FALSE DOWN, FALSE UP all at once
	FIRST = NONE,
	LAST = ALL_POSSIBLE_RULES
};

enum class PrintBehaviour {
	PRINT,
	DONOTPRINT
};

template<typename OptionType>
bool isVerbosityOption(OptionType) {
	return false;
}
template<>
bool isVerbosityOption<IntType>(IntType t);

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

template<class EnumType, class ConcreteType>
class AnyOption: public TypedOption<EnumType, ConcreteType> {
public:
	AnyOption(EnumType type, const std::string& name, PrintBehaviour visible)
			: 	TypedOption<EnumType, ConcreteType>(type, name, visible) {
	}

	bool isAllowedValue(const ConcreteType&) {
		return true;
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
			PrintBehaviour visible);
	void createOption(EnumType type, const std::string& name, const std::set<ValueType>& values, const ValueType& defaultValue,
			PrintBehaviour visible);
	void createOption(EnumType type, const std::string& name, const ValueType& defaultValue, PrintBehaviour visible);
public:
	~OptionPolicy() {
		for (auto option : _options) {
			if (option != NULL) {
				delete (option);
			}
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
		Assert((unsigned int)option<_options.size() && _options.at(option)!=NULL);
		//If this is not the case, check that you ask options through getOption, don't ask for them directly!!
		return _options.at(option)->getValue();
	}
	void setStrValue(const std::string& name, const ValueType& value) {
		Assert(isOption(name));
		setValue(_name2type.at(name), value);
	}
	void setValue(EnumType type, const ValueType& value) {
		Assert((unsigned int)type<_options.size() && _options.at(type)!=NULL);
		//If this is not the case, check that you ask options through getOption, don't ask for them directly!!
		_options.at(type)->setValue(value);
	}
	bool isAllowedValue(const std::string& name, const ValueType& value) const {
		return isOption(name) && _options.at(_name2type.at(name))->isAllowedValue(value);
	}
	std::string printOption(const std::string& name) const {
		Assert(isOption(name));
		return _options.at(_name2type.at(name))->printOption();
	}
	void addOptionStrings(std::vector<std::string>& optionlines) const {
		for (auto option : _options) {
			if (option != NULL) {
				// Check is necessary as not each optionblock has all options
				optionlines.push_back(option->printOption());
			}
		}
	}

	// NOTE mainly used to get all suboption blocks
	std::vector<ValueType> getOptionValues() const {
		std::vector<ValueType> values;
		for (auto option : _options) {
			if (option != NULL) {
				values.push_back(option->getValue());
			}
		}
		return values;
	}

	void copyValues(const OptionPolicy<EnumType, ValueType>& opts);
};

// Note: Makes sure users cannot set the XSB option when there is no XSB available.
template<>
inline void OptionPolicy<BoolType, bool>::setValue(BoolType type, const bool& value) {
#ifndef WITHXSB
	if (type == BoolType::XSB and value != false) {
		Warning::warning("XSB support is not available. Option xsb is ignored.\n");
		return;
	}
#endif

	_options.at(type)->setValue(value);
}

typedef OptionPolicy<IntType, int> IntPol;
typedef OptionPolicy<BoolType, bool> BoolPol;
typedef OptionPolicy<OptionType, Options*> OptionPol;
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
struct OptionValueTraits<Options*> {
	typedef OptionType EnumType;
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
struct OptionTypeTraits<OptionType> {
	typedef Options* ValueType;
};

template<>
struct OptionTypeTraits<StringType> {
	typedef std::string ValueType;
};

/**
 * Class to represent a block of options
 */
class Options: public IntPol, public BoolPol, public DoublePol, public StringPol, public OptionPol {
private:
	bool _isVerbosity;

public:
	Options();
	Options(bool verboseOptions);
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
	typename OptionTypeTraits<EnumType>::ValueType getValue(EnumType type) const {
		return OptionPolicy<EnumType, typename OptionTypeTraits<EnumType>::ValueType>::getValue(type);
	}

	template<class EnumType, class ValueType>
	void setValue(EnumType type, const ValueType& value) {
		OptionPolicy<EnumType, ValueType>::setValue(type, value);
	}
	bool isVerbosityBlock() {
		return _isVerbosity;
	}
	void copyValues(const Options& opts);

	std::string printAllowedValues(const std::string& option) const;
	std::ostream& put(std::ostream&) const;

	Language language() const;
	SymmetryBreaking symmetryBreaking() const;
	ApproxDef approxDef() const;
	SolverHeuristic solverHeuristic() const;

	// NOTE: do NOT call this code outside luaconnection or other user interface methods.
	template<class ValueType>
	void setValue(const std::string& name, const ValueType& value) {
		OptionPolicy<typename OptionValueTraits<ValueType>::EnumType, ValueType>::setStrValue(name, value);
	}

	template<class ValueType>
	bool isAllowedValue(const std::string& name, const ValueType& value) {
		return OptionPolicy<typename OptionValueTraits<ValueType>::EnumType, ValueType>::isAllowedValue(name, value);
	}

	std::vector<Options*> getSubOptionBlocks() const {
		return OptionPol::getOptionValues();
	}
};
