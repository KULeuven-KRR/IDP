/************************************
	parseinfo.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PARSEINFO_HPP
#define PARSEINFO_HPP

#include <string>

/**
 * \file parseinfo.hpp
 *
 *		This file contains the classes to represent error, warning, and debugging information 
 */

/***************************************
	Parse location of parsed objects	
***************************************/

/**
 *		A ParseInfo contains a line number, a column number and a file. 
 *
 *		Almost every object that can be written by a user has a ParseInfo object as attribute. 
 *		The ParseInfo stores where the object was parsed. This is used for producing precise error and warning messages.
 *		Objects that are not parsed (e.g., internally created variables) have line number 0 in their ParseInfo.
 */
class ParseInfo {

	private:
		unsigned int	_line;	//!< line number where the object is declared (0 for non-parsed objects)
		unsigned int	_col;	//!< column number where the object is declared
		std::string*	_file;	//!< file name where the object is declared. _file == 0 when parsed on stdin
	
	public:

		// Constructors
		ParseInfo() : _line(0), _col(0), _file(0) { }
		ParseInfo(unsigned int line, unsigned int col, std::string* file) : _line(line), _col(col), _file(file) { }
		ParseInfo(const ParseInfo& p) : _line(p.line()), _col(p.col()), _file(p.file()) { }

		// Destructor
		virtual ~ParseInfo() { }

		// Inspectors
		unsigned int	line()		const { return _line;		}	//!< Returns the line number
		unsigned int	col()		const { return _col;		}	//!< Returns the column number
		std::string*	file()		const { return _file;		}	//!< Returns the file name (0 for stdin)
		bool			isParsed()	const { return _line != 0;	}	//!< Returns true iff the object was declared by the user
};

class Formula;

/**
 *		ParseInfo for formulas.
 *		
 *		Besides the attributes of a ParseInfo object, it contains also the originally parsed formula
 *
 */
class FormulaParseInfo : public ParseInfo {

	private:
		Formula*	_original;	//!< The original formula written by the user
								//!< Null-pointer when associated to an internally created formula with no
								//!< corresponding original formula

	public:

		// Constructors
		FormulaParseInfo() : ParseInfo(), _original(0) { }
		FormulaParseInfo(unsigned int line, unsigned int col, std::string* file, Formula* orig) : 
			ParseInfo(line,col,file), _original(orig) { }
		FormulaParseInfo(const FormulaParseInfo& p) : ParseInfo(p.line(),p.col(),p.file()), _original(p.original()) { }

		// Destructor
		~FormulaParseInfo() { }

		// Inspectors
		Formula*	original()	const { return _original;	}	//!< Returns the original formula

};

#endif
