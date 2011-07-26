/************************************
	print.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "term.hpp"
#include "theory.hpp"
#include "options.hpp"
#include "monitors/interactiveprintmonitor.hpp"

#include "printers/print.hpp"
#include "printers/idpprinter.hpp"
#include "printers/ecnfprinter.hpp"

using namespace std;
using namespace rel_ops;

const int ID_FOR_UNDEFINED = -1;
int getIDForUndefined() { return ID_FOR_UNDEFINED; }

template<class Stream>
Printer* Printer::create(Options* opts, Stream& stream) {
	switch(opts->language()) {
		case LAN_IDP:
			return new IDPPrinter<Stream>(opts->longnames(), stream);
		case LAN_ECNF:
			return new EcnfPrinter<Stream>(opts->writeTranslation(), stream);
		default:
			assert(false);
			return NULL;
	}
}

template Printer* Printer::create<stringstream>(Options*, stringstream&);
template Printer* Printer::create<InteractivePrintMonitor>(Options*, InteractivePrintMonitor&);

void Printer::visit(const AbstractTheory* t){
	t->accept(this);
}
void Printer::visit(const Formula* f){
	f->accept(this);
}
