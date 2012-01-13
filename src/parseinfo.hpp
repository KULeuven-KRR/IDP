/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef PARSEINFO_HPP
#define PARSEINFO_HPP

#include <string>
#include <map>

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
protected:
	unsigned int _line; //!< line number where the object is declared (0 for non-parsed objects)
	unsigned int _col; //!< column number where the object is declared
	std::string* _file; //!< file name where the object is declared. _file == 0 when parsed on stdin

public:

	// Constructors
	ParseInfo()
			: _line(0), _col(0), _file(0) {
	}
	ParseInfo(unsigned int line, unsigned int col, std::string* file)
			: _line(line), _col(col), _file(file) {
	}
	ParseInfo(const ParseInfo& p)
			: _line(p.line()), _col(p.col()), _file(p.file()) {
	}

	// Destructor
	virtual ~ParseInfo() {
	}

	// Inspectors
	unsigned int line() const {
		return _line;
	} //!< Returns the line number
	unsigned int col() const {
		return _col;
	} //!< Returns the column number
	std::string* file() const {
		return _file;
	} //!< Returns the file name (0 for stdin)
	bool isParsed() const {
		return _line != 0;
	} //!< Returns true iff the object was declared by the user
};

class Variable;
class Term;

/**
 *		ParseInfo for terms.
 *
 *		Besides the attributes of a ParseInfo object, it contains also the originally parsed term
 */
class TermParseInfo: public ParseInfo {
private:
	Term* _original; //!< The original term written by the user
					 //!< Null-pointer when associated to an internally created formula with no
					 //!< corresponding original formula

public:

	// Constructors
	TermParseInfo()
			: ParseInfo(), _original(0) {
	}
	TermParseInfo(unsigned int line, unsigned int col, std::string* file, Term* orig)
			: ParseInfo(line, col, file), _original(orig) {
	}
	TermParseInfo(const TermParseInfo& p);

	TermParseInfo clone() const;
	TermParseInfo clone(const std::map<Variable*, Variable*>&) const;

	// Destructor
	~TermParseInfo();

	// Inspectors
	Term* original() const {
		return _original;
	} //!< Returns the original formula

};

class SetExpr;

/**
 *		ParseInfo for sets.
 *
 *		Besides the attributes of a ParseInfo object, it contains also the originally parsed set
 */
class SetParseInfo: public ParseInfo {
private:
	SetExpr* _original; //!< The original term written by the user
						//!< Null-pointer when associated to an internally created formula with no
						//!< corresponding original formula

public:

	// Constructors
	SetParseInfo()
			: ParseInfo(), _original(0) {
	}
	SetParseInfo(unsigned int line, unsigned int col, std::string* file, SetExpr* orig)
			: ParseInfo(line, col, file), _original(orig) {
	}
	SetParseInfo(const SetParseInfo& p);

	SetParseInfo clone() const;
	SetParseInfo clone(const std::map<Variable*, Variable*>&) const;

	// Destructor
	~SetParseInfo();

	// Inspectors
	SetExpr* original() const {
		return _original;
	} //!< Returns the original formula

};

class Formula;

/**
 *		ParseInfo for formulas.
 *		
 *		Besides the attributes of a ParseInfo object, it contains also the originally parsed formula
 *
 */
class FormulaParseInfo: public ParseInfo {

private:
	Formula* _original; //!< The original formula written by the user
						//!< Null-pointer when associated to an internally created formula with no
						//!< corresponding original formula

public:

	// Constructors
	FormulaParseInfo()
			: ParseInfo(), _original(0) {
	}
	FormulaParseInfo(unsigned int line, unsigned int col, std::string* file, Formula* orig)
			: ParseInfo(line, col, file), _original(orig) {
	}
	FormulaParseInfo(const FormulaParseInfo& p);

	FormulaParseInfo clone() const;
	FormulaParseInfo clone(const std::map<Variable*, Variable*>&) const;

	// Destructor
	~FormulaParseInfo();

	// Inspectors
	Formula* original() const {
		return _original;
	} //!< Returns the original formula

};

#endif
