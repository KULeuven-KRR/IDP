#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <cstring>
#include "xsbinterface.hpp"
#include "compiler.hpp"
#include "common.hpp"
#include "GlobalData.hpp"
#include "theory/TheoryUtils.hpp"
#include "structure/Structure.hpp"
#include "theory/term.hpp"

#include <cinterf.h>

using namespace std;

XSBInterface* xsb_instance = NULL;

XSBInterface* XSBInterface::instance() {
	if (xsb_instance == NULL) {
		xsb_instance = new XSBInterface();
	}
	return xsb_instance;
}

void XSBInterface::setStructure(Structure* structure){
	_pp = new PrologProgram(structure);
	_structure = structure;
}

std::list<string> split(std::string sentence) {
	istringstream iss(sentence);
	std::list<string> tokens;

	stringstream ss(sentence); // Insert the string into a stream
	while (ss >> sentence) {
		tokens.push_back(sentence);
	}
	return tokens;
}

PrologTerm atom2term(PredForm* pf) {
	PrologTerm term(pf->symbol()->name());
	for (auto el = pf->args().begin(); el != pf->args().end(); ++el) {
		auto cnst = toString(*el);
		term.addArgument(new PrologConstant(cnst));
	}
	return term;
}

ElementTuple atom2tuple(PredForm* pf, Structure* s) {
	ElementTuple tuple;
	for (auto it = pf->args().begin(); it != pf->args().end(); ++it) {

		auto el = s->inter(dynamic_cast<FuncTerm*>(*it)->function())->funcTable()->operator [](ElementTuple());
		tuple.push_back(createDomElem(domainelement_idp(toString(el))));
	}
	return tuple;
}

PrologTerm* symbol2term(PFSymbol* symbol) {
	auto term = new PrologTerm(symbol->name());
	int idlength = symbol->nrSorts() / 10 + 1 + 1;
	for (uint i = 0; i < symbol->nrSorts(); ++i) {
		std::stringstream ss;
		ss <<"X" <<i;
		term->addArgument(PrologVariable::create(ss.str()));
	}
	return term;
}

PrologTerm* atom2term(PFSymbol* symbol, ElementTuple el) {
	auto term = new PrologTerm(symbol->name());
	for (auto i = el.begin(); i != el.end(); ++i) {
		term->addArgument(new PrologConstant(toString(*i)));
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
	FormulaUtils::graphFuncsAndAggs(&theory, _structure, {}, true, false /*TODO check*/);
	FormulaUtils::removeEquivalences(&theory);
	FormulaUtils::splitComparisonChains(&theory, theory.vocabulary());
	FormulaUtils::pushNegations(&theory);
	FormulaUtils::flatten(&theory);
	_pp->addDefinition(cloned_definition);
	auto str = _pp->getCode();
	auto str3 = _pp->getRanges();
	auto str2 = _pp->getFacts();
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 3) {
		clog << "The transformation to XSB resulted in the following code\n\n%Rules\n" << str << "\n%Facts\n" << str2 << "\n%Ranges\n" << str3 << "\n";
	}
	sendToXSB(str3, false);
	sendToXSB(str2, true);
	sendToXSB(str, false);
}

void XSBInterface::sendToXSB(string str, bool load) {
	auto name = tmpnam(NULL);

	ofstream tmp;
	tmp.open(name);
	tmp << str;
	tmp.close();
	stringstream ss;
	if (load) {
		ss << "load_dync('" << name << "').\n";
	} else {
		ss << "load_dyn('" << name << "').\n";
	}
	commandCall(ss.str());
	remove(name);
}

void XSBInterface::reset() {
	for(auto pred : _pp->allPredicates()) {
		stringstream ss;
		ss << "abolish(" << pred << ").\n";
		commandCall(ss.str());
	}
	commandCall("abolish_all_tables.\n");
}

void XSBInterface::exit() {
	xsb_close();
	delete(xsb_instance);
	xsb_instance = NULL;
}

SortedElementTable XSBInterface::queryDefinition(PFSymbol* s) {
	auto term = symbol2term(s);
	SortedElementTable result;
	XSB_StrDefine (buff);
	stringstream ss;
	ss << *term << ".";
	auto query = new char[ss.str().size() + 1];
	strcpy(query, ss.str().c_str());
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 5) {
		clog << "Quering XSB with: " <<  query << "\n";
	}
	auto rc = xsb_query_string_string(query, &buff, " ");
	handleResult(rc);

	while (rc == XSB_SUCCESS) {
		std::list<string> answer = split(buff.string);
		ElementTuple tuple;
		for (auto it = answer.begin(); it != answer.end(); ++it) {
			tuple.push_back(createDomElem(domainelement_idp(*it)));
		}
		result.insert(tuple);

		rc = xsb_next_string(&buff, " ");
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

bool XSBInterface::query(PFSymbol* s, ElementTuple t) {
	auto term = atom2term(s, t);

	XSB_StrDefine (buff);

	stringstream ss;
	ss << *term << ".";
	auto query = new char[ss.str().size() + 1];
	strcpy(query, ss.str().c_str());
	auto rc = xsb_query_string_string(query, &buff, " ");
	handleResult(rc);

	bool result = false;
	if (rc == XSB_SUCCESS) {
		result = true;
	}

	XSB_StrDestroy(&buff);

	return result;
}
