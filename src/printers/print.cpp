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
#include "options.hpp"
#include "monitors/interactiveprintmonitor.hpp"

#include "printers/print.hpp"
#include "printers/idpprinter.hpp"
#include "printers/ecnfprinter.hpp"
#include "printers/tptpprinter.hpp"
using namespace std;
using namespace rel_ops;

const int ID_FOR_UNDEFINED = -1;
int getIDForUndefined() { return ID_FOR_UNDEFINED; }

template<class Stream>
Printer* Printer::create(Stream& stream) {
	switch(getGlobal()->getOptions()->language()) {
		case Language::IDP:
			return new IDPPrinter<Stream>(stream);
		case Language::ECNF:
			return new EcnfPrinter<Stream>(getOption(BoolType::CREATETRANSLATION), stream);
		case Language::TPTP:
			return new TPTPPrinter<Stream>(false, stream);
		default:
			Assert(false);
			return NULL;
	}
}

template<class Stream>
Printer* Printer::create(Stream& stream, bool arithmetic) {
	if (getGlobal()->getOptions()->language() == Language::TPTP) {
		return new TPTPPrinter<Stream>(arithmetic, stream);
	} else {
		return create<Stream>(stream);
	}
}

template Printer* Printer::create<ostream>(ostream&);
template Printer* Printer::create<ostream>(ostream&, bool);
template Printer* Printer::create<stringstream>(stringstream&);
template Printer* Printer::create<stringstream>(stringstream&, bool);
template Printer* Printer::create<InteractivePrintMonitor>(InteractivePrintMonitor&);

void Printer::visit(const AbstractTheory* t){
	t->accept(this);
}
void Printer::visit(const Formula* f){
	f->accept(this);
}

template<>
void Printer::print(const AbstractStructure* b){
	visit(b);
}
template<>
void Printer::print(const Namespace* b){
	visit(b);
}
template<>
void Printer::print(const Vocabulary* b){
	visit(b);
}
