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
#include <map>
#include "parseinfo.hpp"

class InternalArgument;

template<class T> class Option {
private:
	T		_value;
	T		_lower;
	T		_upper;
public:
	Option(T l, T u, T v) : _value(v), _lower(l), _upper(u) { }
	bool	value(int v);
	T		value()	const { return _value;	}
	T		lower() const { return _lower; }
	T		upper() const { return _upper; }
};

typedef Option<int> IntOption;
typedef Option<float> FloatOption;

/**
 * A single option that has an string value
 */
class StringOption {
	public:
		virtual ~StringOption() { }
		virtual bool				value(const std::string& v)	= 0;
		virtual const std::string&	value()					const = 0;

		virtual std::string					getPossibleValues() const;
};

class EnumeratedStringOption : public StringOption {
	private:
		unsigned int	_value;
		std::vector<std::string>	_possvalues;
	public:
		EnumeratedStringOption(const std::vector<std::string>& possvalues, const std::string& val);
		~EnumeratedStringOption() { }
		const std::string& value()	const;
		bool value(const std::string& val);
		virtual std::string	getPossibleValues() const;
};

enum Language { LAN_TXT, LAN_IDP, LAN_ECNF, LAN_LATEX, LAN_ASP, LAN_CNF };

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
		bool		trace()					const;
		bool		longnames()				const;
		bool		relativepropsteps()		const;
		bool 		writeTranslation() 		const;
		int			longestbranch()			const;

		std::ostream&	put(std::ostream&)	const;
		std::string		to_string()			const;	

};

#endif
