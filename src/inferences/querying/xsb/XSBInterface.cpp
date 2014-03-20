#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <cstring>
#include "XSBInterface.hpp"
#include "PrologProgram.hpp"
#include "FormulaClause.hpp"
#include "XSBToIDPTranslator.hpp"
#include "common.hpp"
#include "GlobalData.hpp"
#include "theory/TheoryUtils.hpp"
#include "structure/Structure.hpp"
#include "vocabulary/vocabulary.hpp"
#include "theory/theory.hpp"
#include "theory/term.hpp"

#include <cinterf.h>

using std::stringstream;
using std::ofstream;
using std::clog;

extern void setIDPSignalHanders();

XSBInterface* interface_instance = NULL;

XSBInterface* XSBInterface::instance() {
	if (interface_instance == NULL) {
		interface_instance = new XSBInterface();
	}
	return interface_instance;
}

void XSBInterface::setStructure(Structure* structure){
	// TODO: delete possible previous PrologProgram?
	_pp = new PrologProgram(structure,_translator);
	_structure = structure;
}

std::list<string> split(std::string sentence) {
	std::istringstream iss(sentence);
	std::list<string> tokens;

	stringstream ss(sentence); // Insert the string into a stream
	while (ss >> sentence) {
		tokens.push_back(sentence);
	}
	return tokens;
}

//ElementTuple XSBInterface::atom2tuple(PredForm* pf, Structure* s) {
//	ElementTuple tuple;
//	for (auto it = pf->args().begin(); it != pf->args().end(); ++it) {
//		auto el = s->inter(dynamic_cast<FuncTerm*>(*it)->function())->funcTable()->operator [](ElementTuple());
//		tuple.push_back(createDomElem(_translator->to_idp_domelem(toString(el))));
//	}
//	return tuple;
//}

PrologTerm* XSBInterface::symbol2term(const PFSymbol* symbol) {
	auto term = new PrologTerm(_translator->to_prolog_term(symbol));
	for (uint i = 0; i < symbol->nrSorts(); ++i) {
		std::stringstream ss;
		ss <<"X" <<i;
		term->addArgument(_translator->create(ss.str()));
	}
	return term;
}

void XSBInterface::commandCall(const std::string& command) {
	auto checkvalue = xsb_command_string(const_cast<char*>(command.c_str()));
	handleResult(checkvalue);
}

void XSBInterface::handleResult(int xsb_status){
	if(xsb_status==XSB_ERROR){
		stringstream ss;
		ss <<"Error in XSB: " << xsb_get_error_message();
		throw InternalIdpException(ss.str());
	}
}

XSBInterface::XSBInterface() {
	_pp = NULL;
	_structure = NULL;
	_translator = new XSBToIDPTranslator();
	stringstream ss;
	ss << getInstallDirectoryPath() << XSB_INSTALL_URL << " -n --quietload";
	auto checkvalue = xsb_init_string(const_cast<char*>(ss.str().c_str()));
	handleResult(checkvalue);
	commandCall("[basics].");
	stringstream ss2;
	ss2 << "consult('" << getInstallDirectoryPath() << "/share/std/xsb_compiler.P').";
	commandCall(ss2.str());
}

void XSBInterface::loadDefinition(Definition* d) {
	auto cloned_definition = d->clone();
	Theory theory("", _structure->vocabulary(), ParseInfo());
	theory.add(cloned_definition);
	FormulaUtils::unnestFuncsAndAggs(&theory, _structure);
	FormulaUtils::graphFuncsAndAggs(&theory, _structure, cloned_definition->defsymbols(), true, false);
	FormulaUtils::removeEquivalences(&theory);
	FormulaUtils::pushNegations(&theory);
	FormulaUtils::flatten(&theory);
	_pp->setDefinition(cloned_definition);
	//TODO: Not really a reason anymore to generate code separate from facts and ranges, since "facts" now also possibly contain P :- tnot(P) rules
	auto str = _pp->getCode();
	auto str2 = _pp->getFacts();
	auto str3 = _pp->getRanges();
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 3) {
		clog << "The transformation to XSB resulted in the following code\n\n%Rules\n" << str << "\n%Facts\n" << str2 << "\n%Ranges\n" << str3 << "\n";
	}
	sendToXSB(str3);
	sendToXSB(str2);
	sendToXSB(str);
}

