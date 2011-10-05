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

enum OptionType{
	LANGUAGE, MODELFORMAT,
	SATVERBOSITY, GROUNDVERBOSITY, PROPAGATEVERBOSITY, NRMODELS, NRPROPSTEPS, LONGESTBRANCH, SYMMETRY, PROVERTIMEOUT,
	PRINTTYPES, CPSUPPORT, TRACE, AUTOCOMPLETE, LONGNAMES, RELATIVEPROPAGATIONSTEPS, CREATETRANSLATION
};

template<class ConcreteType>
class TypedOption{
private:
	const std::string name;
	ConcreteType chosenvalue_;

public:
	TypedOption(const std::string& name): name(name){}
	~TypedOption(){}

	const std::string& getName() const { return name; }

	virtual bool 		isAllowedValue(const ConcreteType& value) = 0;
	virtual std::string printOption() const = 0;

	const ConcreteType&	getValue() const { return chosenvalue_; }
	void setValue(const ConcreteType& chosenvalue){
		assert(isAllowedValue(chosenvalue));
		chosenvalue_ = chosenvalue;
	}
};

template<class ConcreteType>
class RangeOption: public TypedOption<ConcreteType> {
private:
	ConcreteType lowerbound_, upperbound_;

	const ConcreteType&	lower() const { return lowerbound_;  }
	const ConcreteType&	upper() const { return upperbound_;  }

public:
	RangeOption(const std::string& name, const ConcreteType& lowerbound, const ConcreteType& upperbound)
		: TypedOption<ConcreteType>(name), lowerbound_(lowerbound), upperbound_(upperbound) { }

	bool isAllowedValue(const ConcreteType& value){
		return value >= lower() && value <= upper();
	}

	virtual std::string printOption() const {
		std::stringstream ss; // TODO name
		ss <<"<name>" <<" lies between " <<lower() <<" and " <<upper() <<". Current value is " <<TypedOption<ConcreteType>::getValue() <<"\n";
		return ss.str();
	}
};

template<class ConcreteType>
class EnumeratedOption: public TypedOption<ConcreteType> {
private:
	std::set<ConcreteType>	allowedvalues_;
	const std::set<ConcreteType>& getAllowedValues() const { return allowedvalues_; }

public:
	EnumeratedOption(const std::string& name, const std::set<ConcreteType>& allowedvalues)
		: TypedOption<ConcreteType>(name), allowedvalues_(allowedvalues) { }

	bool isAllowedValue(const ConcreteType& value){
		return getAllowedValues().find(value)!=getAllowedValues().end();
	}

	virtual std::string printOption() const {
		std::stringstream ss;
		ss <<"<name>" <<" is one of "; // TODO name
		for(auto i=getAllowedValues().begin(); i!=getAllowedValues().end(); ++i){
			ss <<*i <<", ";
		}
		ss <<". Current value is " <<TypedOption<ConcreteType>::getValue() <<"\n";
		return ss.str();
	}
};

template<class List>
bool hasOption(const List& list, OptionType type){
	return list.size()>type && list[type]!=NULL;
}

/**
 * Class to represent a block of options
 */
class Options{
	private:
		std::string		_name;	//!< The name of the options block
		ParseInfo		_pi;	//!< The place where the options were parsed

		std::vector<TypedOption<int>* > _intoptions;
		std::vector<TypedOption<bool>* > _booloptions;
		std::vector<TypedOption<std::string>* > _stringoptions;

		std::vector<std::string> _option2name;
		std::map<std::string, OptionType> _name2optionType;

		template<class ConcreteType>
		std::vector<TypedOption<ConcreteType>* >& getOptions(const ConcreteType& value);

		template<class ConcreteType>
		void createOption(OptionType type, const std::string& name, const ConcreteType& lowerbound, const ConcreteType& upperbound, const ConcreteType& defaultValue);
		template<class ConcreteType>
		void createOption(OptionType type, const std::string& name, const std::set<ConcreteType>& values, const ConcreteType& defaultValue);

	public:
		Options(const std::string& name, const ParseInfo& pi);
		~Options();

		const std::string&	name()	const	{ return _name;	}
		const ParseInfo&	pi()	const	{ return _pi;	}

		template<class ValueType>
		void setValue(const std::string& name, ValueType value){
			OptionType type = _name2optionType.at(name);
			assert(hasOption(getOptions(value), type));
			getOptions(value).at(type)->setValue(value);
		}
		template<class ValueType>
		bool isAllowedValue(const std::string& name, ValueType value){
			OptionType type = _name2optionType.at(name);
			assert(hasOption(getOptions(value), type));
			return getOptions(value).at(type)->isAllowedValue(value);
		}

		bool			isOption(const std::string&) const;

		bool			isStringOption(const std::string&) const;
		bool			isBoolOption(const std::string&) const;
		bool			isIntOption(const std::string&) const;

		int				getIntValue(const std::string&) const;
		bool			getBoolValue(const std::string&) const;
		std::string 	getStringValue(const std::string&) const;

		void			copyValues(Options*);

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
