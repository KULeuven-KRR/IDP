/************************************
	parseinfo.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PARSEINFO_HPP
#define PARSEINFO_HPP

#include <string>

class Formula;

/***************************************
	Parse location of parsed objects	
***************************************/

/**
 *		A ParseInfo contains a line number, a column number and a file. 
 *
 *		Almost every object that can be written by a user has a ParseInfo object as attribute. 
 *		The ParseInfo stores where the object was parsed. This is used for producing precise error and warning messages.
 *		Objects that are not parsed (e.g., internally created variables) have line number 0 in their ParseInfo.
 *
 */
class ParseInfo {
	private:
		unsigned int	_line;		// line number where the object is declared (0 for non-parsed objects)
		unsigned int	_column;	// column number where the object is declared
		std::string*	_file;		// file name where the object is declared
									// NOTE: _file == 0 when parsed on stdin
	
	public:
		// Constructors
		ParseInfo() : _line(0), _column(0), _file(0) { }
		ParseInfo(unsigned int line, unsigned int column, std::string* file) : _line(line), _column(column), _file(file) { }
		ParseInfo(const ParseInfo& p) : _line(p.line()), _column(p.column()), _file(p.file()) { }

		// Destructor
		virtual ~ParseInfo() { }

		// Inspectors
		unsigned int	line()		const { return _line;		}
		unsigned int	column()	const { return _column;		}
		std::string*	file()		const { return _file;		}
		bool			isParsed()	const { return _line != 0;	}
};

/**
 *		ParseInfo for formulas.
 *		
 *		Besides the attributes of a ParseInfo object, it contains also the originally parsed formula
 *
 */
class FormParseInfo : public ParseInfo {
	private:
		Formula*	_original;	// The original formula written by the user
								// Null-pointer when associated to an internally created formula with no
								// corresponding original formula

	public:
		// Constructors
		FormParseInfo() : ParseInfo(), _original(0) { }
		FormParseInfo(unsigned int line, unsigned int column, std::string* file, Formula* orig) : 
			ParseInfo(line,column,file), _original(orig) { }
		FormParseInfo(const FormParseInfo& p) : ParseInfo(p.line(),p.column(),p.file()), _original(p.original()) { }

		// Destructor
		~FormParseInfo() { }

		// Inspectors
		Formula*	original()	const { return _original;	}
};

#endif
