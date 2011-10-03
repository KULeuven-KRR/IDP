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
#include "parseinfo.hpp"

class InternalArgument;

template<class OptionValue> class RangeOption {
private:
	OptionValue chosenvalue_, lowerbound_, upperbound_;

	const OptionValue&	lower() const { return lowerbound_;  }
	const OptionValue&	upper() const { return upperbound_;  }

public:
	RangeOption(OptionValue lowerbound, OptionValue upperbound, OptionValue chosenvalue) : chosenvalue_(chosenvalue), lowerbound_(lowerbound), upperbound_(upperbound) { }
	bool isAllowedValue(OptionValue value){
		return value >= lower() && value <= upper();
	}
	void setValue(OptionValue chosenvalue){
		assert(isAllowedValue(value));
		chosenvalue_ = chosenvalue;
	}
	const OptionValue&	value()	const { return chosenvalue_; }
};

typedef RangeOption<int> IntOption;
typedef RangeOption<float> FloatOption;

template<class OptionValue> class EnumeratedOption {
private:
	OptionValue 				chosenvalue_;
	std::vector<OptionValue>	allowedvalues_;
public:
	EnumeratedOption(const std::vector<OptionValue>& allowedvalues, const OptionValue& chosenvalue): allowedvalues_(allowedvalues) {
		setValue(chosenvalue);
	}
	const OptionValue& value()	const;
	void setValue(const std::string& val);
	virtual std::string	getPossibleValues() const;
};

enum Language { LAN_TXT, LAN_IDP, LAN_ECNF, LAN_LATEX, LAN_ASP, LAN_CNF, LAN_TPTP };

/**
 * Class to represent a block of options
 */
class Options {
	private:
		std::string		_name;	//!< The name of the options block
		ParseInfo		_pi;	//!< The place where the options were parsed

		std::map<std::string,bool>			_booloptions;	//!< Options that have a boolean value
		std::map<std::string,IntOption*>	_intoptions;	//!< Options that have an integer value
		std::map<std::string,FloatOption*>	_floatoptions;	//!< Options that have a floating point number value
		std::map<std::string,StringOption*>	_stringoptions;	//!< Options that have a string value

	public:
		Options(const std::string& name, const ParseInfo& pi);
		~Options();

		InternalArgument	getvalue(const std::string&)		const;

		const std::string&	name()							const	{ return _name;	}
		const ParseInfo&	pi()							const	{ return _pi;	}

		const std::map<std::string,bool>&			booloptions()		const { return _booloptions;	}
		const std::map<std::string,IntOption*>&		intoptions()		const { return _intoptions;		}
		const std::map<std::string,FloatOption*>&	floatoptions()		const { return _floatoptions;	}
		const std::map<std::string,StringOption*>&	stringoptions()		const { return _stringoptions;	}

		bool	isoption(const std::string&) const;
		void	setvalues(Options*);
		bool	setvalue(const std::string&,bool);
		bool	setvalue(const std::string&,int);
		bool	setvalue(const std::string&,double);
		bool	setvalue(const std::string&,const std::string&);

		std::string getPossibleValues(const std::string& option) const;

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

		std::ostream&	put(std::ostream&)	const;
		std::string		to_string()			const;	

};

#endif
