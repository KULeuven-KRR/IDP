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

#include "utils/StringUtils.hpp"
#include "XSBToIDPTranslator.hpp"
#include "common.hpp"
#include "vocabulary/vocabulary.hpp"
#include "structure/DomainElement.hpp"
#include "structure/DomainElementFactory.hpp"
#include "FormulaClause.hpp"

using std::string;
using std::list;
using std::stringstream;

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

bool XSBToIDPTranslator::isXSBIntegerNumber(std::string str) {
	bool isNumber = true;
	auto start = str.begin();
	if (*start == '-') {
		start++;
	}
	for (auto i = start; i != str.end() && isNumber; ++i) {
		if (!isdigit(*i)) {
			isNumber = false;
		}
	}
	return isNumber;
}

std::string XSBToIDPTranslator::toIntegerNumberString(std::string str) const {
	Assert(isDouble(str));
	std::stringstream ss;
	auto start = str.begin();
	if (*start == '-') {
		ss << *start;
		start++;
	}
	for (auto i = start; i != str.end(); ++i) {
		if (!isdigit(*i)) {
			return ss.str();
		}
		ss << *i;
	}
}

// Note: It is important that each of these strings are present as
// predicates in data/share/std/xsb_compiler.P, accompanied of the
// IDPXSB_PREFIX.
bool XSBToIDPTranslator::isXSBCompilerSupported(const PFSymbol* symbol) {
	for (auto name2sort : Vocabulary::std()->getSorts()) {
		if (symbol == name2sort.second->pred()) {
			return true;
		}
	}
	return is(symbol,STDFUNC::ABS);
}
bool XSBToIDPTranslator::isXSBCompilerSupported(const Sort* sort) {
	return sort == get(STDSORT::INTSORT) ||
		sort == get(STDSORT::NATSORT) ||
		sort == get(STDSORT::FLOATSORT) ||
		sort == get(STDSORT::STRINGSORT) ||
		sort == get(STDSORT::CHARSORT);
}

string XSBToIDPTranslator::to_prolog_term(const PFSymbol* symbol) {
	auto found = _pfsymbols_to_prolog_string.find(symbol);
	if (found != _pfsymbols_to_prolog_string.end()) {
		return found->second;
	}
	string ret;
	if (isXSBCompilerSupported(symbol)) {
		stringstream ss;
		ss << get_idp_prefix() << symbol->nameNoArity();
		ret = ss.str();
	} else if (symbol->isNonConstructorBuiltin()) {
		// When translating to XSB, it does not matter for comparison symbols or
		// built-in functions which namespace they are in or which types of arguments
		// they get since they need to be mapped to the same XSB built-in anyway
		// Also, the pointer cannot be added because this causes translation to the
		// XSB Built-in string to go wrong.
		ret = make_into_prolog_term_name(symbol->nameNoArity());
	} else {
		stringstream ss;
		ss << symbol->fqn_name() << symbol;
		ret = make_into_prolog_term_name(ss.str());
	}
	add_to_mappings(symbol,ret);
	return ret;
}

void XSBToIDPTranslator::add_to_mappings(const PFSymbol* symbol, std::string str) {
	Assert(isXSBBuiltIn(str) or _prolog_string_to_pfsymbols.find(str) == _prolog_string_to_pfsymbols.end());
	_prolog_string_to_pfsymbols.insert({str,symbol});
	Assert(_pfsymbols_to_prolog_string.find(symbol) == _pfsymbols_to_prolog_string.end());
	_pfsymbols_to_prolog_string.insert({symbol,str});
}

void XSBToIDPTranslator::add_to_mappings(const DomainElement* domelem, std::string str) {
	Assert(_prolog_string_to_domainels.find(str) == _prolog_string_to_domainels.end());
	_prolog_string_to_domainels.insert({str,domelem});
#ifdef DEBUG
	for (auto it = _domainels_to_prolog_string.cbegin(); it != _domainels_to_prolog_string.cend(); ++it) {
		Assert((*it).second != make_into_prolog_term_name(str)); // Value that this str will map to may not already be mapped to!
	}
#endif
	Assert(_domainels_to_prolog_string.find(domelem) == _domainels_to_prolog_string.end());
	_domainels_to_prolog_string.insert({domelem,str});
}


string XSBToIDPTranslator::make_into_prolog_term_name(string str) {
	if (isXSBBuiltIn(str)) {
		return str;
	} else if (getOption(BoolType::XSB_SHORT_NAMES)) {
		stringstream ss;
		ss << new_pred_name();
		return ss.str();
	} else {
		stringstream ss;
		ss << get_idp_prefix() << "_" << getNewID() << "_" << to_simple_chars(str);
		return ss.str();
	}
}

const PFSymbol* XSBToIDPTranslator::to_idp_pfsymbol(string str) {
	auto it = _prolog_string_to_pfsymbols.find(str);
	Assert(it != _prolog_string_to_pfsymbols.end());
	return it->second;
}

string XSBToIDPTranslator::to_prolog_pred_and_arity(const PFSymbol* symbol) {
	return to_prolog_term(symbol).append("/").append(toString(symbol->sorts().size()));
}

string XSBToIDPTranslator::to_prolog_pred_and_arity(const Sort* sort) {
	// Sorts always have arity 1
	return to_prolog_sortname(sort).append("/1");
}