void XSBInterface::sendToXSB(string str) {
	auto name = tmpnam(NULL);

	ofstream tmp;
	tmp.open(name);
	tmp << str;
	tmp.close();
	stringstream ss;
	ss << "load_dyn('" << name << "').\n";
	commandCall(ss.str());
	remove(name);
}

void XSBInterface::reset() {
	commandCall("abolish_all_tables.\n");
	for(auto pred : _pp->allPredicates()) {
		stringstream ss;
		ss << "abolish(" << pred << ").\n";
		commandCall(ss.str());
	}
	delete(_translator);
	_translator = new XSBToIDPTranslator();
	_structure = NULL;
	delete(_pp);
	_pp = NULL;
	setIDPSignalHanders();
}

void XSBInterface::exit() {
	xsb_close();
	delete(interface_instance);
	interface_instance = NULL;
}

SortedElementTable XSBInterface::queryDefinition(PFSymbol* s, TruthValue tv) {
	auto term = symbol2term(s);
	SortedElementTable result;
	XSB_StrDefine (buff);
	stringstream ss;
	ss << "call_tv(" << *term << "," << _translator->to_xsb_truth_type(tv) << ").";
	auto query = new char[ss.str().size() + 1];
	strcpy(query, ss.str().c_str());
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 5) {
		clog << "Quering XSB with: " <<  query << "\n";
	}
    char* delimiter = new char [strlen(" ") + 1];
    strcpy(delimiter," ");
	auto rc = xsb_query_string_string(query, &buff, delimiter);
	handleResult(rc);

	while (rc == XSB_SUCCESS) {
		std::list<string> answer = split(buff.string);
		ElementTuple tuple;
		for (auto it = answer.begin(); it != answer.end(); ++it) {
			tuple.push_back(_translator->to_idp_domelem(*it));
		}
		result.insert(tuple);

		rc = xsb_next_string(&buff, delimiter);
		handleResult(rc);
	}
	XSB_StrDestroy(&buff);

	delete (term);

	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 5) {
		clog << "Resulted in the following answer tuples:\n";
		if (result.empty()) {
			clog << "<none>\n\n";
		} else {
			for(auto elem : result) {
				clog << toString(elem) << "\n";
			}
			clog << "\n";
		}
	}
	return result;
}

//bool XSBInterface::query(PFSymbol* s, ElementTuple t) {
//	auto term = atom2term(s, t);
//
//	XSB_StrDefine (buff);
//
//	stringstream ss;
//	ss << *term << ".";
//	auto query = new char[ss.str().size() + 1];
//	strcpy(query, ss.str().c_str());
//	auto rc = xsb_query_string_string(query, &buff, " ");
//	handleResult(rc);
//
//	bool result = false;
//	if (rc == XSB_SUCCESS) {
//		result = true;
//	}
//
//	XSB_StrDestroy(&buff);
//
//	return result;
//}
//
//PrologTerm* XSBInterface::atom2term(PredForm* pf) {
//	auto term = new PrologTerm(_translator->to_prolog_term(pf->symbol()));
//	for (auto el = pf->args().begin(); el != pf->args().end(); ++el) {
//		auto cnst = toString(*el);
//		term->addArgument(new PrologConstant(cnst));
//	}
//	return term;
//}
//
//PrologTerm* XSBInterface::atom2term(PFSymbol* symbol, ElementTuple el) {
//	auto term = new PrologTerm(_translator->to_prolog_term(symbol));
//	for (auto i = el.begin(); i != el.end(); ++i) {
//		term->addArgument(new PrologConstant(toString(*i)));
//	}
//	return term;
//}
