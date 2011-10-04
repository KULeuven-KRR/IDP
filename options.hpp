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

class InternalArgument;

// TODO enum class does not yet support comparison operators in 4.4.3

enum Language { TXT, IDP, ECNF, LATEX, ASP, CNF, TPTP };
enum Format { THREEVALUED, ALL, TWOVALUED};

enum OptionType{
	LANGUAGE, MODELFORMAT,
	SATVERBOSITY, GROUNDVERBOSITY, PROPAGATEVERBOSITY, NRMODELS, NRPROPSTEPS, LONGESTBRANCH, SYMMETRY, PROVERTIMEOUT,
	PRINTTYPES, CPSUPPORT, TRACE, AUTOCOMPLETE, LONGNAMES, RELATIVEPROPAGATIONSTEPS, CREATETRANSLATION
};

template<class TypeEnum, class ConcreteType>
class TypedOption{
private:
	ConcreteType chosenvalue_;

public:
	TypedOption(){}
	~TypedOption(){}

	virtual bool 		isAllowedValue(const ConcreteType& value) = 0;
	virtual std::string printOption() const = 0;

	const ConcreteType&	getValue() const { return chosenvalue_; }
	void setValue(const ConcreteType& chosenvalue){
		assert(isAllowedValue(chosenvalue));
		chosenvalue_ = chosenvalue;
	}
};

template<class TypeEnum, class ConcreteType>
class RangeOption: public TypedOption<TypeEnum, ConcreteType> {
private:
	ConcreteType lowerbound_, upperbound_;

	const ConcreteType&	lower() const { return lowerbound_;  }
	const ConcreteType&	upper() const { return upperbound_;  }

public:
	RangeOption(const ConcreteType& lowerbound, const ConcreteType& upperbound)
		: lowerbound_(lowerbound), upperbound_(upperbound) { }

	bool isAllowedValue(const ConcreteType& value){
		return value >= lower() && value <= upper();
	}

	virtual std::string printOption() const {
		std::stringstream ss; // TODO name
		ss <<"<name>" <<" lies between " <<lower() <<" and " <<upper() <<". Current value is " <<TypedOption<TypeEnum, ConcreteType>::getValue() <<"\n";
		return ss.str();
	}
};

template<class TypeEnum, class ConcreteType>
class EnumeratedOption: public TypedOption<TypeEnum, ConcreteType> {
private:
	std::set<ConcreteType>	allowedvalues_;
	const std::set<ConcreteType>& getAllowedValues() const { return allowedvalues_; }

public:
	EnumeratedOption(const std::set<ConcreteType>& allowedvalues): allowedvalues_(allowedvalues) { }

	bool isAllowedValue(const ConcreteType& value){
		return getAllowedValues().find(value)!=getAllowedValues().end();
	}

	virtual std::string printOption() const {
		std::stringstream ss;
		ss <<"<name>" <<" is one of "; // TODO name
		for(auto i=getAllowedValues().begin(); i!=getAllowedValues().end(); ++i){
			ss <<*i <<", ";
		}
		ss <<". Current value is " <<TypedOption<TypeEnum, ConcreteType>::getValue() <<"\n";
		return ss.str();
	}
};

/**
 * Class to represent a block of options
 */
class Options{
	private:
		std::string		_name;	//!< The name of the options block
		ParseInfo		_pi;	//!< The place where the options were parsed

		std::map<OptionType,TypedOption<OptionType, int>* > _intoptions;
		std::map<OptionType,TypedOption<OptionType, bool>* > _booloptions;
		std::map<OptionType,TypedOption<OptionType, std::string>* > _stringoptions;

		template<class ConcreteType>
		std::map<OptionType,TypedOption<OptionType, ConcreteType>* >& getOptions(const ConcreteType& value);

		std::map<std::string, OptionType> _name2optionType;
		std::map<OptionType, std::string> _optionType2name;

		template<class ConcreteType>
		void createOption(OptionType option, const std::string& name, const ConcreteType& lowerbound, const ConcreteType& upperbound, const ConcreteType& defaultValue){
			_name2optionType[name] = option;
			_optionType2name[option] = name;
			auto newoption = new RangeOption<OptionType, ConcreteType>(lowerbound, upperbound);
			newoption->setValue(defaultValue);
			getOptions(defaultValue)[option] =  newoption;
		}
		template<class ConcreteType>
		void createOption(OptionType option, const std::string& name, const std::set<ConcreteType>& values, const ConcreteType& defaultValue){
			_name2optionType[name] = option;
			_optionType2name[option] = name;
			auto newoption = new EnumeratedOption<OptionType, ConcreteType>(values);
			newoption->setValue(defaultValue);
			getOptions(defaultValue)[option] = newoption;
		}

	public:
		Options(const std::string& name, const ParseInfo& pi);
		~Options();

		InternalArgument getValue(OptionType name) const;
		InternalArgument getValue(const std::string& name) const;

		template<class ValueType>
		void setValue(const std::string& name, ValueType value){
			setValue(_name2optionType.at(name), value);
		}
		template<class ValueType>
		void setValue(OptionType type, ValueType value){
			getOptions(value).at(type)->setValue(value);
		}

		template<class ValueType>
		bool isAllowedValue(const std::string& name, ValueType value){
			return isAllowedValue(_name2optionType.at(name), value);
		}
		template<class ValueType>
		bool isAllowedValue(OptionType option, ValueType value){
			auto it = getOptions(value).find(option);
			if(it==getOptions(value).end()){
				return false;
			}
			return it->second->isAllowedValue(value);
		}

		const std::string&	name()	const	{ return _name;	}
		const ParseInfo&	pi()	const	{ return _pi;	}

		bool	isOption(const std::string&) const;
		void	copyValues(Options*);

		std::string 	printAllowedValues	(const std::string& option) const;
		std::ostream&	put					(std::ostream&)	const;
		std::string		to_string			()			const;

		Language	language()				const;
		bool		printtypes()			const;
		bool		autocomplete()			const;
		bool		cpsupport()				const;
		int			nrmodels()				const;
		int			satverbosity()			const;
		int			groundverbosity()		const;
		int			propagateverbosity()	const;
		int			nrpropsteps()			const;
		int			provertimeout()			const;
		bool		trace()					const;
		bool		longnames()				const;
		bool		relativepropsteps()		const;
		bool 		writeTranslation() 		const;
		int			longestbranch()			const;
		int			symmetry()				const;
};

#endif
