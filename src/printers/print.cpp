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

#include "IncludeComponents.hpp"
#include "options.hpp"
#include "monitors/interactiveprintmonitor.hpp"

#include "printers/print.hpp"
#include "printers/idpprinter.hpp"
#include "printers/idp2printer.hpp"
#include "printers/ecnfprinter.hpp"
#include "printers/tptpprinter.hpp"
#include "printers/aspprinter.hpp"
using namespace std;
using namespace rel_ops;

template<typename Stream> bool ASPPrinter<Stream>::threevalWarningIssued = false;

template<class Stream>
Printer* Printer::create(Stream& stream) {
	switch (getGlobal()->getOptions()->language()) {
	case Language::IDP:
		return new IDPPrinter<Stream>(stream);
	case Language::IDP2:
		return new IDP2Printer<Stream>(stream);
	case Language::ECNF:
		return new EcnfPrinter<Stream>(getOption(BoolType::CREATETRANSLATION), stream);
	case Language::TPTP:
		return new TPTPPrinter<Stream>(getOption(BoolType::PROVER_SUPPORTS_TFA), stream);
	case Language::ASP:
		return new ASPPrinter<Stream>(stream);
	default:
		Assert(false);
		return NULL;
	}
}

template Printer* Printer::create<ostream>(ostream&);
template Printer* Printer::create<stringstream>(stringstream&);
template Printer* Printer::create<InteractivePrintMonitor>(InteractivePrintMonitor&);

void Printer::visit(const AbstractTheory* t) {
	t->accept(this);
}
void Printer::visit(const Formula* f) {
	f->accept(this);
}

template<>
void Printer::print(const Structure* b) {
	visit(b);
}
template<>
void Printer::print(const Namespace* b) {
	visit(b);
}
template<>
void Printer::print(const UserProcedure* p) {
	visit(p);
}

template<>
void Printer::print(const Vocabulary* b) {
	visit(b);
}

template<>
void Printer::print(const Query* b) {
	visit(b);
}

template<>
void Printer::print(const FOBDD* b) {
	visit(b);
}
template<>
void Printer::print(const Compound* b) {
	visit(b);
}