string XSBToIDPTranslator::to_prolog_term(const DomainElement* domelem) {
	auto found = _domainels_to_prolog_string.find(domelem);
	if (found != _domainels_to_prolog_string.end()) {
		return found->second;
	}
	auto str = toString(domelem);
	string ret;
	if(domelem->type() == DomainElementType::DET_INT ||
			domelem->type() == DomainElementType::DET_DOUBLE) {
		ret = str;
	} else {
		// filter the string
		stringstream s;
		s << make_into_prolog_term_name(to_simple_chars(str));
		ret = s.str();
	}
	add_to_mappings(domelem,ret);
	return ret;
}

const DomainElement* XSBToIDPTranslator::to_idp_domelem(string str, Sort* sort) {
	auto it = _prolog_string_to_domainels.find(str);
	if (it != _prolog_string_to_domainels.end()) {
		return it->second;
	}
	const DomainElement* ret;
	if (sort->isConstructed()) {
		auto ctor = split(str,"(")[0];
		list<string> args = {};
		if (ctor.length() < str.length()) { // only if there arguments
			Assert(split(str,")")[0].length() != str.length()); // assert: a ')' has to occur in the string
			auto allargs = string(str,ctor.length() + 1,str.length()-2-ctor.length());
			stringstream ss;
			int nestings = 0;
			for (auto ch : allargs) {
				if (ch == '(') { nestings++; }
				if (ch == ')') { nestings--; }
				if (ch == ',' and nestings == 0) {
					args.push_back(ss.str());
					ss.str(string()); // empty contents of stringstream
			} else {
				ss << ch;
			}
		}
		Assert(nestings == 0);
		args.push_back(ss.str());
		}
		auto sortctor = _prolog_string_to_pfsymbols[ctor];
		Assert(isa<Function>(*sortctor));
		const Function* constructorfunction = (Function*) sortctor;
		Function* f = const_cast<Function*>(constructorfunction);
		auto elemtuple = to_idp_elementtuple(args,f);
		ret = createDomElem(createCompound(f,elemtuple));
	} else { 
		// Create a domain element. Since everything XSB gives back is a string, we inspect the string format as well as the expected Sort
		if (isXSBIntegerNumber(str)) { // If 
			ret = createDomElem((int)(toDouble(str)));
		} else if (SortUtils::isSubsort(sort,get(STDSORT::INTSORT))) {
			return to_idp_domelem(toIntegerNumberString(str),sort);
		} else if (isXSBNumber(str)) {
			ret = createDomElem(toDouble(str), NumType::CERTAINLYNOTINT);
		} else { 
			ret = createDomElem(str);
		}
	}
	add_to_mappings(ret,str);
	return ret;
}

bool XSBToIDPTranslator::isValidArg(std::list<std::string> answers, const PFSymbol* symbol) {
	if (VocabularyUtils::isConstructorFunction(symbol)) {
		return answers.size() == symbol->nrSorts() - 1;
	} else {
		return answers.size() == symbol->nrSorts();
	}
}


ElementTuple XSBToIDPTranslator::to_idp_elementtuple(list<string> answers, const PFSymbol* symbol) {
	Assert(isValidArg(answers,symbol));
	ElementTuple ret = {};
	int argnr = 0;
	for (auto answer : answers) {
		auto ans = (to_idp_domelem(answer,symbol->sorts()[argnr]));
		ret.push_back(ans);
		argnr++;
	}
	
	return ret;
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

std::string XSBToIDPTranslator::to_xsb_truth_type(TruthValue tv) {
	string str;
	switch (tv) {
	case TruthValue::True:
		str = "true";
		break;
	case TruthValue::Unknown:
		str = "undefined";
		break;
	case TruthValue::False:
		str = "false";
		break;
	default:
		throw IdpException("Invalid code path.");
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
	s << get_idp_caps_prefix() << to_simple_chars(str);
	return s.str();
}

string XSBToIDPTranslator::to_prolog_sortname(const Sort* sort) {
	return to_prolog_term(sort->pred());
}


// ATTENTION!
// when changing this prefix, also adapt the XSB "built-in" predicates
// that IDP provides in data/shared/std/xsb_compiler.P
// It is generally also a good idea to search the code for hard-coded use
// of these predicates and adapt them.
string XSBToIDPTranslator::get_idp_prefix() {
	return "ix";
}

string XSBToIDPTranslator::get_short_idp_prefix() {
	return "x";
}

string XSBToIDPTranslator::get_idp_caps_prefix() {
	return "IX";
}

string XSBToIDPTranslator::get_forall_term_name() {
	std::stringstream ss;
	ss << get_idp_prefix() << "forall";
	return ss.str();
}

string XSBToIDPTranslator::get_twovalued_findall_term_name() {
	std::stringstream ss;
	ss << "findall";
	return ss.str();
}

string XSBToIDPTranslator::get_threevalued_findall_term_name() {
	std::stringstream ss;
	ss << get_idp_prefix() << "threeval_findall";
	return ss.str();
}

string XSBToIDPTranslator::get_division_term_name() {
	std::stringstream ss;
	ss << get_idp_prefix() << "division";
	return ss.str();
}

string XSBToIDPTranslator::get_exponential_term_name() {
	std::stringstream ss;
	ss << get_idp_prefix() << "exponential";
	return ss.str();
}

string XSBToIDPTranslator::new_pred_name_with_prefix(string str) {
	if (getOption(BoolType::XSB_SHORT_NAMES)) {
		stringstream ss;
		ss << new_pred_name();
		return ss.str();
	} else {
		stringstream ss;
		ss << str << getNewID();
		return ss.str();
	}
}

string XSBToIDPTranslator::new_pred_name() {
	stringstream ss;
	ss << get_short_idp_prefix() << getNewID();
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
