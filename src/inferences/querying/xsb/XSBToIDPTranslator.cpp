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

#include <memory>
#include <utility>
#include "Assert.hpp"

#include "XSBToIDPTranslator.hpp"
#include "common.hpp"
#include "vocabulary/vocabulary.hpp"
#include "structure/DomainElement.hpp"
#include "structure/DomainElementFactory.hpp"
#include "FormulaClause.hpp"

using std::string;
using std::stringstream;


// ATTENTION!
// when changing this prefix, also adapt the XSB "built-in" predicates
// that IDP provides in data/shared/std/xsb_compiler.P
// It is generally also a good idea to search the code for hard-coded use
// of these predicates and adapt them.
#ifndef IDPXSB_PREFIX
#define IDPXSB_PREFIX "ix"
#endif

#ifndef IDPXSB_CAPS_PREFIX
#define IDPXSB_CAPS_PREFIX "IX"
#endif

bool XSBToIDPTranslator::isoperator(int c) {
	return c == '*' ||
			c == '(' ||
			c == ')' ||
			c == '/' ||
			c == '-' ||
			c == '+' ||
			c == '=' ||
			c == '%' ||
			c == '<' ||
			c == '>' ||
			c == '^' ||
			c == '.';		// Dot for floating point numbers
}

bool XSBToIDPTranslator::isXSBBuiltIn(std::string str) {
	return isXSBNumber(str);
}

bool XSBToIDPTranslator::isXSBNumber(std::string str) {
	bool isNumber = true;
	for (auto i = str.begin(); i != str.end() && isNumber; ++i) {
		if (!isdigit(*i) && !isoperator(*i)) {
			isNumber = false;
		}
	}
	return isNumber;
}

// Note: It is important that each of these strings are present as
// predicates in data/share/std/xsb_compiler.P, accompanied of the
// IDPXSB_PREFIX.
bool XSBToIDPTranslator::isXSBCompilerSupported(const PFSymbol* symbol) {
	return is(symbol,STDFUNC::ABS);
}
bool XSBToIDPTranslator::isXSBCompilerSupported(const Sort* sort) {
	return sort == get(STDSORT::INTSORT) ||
		sort == get(STDSORT::NATSORT) ||
		sort == get(STDSORT::FLOATSORT);
}

string XSBToIDPTranslator::to_prolog_term(const PFSymbol* symbol) {
	if(is(symbol,STDFUNC::ABS)) {
		return get_abs_term_name();
	}
	if(symbol->builtin()) {
		// When translating to XSB, it does not matter for builtin symbols which
		// namespace they are in or which types of arguments they get since they
		// need to be mapped to the same XSB built-in anyway
		return to_prolog_term(symbol->nameNoArity());
	}
	return to_prolog_term(symbol->fqn_name());
}

string XSBToIDPTranslator::to_prolog_term(string str) {
	for (auto it = _termnames.cbegin(); it != _termnames.cend(); ++it) {
		if ((*it).first == str) {
			return (*it).second;
		}
		Assert((*it).second != transform_into_term_name(str)); // Value that this str will map to may not already be mapped to!
	}
	auto ret = transform_into_term_name(str);
	_termnames[str] = ret;
	return ret;
}

string XSBToIDPTranslator::transform_into_term_name(string str) {
	if (isXSBBuiltIn(str)) {
		return str;
	} else {
		stringstream ss;
		ss << IDPXSB_PREFIX << "_" << getGlobal()->getNewID() << "_" << to_simple_chars(str);
		return ss.str();
	}
}

string XSBToIDPTranslator::to_idp_pfsymbol(string str) {
	auto it = _termnames.find(str);
	if (it == _termnames.end()) {
		return str;
	} else {
		return it->second;
	}
}

string XSBToIDPTranslator::to_prolog_pred_and_arity(const PFSymbol* symbol) {
	return to_prolog_term(symbol).append("/").append(toString(symbol->sorts().size()));
}

