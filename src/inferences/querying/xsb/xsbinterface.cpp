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
#include "structure/AbstractStructure.hpp"
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

void XSBInterface::setStructure(AbstractStructure* structure){
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

ElementTuple atom2tuple(PredForm* pf, AbstractStructure* s) {
	ElementTuple tuple;
	for (auto it = pf->args().begin(); it != pf->args().end(); ++it) {

		auto el = s->inter(dynamic_cast<FuncTerm*>(*it)->function())->funcTable()->operator [](ElementTuple());
		auto string = StringPointer(domainelement_idp(toString(el)));
		//			cout << toString(createDomElem(&string)) << endl;
		tuple.push_back(createDomElem(string));
	}
	return tuple;
}

PrologTerm* symbol2term(PFSymbol* symbol) {
	auto term = new PrologTerm(symbol->name());
	int idlength = symbol->nrSorts() / 10 + 1 + 1;
	for (auto i = 0; i < symbol->nrSorts(); ++i) {
		char varname[idlength];
		sprintf(varname, "X%d", i);
		term->addArgument(PrologVariable::create(varname));
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

XSBInterface::XSBInterface() {
	_pp = NULL;
	_structure = NULL;
	stringstream ss;
	ss << getInstallDirectoryPath() << XSB_INSTALL_URL << " -n --quietload";
	cerr <<"OUTPUT " <<ss.str() <<"\n";
	xsb_init_string(const_cast<char*>(ss.str().c_str()));
	xsb_command_string("[basics].");
	stringstream ss2;
	ss2 << "consult('" << getInstallDirectoryPath() << "/share/std/xsb_compiler.P').";
	xsb_command_string(const_cast<char*>(ss2.str().c_str()));
}

void XSBInterface::loadDefinition(Definition* d) {
	Theory theory("", _structure->vocabulary(), ParseInfo());
	theory.add(d);
	FormulaUtils::unnestFuncsAndAggs(&theory, _structure);
	FormulaUtils::graphFuncsAndAggs(&theory, _structure, true, false /*TODO check*/);
	FormulaUtils::removeEquivalences(&theory);
	FormulaUtils::splitComparisonChains(&theory, theory.vocabulary());
	FormulaUtils::pushNegations(&theory);
	FormulaUtils::flatten(&theory);
	_pp->addDefinition(d);
//	cout << toString(theory);
	auto str = _pp->getCode();
	auto str3 = _pp->getRanges();
	auto str2 = _pp->getFacts();
//	cout << "CODE:\n\n" << endl;
//	cout << str << endl;
//	cout << "FACTS:\n\n" << endl;
//	cout << str2 << endl;
//	cout << "RANGES:\n\n" << endl;
//	cout << str3 << endl;
	this->sendToXSB(str3, false);
	this->sendToXSB(str2, true);
	this->sendToXSB(str, false);
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
	xsb_command_string(const_cast<char*>(ss.str().c_str()));
	remove(name);
}

void XSBInterface::reset() {
	for(auto pred : _pp->allPredicates()) {
		stringstream ss;
		ss << "abolish(" << pred << ").\n";
		xsb_command_string(const_cast<char*>(ss.str().c_str()));
	}
	stringstream ss2;
	ss2 << "abolish_all_tables.\n";
	xsb_command_string(const_cast<char*>(ss2.str().c_str()));
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
	int rc = xsb_query_string_string(query, &buff, " ");
	while (rc == XSB_SUCCESS) {
		std::list<string> answer = split(buff.string);
		ElementTuple tuple;
		for (auto it = answer.begin(); it != answer.end(); ++it) {
			auto string = StringPointer(domainelement_idp(*it));
			tuple.push_back(createDomElem(string));
		}
		result.insert(tuple);

		rc = xsb_next_string(&buff, " ");
	}
	XSB_StrDestroy(&buff);

	delete (term);
	return result;
}

bool XSBInterface::query(PFSymbol* s, ElementTuple t) {

	auto term = atom2term(s, t);

	XSB_StrDefine (buff);

	stringstream ss;
	ss << *term << ".";
	auto query = new char[ss.str().size() + 1];
	strcpy(query, ss.str().c_str());
	int rc = xsb_query_string_string(query, &buff, " ");

	bool result = false;
	if (rc == XSB_SUCCESS) {
		result = true;
	}

	XSB_StrDestroy(&buff);

	return result;
}
