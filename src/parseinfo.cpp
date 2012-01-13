/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "term.hpp"
#include "theory.hpp"
using namespace std;

TermParseInfo::TermParseInfo(const TermParseInfo& p)
		: ParseInfo(p.line(), p.col(), p.file()), _original(0) {
	if (p.original())
		_original = p.original()->clone();
}

TermParseInfo TermParseInfo::clone() const {
	return TermParseInfo(_line, _col, _file, _original ? _original->clone() : 0);
}

TermParseInfo TermParseInfo::clone(const map<Variable*, Variable*>& mvv) const {
	return TermParseInfo(_line, _col, _file, _original ? _original->clone(mvv) : 0);
}

FormulaParseInfo::FormulaParseInfo(const FormulaParseInfo& p)
		: ParseInfo(p.line(), p.col(), p.file()), _original(0) {
	if (p.original())
		_original = p.original()->clone();
}

FormulaParseInfo FormulaParseInfo::clone() const {
	return FormulaParseInfo(_line, _col, _file, _original ? _original->clone() : 0);
}

FormulaParseInfo FormulaParseInfo::clone(const map<Variable*, Variable*>& mvv) const {
	return FormulaParseInfo(_line, _col, _file, _original ? _original->clone(mvv) : 0);
}

SetParseInfo::SetParseInfo(const SetParseInfo& p)
		: ParseInfo(p.line(), p.col(), p.file()), _original(0) {
	if (p.original())
		_original = p.original()->clone();
}

SetParseInfo SetParseInfo::clone() const {
	return SetParseInfo(_line, _col, _file, _original ? _original->clone() : 0);
}

SetParseInfo SetParseInfo::clone(const map<Variable*, Variable*>& mvv) const {
	return SetParseInfo(_line, _col, _file, _original ? _original->clone(mvv) : 0);
}

TermParseInfo::~TermParseInfo() {
	if (_original)
		_original->recursiveDelete();
}

FormulaParseInfo::~FormulaParseInfo() {
	if (_original)
		_original->recursiveDelete();
}

SetParseInfo::~SetParseInfo() {
	if (_original)
		_original->recursiveDelete();
}
