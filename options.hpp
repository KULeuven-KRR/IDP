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
		int		value()	const;
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
		double	value()	const;
};

/**
 * A single option that has an string value
 */
class StringOption {
	public:
		virtual bool				value(const std::string& v)	= 0;
		virtual const std::string&	value()						const = 0;
};

/**
 * Class to represent a block of options
 */
class Options {
	private:
		std::string		_name;	//!< The name of the options block
		ParseInfo		_pi;	//!< The place where the options were parsed

		std::map<std::string,bool>			_booloptions;	//!< Options that have a boolean value
		std::map<std::string,IntOption*>		_intoptions;	//!< Options that have an integer value
		std::map<std::string,FloatOption*>	_floatoptions;	//!< Options that have a floating point number value
		std::map<std::string,StringOption*>	_stringoptions;	//!< Options that have a string value

	public:

		Options(const std::string& name, const ParseInfo& pi);

		InternalArgument	getvalue(const std::string&)	const;
		const std::string&	name()							const	{ return _name;	}
		const ParseInfo&	pi()							const	{ return _pi;	}

		bool	setvalue(const std::string&,bool);
		bool	setvalue(const std::string&,int);
		bool	setvalue(const std::string&,double);
		bool	setvalue(const std::string&,const std::string&);
};

#endif
