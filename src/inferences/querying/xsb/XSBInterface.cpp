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
	// Set warnings to not be printed
	if (not getOption(SHOW_XSB_WARNINGS)) {
		stringstream ss1;
		ss1 << "set_prolog_flag(warning_action,silent_warning).";
		commandCall(ss1.str());
	}
}

void XSBInterface::load(const Definition* d, Structure* structure) {
	// TODO: delete possible previous PrologProgram?
	_pp = new PrologProgram(structure,_translator);
	_structure = structure;
	auto cloned_definition = d->clone();
	auto theory = new Theory("", _structure->vocabulary(), ParseInfo());
	theory->add(cloned_definition);
	FormulaUtils::unnestFuncsAndAggs(theory, _structure);
	FormulaUtils::graphFuncsAndAggs(theory, _structure, cloned_definition->defsymbols(), true, false);
	FormulaUtils::removeEquivalences(theory);
	FormulaUtils::pushNegations(theory);
	FormulaUtils::flatten(theory);
	_pp->setDefinition(cloned_definition->clone());
	theory->recursiveDelete(); // memory management - delete everything of the temp. theory
	//TODO: Not really a reason anymore to generate code separate from facts and ranges, since "facts" now also possibly contain P :- tnot(P) rules
	auto str2 = _pp->getFacts();
	auto str = _pp->getCode();
	auto str3 = _pp->getRanges();
	auto compiler = PrologProgram::getCompilerCode();

	// TODO: lost the optimisation of using load_dyn/2 when facts are two-valued, refactoring code is in order to realise this again
	// Currently all 3 files are loaded into XSB at the same time always, but this should be just when the facts are three-valued.
	// In this way, the table declarations are joined into one file
	stringstream ss;
	ss << compiler << "\n%Rules\n" << str << "\n%Facts\n" << str2 << "\n%Ranges\n" << str3;
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 3) {
		clog << "The transformation to XSB resulted in the following code:\n\n" << ss.str() << "\n";
	}
	sendToXSB(ss.str());
}

void XSBInterface::sendToXSB(string str) {
	auto name = GlobalData::instance()->getTempFileName();
	try {
		ofstream tmp;
		tmp.open(name);
		tmp << str;
		tmp.close();
		stringstream ss;
		ss << "load_dyn('" << name << "').\n";
		commandCall(ss.str());
	} catch (const Exception& ex) {
		stringstream ss;
		ss << "Exception caught: " << ex.getMessage();
		Error::error(ss.str());
		clog.flush();
	}
	GlobalData::instance()->removeTempFile(name);
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
	delete[] (delimiter);
	delete[] (query);

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

bool XSBInterface::hasUnknowns(PFSymbol* s) {
	auto term = symbol2term(s);
	XSB_StrDefine (buff);
	stringstream ss;
	ss << "call_tv(" << *term << ",undefined).";
	auto query = new char[ss.str().size() + 1];
	strcpy(query, ss.str().c_str());
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 5) {
		clog << "To determine whether there XSB program is non-total, quering XSB with: " <<  query << "\n";
	}
	char* delimiter = new char [strlen(" ") + 1];
	strcpy(delimiter," ");
	auto rc = xsb_query_string_string(query, &buff, delimiter);
	handleResult(rc);
	auto hasUnknowns = (rc == XSB_SUCCESS);
	while (rc == XSB_SUCCESS) {
		rc = xsb_next_string(&buff, delimiter);
		handleResult(rc);
	}
	XSB_StrDestroy(&buff);

	delete (term);
	delete[] (delimiter);
	delete[] (query);

	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 5) {
		if (hasUnknowns) {
			clog << "Resulted in at least one answer (the XSB program is non-total).\n";
		} else {
			clog << "Resulted in no answers (the XSB program is total).\n";
		}
	}
	return hasUnknowns;
}
