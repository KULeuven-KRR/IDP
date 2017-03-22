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

#pragma once

#include <unordered_map>
#include <list>

#include "commontypes.hpp"
#include "vocabulary/VarCompare.hpp"

class PrologTerm;
class DomainElement;
class PFSymbol;
class Sort;
class PrologVariable;

class XSBToIDPTranslator {

private:
	std::unordered_map<std::string, const DomainElement*> _domainels;
	std::unordered_map<std::string, std::string> _termnames;
	unsigned int _predicate_name_counter;
	bool isValidArg(std::list<std::string>, const PFSymbol*);

	std::string transform_into_term_name(std::string);

	unsigned int getNewID() {
		return ++_predicate_name_counter;
	}

public:
	XSBToIDPTranslator() :
		_predicate_name_counter(1) {}

//  These 4 procedures decide whether a given string represents an
//	XSB built-in or an XSB predicate that is supported in the
//	predefined XSB code for IDP found in data/share/std/xsb_compiler.P
	bool isoperator(int c);
	bool isXSBNumber(std::string str);
	bool isXSBBuiltIn(std::string str);
	bool isXSBCompilerSupported(const PFSymbol* symbol);
	bool isXSBCompilerSupported(const Sort*);

	std::string to_prolog_term(const PFSymbol*);
	std::string to_prolog_term(const std::string);
	std::string to_idp_pfsymbol(std::string);
	std::string to_prolog_pred_and_arity(const PFSymbol*);
	std::string to_prolog_pred_and_arity(const Sort*);

	std::string to_prolog_term(const DomainElement*);
	const DomainElement* to_idp_domelem(std::string);
	const DomainElement* to_idp_domelem(std::string, Sort*);
	ElementTuple to_idp_elementtuple(std::list<std::string>, PFSymbol*);

	std::string to_prolog_term(CompType);
	std::string to_prolog_term(AggFunction);
	std::string to_xsb_truth_type(TruthValue);

	std::string to_prolog_varname(std::string);
	std::string to_prolog_sortname(const Sort*);
	static std::string to_simple_chars(std::string);

	static std::string get_idp_prefix();
	static std::string get_short_idp_prefix();
	static std::string get_idp_caps_prefix();
	static std::string get_forall_term_name();
	static std::string get_twovalued_findall_term_name();
	static std::string get_threevalued_findall_term_name();
	static std::string get_division_term_name();
	static std::string get_exponential_term_name();

	std::string new_pred_name_with_prefix(std::string);
	std::string new_pred_name();

private:
	std::map<std::string, PrologVariable*> vars;
public:
	PrologVariable* create(std::string name, std::string type = "");
	std::set<PrologVariable*> prologVars(const varset&);
};
