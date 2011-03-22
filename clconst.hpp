/************************************
	clconst.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CLCONST_HPP
#define CLCONST_HPP

#include <string>

/************************************************
	class to represent the different 
	types of constants set at the command line
	(only used by the lexical analyser)
************************************************/

class CLConst {
	public:
		virtual int execute() const = 0;	// Set yylval to the value of the constant
};

// Integers
class IntClConst : public CLConst {
	private:
		int	_value;
	public:
		IntClConst(int v) : _value(v) { }
		int execute() const { yylval.nmr = _value; return INTEGER;		}
};

// Doubles
class DoubleClConst : public CLConst {
	private:
		double _value;
	public:
		DoubleClConst(double d) : _value(d) { }
		int execute() const { yylval.dou = _value; return FLNUMBER;	}
};

// Chars
class CharCLConst : public CLConst {
	private:
		char _value;
		bool _cons;
	public:
		CharCLConst(char c, bool b) : _value(c), _cons(b) { }
		int execute() const { yylval.chr = _value; return _cons ? CHARCONS : CHARACTER;	}
};

// Strings
class StrClConst : public CLConst {
	private:
		std::string	_value;
		bool	_cons;
	public:
		StrClConst(std::string s, bool b) : _value(s), _cons(b) { }
		int execute() const { yylval.str = new std::string(_value); return _cons ? STRINGCONS : IDENTIFIER;	}
		std::string value() const { return _value;	}
};

#endif
