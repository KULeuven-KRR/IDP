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
Printer* Printer::create(Options* opts, Stream& stream) {
	switch(opts->language()) {
		case Language::IDP:
			return new IDPPrinter<Stream>(opts->getValue(BoolType::LONGNAMES), stream);
		case Language::ECNF:
			return new EcnfPrinter<Stream>(opts->getValue(BoolType::CREATETRANSLATION), stream);
		case Language::TPTP:
			return new TPTPPrinter<Stream>(false, stream);
		default:
			assert(false);
			return NULL;
	}
}

template<class Stream>
Printer* Printer::create(Options* opts, Stream& stream, bool arithmetic) {
	if (opts->language() == Language::TPTP) {
		return new TPTPPrinter<Stream>(arithmetic, stream);
	} else {
		return create<Stream>(opts, stream);
	}
}

template Printer* Printer::create<stringstream>(Options*, stringstream&);
template Printer* Printer::create<stringstream>(Options*, stringstream&, bool);
template Printer* Printer::create<InteractivePrintMonitor>(Options*, InteractivePrintMonitor&);

void Printer::visit(const AbstractTheory* t){
	t->accept(this);
}
void Printer::visit(const Formula* f){
	f->accept(this);
}
