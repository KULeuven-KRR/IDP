/************************************
	print.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "print.hpp"
#include "theory.hpp"
#include "term.hpp"

/** Theory **/

void IDPPrinter::visit(Theory* t) {
	fprintf(_out,"#theory %s",t->name().c_str());
	if(t->vocabulary()) {
		fprintf(_out," : %s",t->vocabulary()->name().c_str());
		if(t->structure())
			fprintf(_out," %s",t->structure()->name().c_str());
	}
	fprintf(_out," {\n");
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		t->sentence(n)->accept(this);
		fprintf(_out,".\n");
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		t->definition(n)->accept(this);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		t->fixpdef(n)->accept(this);
	}
	fprintf(_out,"}\n");
}

/** Formulas **/

void IDPPrinter::visit(PredForm* f) {
	if(! f->sign())
		fputs("~",_out);
	string fullname = f->symb()->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	fputs(shortname.c_str(),_out);
	if(f->nrSubterms()) {
		fputs("(",_out);
		f->subterm(0)->accept(this);
		for(unsigned int n = 1; n < f->nrSubterms(); ++n) {
			fputs(",",_out);
			f->subterm(n)->accept(this);
		}
		fputs(")",_out);
	}
}

void IDPPrinter::visit(EqChainForm* f) {
	if(! f->sign())
		fputs("~",_out);
	fputs("(",_out);
	f->subterm(0)->accept(this);
	for(unsigned int n = 0; n < f->nrComps(); ++n) {
		switch(f->comp(n)) {
			case '=':
				if(f->compsign(n)) fputs(" = ",_out);
				else fputs(" ~= ",_out);
				break;
			case '<':
				if(f->compsign(n)) fputs(" < ",_out);
				else fputs(" >= ",_out);
				break;
			case '>':
				if(f->compsign(n)) fputs(" > ",_out);
				else fputs(" =< ",_out);
				break;
		}
		f->subterm(n+1)->accept(this);
		if(! f->conj() && n+1 < f->nrComps()) {
			fputs(" | ",_out);
			f->subterm(n+1)->accept(this);
		}
	}
	fputs(")",_out);
}

void IDPPrinter::visit(EquivForm* f) {
	fputs("(",_out);
	f->left()->accept(this);
	fputs(" <=> ",_out);
	f->right()->accept(this);
	fputs(")",_out);
}

void IDPPrinter::visit(BoolForm* f) {
	if(! f->nrSubforms()) {
		if(f->sign() == f->conj())
			fputs("true",_out);
		else
			fputs("false",_out);
	} else {
		if(! f->sign())
			fputs("~",_out);
		fputs("(",_out);
		f->subform(0)->accept(this);
		for(unsigned int n = 1; n < f->nrSubforms(); ++n) {
			if(f->conj())
				fputs(" & ",_out);
			else
				fputs(" | ",_out);
			f->subform(n)->accept(this);
		}
		fputs(")",_out);
	}
}

void IDPPrinter::visit(QuantForm* f) {
	if(! f->sign())
		fputs("~",_out);
	fputs("(",_out);
	if(f->univ())
		fputs("!",_out);
	else
		fputs("?",_out);
	for(unsigned int n = 0; n < f->nrQvars(); ++n) {
		fputs(" ",_out);
		fputs(f->qvar(n)->name().c_str(),_out);
		if(f->qvar(n)->sort())
			fprintf(_out,"[%s]",f->qvar(n)->sort()->name().c_str());
	}
	fputs(" : ",_out);
	f->subform(0)->accept(this);
	fputs(")",_out);
}

/** Definitions **/

void IDPPrinter::visit(Rule* r) {
	if(r->nrQvars()) {
		fprintf(_out,"!");
		for(unsigned int n = 0; n < r->nrQvars(); ++n) {
			fprintf(_out," %s",r->qvar(n)->name().c_str());
			if(r->qvar(n)->sort())
				fprintf(_out,"[%s]",r->qvar(n)->sort()->name().c_str());
		}
		fprintf(_out," : ");
	}
	r->head()->accept(this);
	fprintf(_out," <- ");
	r->body()->accept(this);
	fprintf(_out,".");
}

void IDPPrinter::visit(Definition* d) {
	// What about tabs?
	fprintf(_out,"{\n");
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		fprintf(_out," ");
		d->rule(n)->accept(this);
		fprintf(_out,"\n");
	}
	fprintf(_out,"}\n");
}

void IDPPrinter::visit(FixpDef* d) {
	// What about tabs?
	fprintf(_out,"%s [\n",(d->lfp() ? "LFD" : "GFD"));
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		fprintf(_out," ");
		d->rule(n)->accept(this);
		fprintf(_out,"\n");
	}
	for(unsigned int n = 0; n < d->nrDefs(); ++n) {
		d->def(n)->accept(this);
	}
	fprintf(_out,"]\n");
}

/** Terms **/

void IDPPrinter::visit(VarTerm* t) {
	fputs(t->var()->name().c_str(),_out);
}

void IDPPrinter::visit(FuncTerm* t) {
	string fullname = t->func()->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	fputs(shortname.c_str(),_out);
	if(t->nrSubterms()) {
		fputs("(",_out);
		t->arg(0)->accept(this);
		for(unsigned int n = 1; n < t->nrSubterms(); ++n) {
			fputs(",",_out);
			t->arg(n)->accept(this);
		}
		fputs(")",_out);
	}
}

void IDPPrinter::visit(DomainTerm* t) {
	fputs(ElementUtil::ElementToString(t->value(),t->type()).c_str(),_out);
}

void IDPPrinter::visit(AggTerm* t) {
	string AggTypeNames[5] = { "#", "sum", "prod", "min", "max" };
	fputs(AggTypeNames[t->type()].c_str(),_out);
	t->set()->accept(this);
}

/** Sets **/

void IDPPrinter::visit(EnumSetExpr* s) {
	fputs("[ ",_out);
	if(s->nrSubforms()) {
		s->subform(0)->accept(this);
		for(unsigned int n = 1; n < s->nrSubforms(); ++n) {
			fputs(",",_out);
			s->subform(n)->accept(this);
		}
	}
	fputs(" ]",_out);
}

void IDPPrinter::visit(QuantSetExpr* s) {
	fputs("{",_out);
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		fputs(" ",_out);
		fputs(s->qvar(n)->name().c_str(),_out);
		if(s->firstargsort())
			fprintf(_out,"[%s]",s->qvar(n)->sort()->name().c_str());
	}
	fputs(": ",_out);
	s->subform(0)->accept(this); //Stef: use subf() instead?
	fputs(" }",_out);
}
