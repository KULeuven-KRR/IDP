/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef PARSEINFO_HPP
#define PARSEINFO_HPP

#include <string>
#include <memory>
#include "Assert.hpp"

/**
 *	A ParseInfo contains a line number, a column number and a file.
 *
 *	Almost every object that can be written by a user has a ParseInfo object as attribute.
 *	The ParseInfo stores where the object was parsed. This is used for producing precise error and warning messages.
 *	Objects that are not parsed (e.g., internally created variables) have line number 0 in their ParseInfo.
 */
class ParseInfo {
protected:
	unsigned int _line; //!< line number where the object is declared (0 for non-parsed objects)
	unsigned int _col; //!< column number where the object is declared
	std::string* _file; //!< file name where the object is declared. _file == 0 when parsed on stdin

public:
	ParseInfo()
			: _line(0), _col(0), _file(0) {
	}
	ParseInfo(unsigned int line, unsigned int col, std::string* file)
			: _line(line), _col(col), _file(file) {
	}

	virtual ~ParseInfo() {
	}

	bool userDefined() const {
		return _line != 0;
	}

	unsigned int linenumber() const {
		return _line;
	}
	unsigned int columnnumber() const {
		return _col;
	}
	std::string* filename() const {
		return _file;
	}

	std::ostream& put(std::ostream& stream) const{
		if (_file!=NULL) {
			stream << *(_file) <<":" <<_line <<":" <<_col;
		}
		return stream;
	}
};

template<typename RepresentedObject>
class ObjectInfo: public ParseInfo {
protected:
	std::shared_ptr<RepresentedObject> _original; //!< The original term written by the user, NULL if it was internal

public:
	ObjectInfo()
			: ParseInfo(), _original() {
	}
	ObjectInfo(unsigned int line, unsigned int col, std::string* file, const RepresentedObject& original)
			: ParseInfo(line, col, file), _original(std::shared_ptr<RepresentedObject>(original.clone())) {
		_original->allwaysDeleteRecursively(true);
	}

	const RepresentedObject& originalobject() const {
		Assert(_original.get()!=NULL);
		return *_original;
	}
};

class Term;
class SetExpr;
class Formula;
typedef ObjectInfo<Term> TermParseInfo;
typedef ObjectInfo<SetExpr> SetParseInfo;
typedef ObjectInfo<Formula> FormulaParseInfo;

#endif
