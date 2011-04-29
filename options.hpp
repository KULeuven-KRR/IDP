/************************************
	options.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>
#include <map>
#include "parseinfo.hpp"

struct InternalArgument;

/**
 * A single option that has an integer value
 */
class IntOption {
	private:
		int		_value;
		int		_lower;
		int		_upper;
	public:
		IntOption(int l, int u, int v) : _value(v), _lower(l), _upper(u) { }
		bool	value(int v);
		int		value()	const { return _value;	}
};

/**
 * A single option that has an floating point value
 */
class FloatOption {
	private:
		double	_value;
		double	_lower;
		double	_upper;
	public:
		FloatOption(double l, double u, double v) : _value(v), _lower(l), _upper(u) { }
		bool	value(double v);
		double	value()	const { return _value;	}
};

/**
 * A single option that has an string value
 */
class StringOption {
	public:
		virtual ~StringOption() { }
		virtual bool				value(const std::string& v)	= 0;
		virtual const std::string&	value()						const = 0;
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

		Language	language()			const;
		bool		printtypes()		const;
		bool		cpsupport()			const;
		int			nrmodels()			const;
		int			satverbosity()		const;
		int			groundverbosity()	const;

		std::ostream&	put(std::ostream&)	const;
		std::string		to_string()			const;	

};

#endif