string XSBToIDPTranslator::to_prolog_pred_and_arity(const Sort* sort) {
	// Sorts always have arity 1
	return to_prolog_sortname(sort).append("/1");
}

string XSBToIDPTranslator::to_prolog_term(const DomainElement* domelem) {
	auto str = toString(domelem);
	string ret;
	if(domelem->type() == DomainElementType::DET_INT ||
			domelem->type() == DomainElementType::DET_DOUBLE) {
		_domainels[str] = domelem;
		ret = str;
	} else {
		// filter the string
		stringstream s;
		s << IDPXSB_PREFIX << to_simple_chars(str);
		ret = s.str();
		_domainels[ret] = domelem;
	}
	return ret;
}

const DomainElement* XSBToIDPTranslator::to_idp_domelem(string str) {
	auto it = _domainels.find(str);
	if (it == _domainels.end()) {
		return createDomElem(str);
	}
	return it->second;
}

string XSBToIDPTranslator::to_prolog_term(CompType c) {
	string str;
	switch (c) {
	case CompType::EQ:
		str = "=";
		break;
	case CompType::NEQ:
		str = "!=";
		break;
	case CompType::LT:
		str = "<";
		break;
	case CompType::GT:
		str = ">";
		break;
	case CompType::LEQ:
		str = "<=";
		break;
	case CompType::GEQ:
		str = ">=";
		break;
	default:
		break;
	}
	return str;
}

string XSBToIDPTranslator::to_prolog_term(AggFunction af) {
	string str;
	switch (af) {
	case AggFunction::CARD:
		str = "ixcard";
		break;
	case AggFunction::SUM:
		str = "ixsum";
		break;
	case AggFunction::PROD:
		str = "ixprod";
		break;
	case AggFunction::MIN:
		str = "ixmin";
		break;
	case AggFunction::MAX:
		str = "ixmax";
		break;
	default:
		break;
	}
	return str;
}

string XSBToIDPTranslator::to_simple_chars(string str) {
	stringstream s;
	for (auto i = str.begin(); i != str.end(); ++i) {
		if (isalnum(*i)) {
			s << *i;
		} else {
			unsigned int tmp = *i;
			s << "x" << tmp << "x";
		}
	}
	return s.str();
}

string XSBToIDPTranslator::to_prolog_varname(string str) {
	stringstream s;
	s << IDPXSB_CAPS_PREFIX << to_simple_chars(str);
	return s.str();
}

string XSBToIDPTranslator::to_prolog_sortname(const Sort* sort) {
	if (isXSBCompilerSupported(sort)) {
		std::stringstream ss;
		ss << IDPXSB_PREFIX << sort->name();
		return ss.str();
	} else {
		return to_prolog_term(to_simple_chars(sort->name()));
	}
}

string XSBToIDPTranslator::get_idp_prefix() {
	return IDPXSB_PREFIX;
}

string XSBToIDPTranslator::get_idp_caps_prefix() {
	return IDPXSB_CAPS_PREFIX;
}

string XSBToIDPTranslator::get_forall_term_name() {
	std::stringstream ss;
	ss << IDPXSB_PREFIX << "forall";
	return ss.str();
}

string XSBToIDPTranslator::get_abs_term_name() {
	std::stringstream ss;
	ss << IDPXSB_PREFIX << "abs";
	return ss.str();
}

PrologVariable* XSBToIDPTranslator::create(std::string name, std::string type) {
	std::stringstream ss;
	ss << name << type;
	std::string str = ss.str();

	auto v = vars[str];
	if (v == NULL) {
		v = new PrologVariable(name, type);
		vars[str] = v;
	}
	return v;

}

std::set<PrologVariable*> XSBToIDPTranslator::prologVars(const varset& vars) {
	std::set<PrologVariable*> list;
	for (auto var = vars.begin(); var != vars.end(); ++var) {
		list.insert(create((*var)->name(), (*var)->sort()->name()));
	}
	return list;
}
