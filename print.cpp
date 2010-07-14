/************************************
	print.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "print.hpp"
#include "theory.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "options.hpp"

extern Options options;

/**************
    Printer
**************/

Printer::Printer() {
	// Set indentation level to zero
	_indent = 0;
	// Open outputfile if given in options, use stdout otherwise
	if(options._outputfile.empty())
		_out = stdout;
	else
		_out = fopen(options._outputfile.c_str(),"a");
}

Printer::~Printer() {
	if(! options._outputfile.empty())
		fclose(_out);
}

Printer* Printer::create() {
	switch(options._format) {
		case OF_TXT:
			return new SimplePrinter();
		case OF_IDP:
			return new IDPPrinter();
		default:
			assert(false);
	}
}

void Printer::print(Vocabulary* v) 	{ v->accept(this); }
void Printer::print(Theory* t) 		{ t->accept(this); }
void Printer::print(Structure* s) 	{ s->accept(this); }

void Printer::indent() 		{ _indent++; }
void Printer::unindent() 	{ _indent--; }
void Printer::printtab() {
	for(unsigned int n = 0; n < _indent; ++n)
		fputs("  ",_out);
}

/*******************
    SimplePrinter
*******************/

void SimplePrinter::visit(Vocabulary* v) {
	string str = v->to_string();
	fputs(str.c_str(),_out);
}

void SimplePrinter::visit(Theory* t) {
	string str = t->to_string();
	fputs(str.c_str(),_out);
}

void SimplePrinter::visit(Structure* s) {
	string str = s->to_string();
	fputs(str.c_str(),_out);
}

/*****************
    IDPPrinter
*****************/

/** Theory **/

void IDPPrinter::visit(Theory* t) {
	fprintf(_out,"#theory %s",t->name().c_str());
	if(t->vocabulary()) {
		fprintf(_out," : %s",t->vocabulary()->name().c_str());
//		if(t->structure())
//			fprintf(_out," %s",t->structure()->name().c_str());
	}
	fprintf(_out," {\n");
	indent();
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		printtab();
		t->sentence(n)->accept(this);
		fprintf(_out,".\n");
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		t->definition(n)->accept(this);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		t->fixpdef(n)->accept(this);
	}
	unindent();
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
	printtab();
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
	printtab();
	fprintf(_out,"{\n");
	indent();
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		d->rule(n)->accept(this);
		fprintf(_out,"\n");
	}
	unindent();
	printtab();
	fprintf(_out,"}\n");
}

void IDPPrinter::visit(FixpDef* d) {
	printtab();
	fprintf(_out,"%s [\n",(d->lfp() ? "LFD" : "GFD"));
	indent();
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		d->rule(n)->accept(this);
		fprintf(_out,"\n");
	}
	for(unsigned int n = 0; n < d->nrDefs(); ++n) {
		d->def(n)->accept(this);
	}
	unindent();
	printtab();
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
	s->subform(0)->accept(this);
	fputs(" }",_out);
}

/*****************
    Structures
*****************/

void IDPPrinter::visit(Structure* s) {
	_currstructure = s;
	Vocabulary* v = s->vocabulary();
	fprintf(_out,"#structure %s",s->name().c_str());
	if(s->vocabulary())
		fprintf(_out," : %s",v->name().c_str());
	fprintf(_out,"{\n");
	indent();
	for(unsigned int n = 0; n < v->nrPreds(); ++n) {
		_currsymbol = v->pred(n);
		if(s->hasInter(v->pred(n)))
			s->inter(v->pred(n))->accept(this);
	}
	for(unsigned int n = 0; n < v->nrFuncs(); ++n) {
		_currsymbol = v->func(n);
		if(s->hasInter(v->func(n)))
			s->inter(v->func(n))->accept(this);
	}
	unindent();
	fprintf(_out,"}\n");
}

void IDPPrinter::visit(SortTable* t) {
	for(unsigned int n = 0; n < t->size(); ++n) {
		string s = ElementUtil::ElementToString(t->element(n),t->type());
		fputs(s.c_str(),_out);
		if(n < t->size()-1)
			fprintf(_out,";");
	}
}

void IDPPrinter::print(PredTable* t) {
	for(unsigned int r = 0; r < t->size(); ++r) {
		for(unsigned int c = 0; c < t->arity(); ++c) {
			string s = ElementUtil::ElementToString(t->element(r,c),t->type(c));
			fputs(s.c_str(),_out);
			if(c < t->arity()-1)
				fprintf(_out,(_currsymbol->ispred() ? "," : "->"));
		}
		if(r < t->size()-1)
			fprintf(_out,"; ");
	}
}

void IDPPrinter::printInter(const char* pt1name,const char* pt2name,PredTable* pt1,PredTable* pt2) {
	string fullname = _currsymbol->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	printtab();
	fprintf(_out,"%s[%s] = { ",shortname.c_str(),pt1name);
	if(pt1) print(pt1);
	fprintf(_out," }\n");
	printtab();
	fprintf(_out,"%s[%s] = { ",shortname.c_str(),pt2name);
	if(pt2) print(pt2);
	fprintf(_out," }\n");
}

void IDPPrinter::visit(PredInter* p) {
	string fullname = _currsymbol->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	if(_currsymbol->nrSorts() == 0) { // proposition
		printtab();
		fprintf(_out,"%s = %s\n",shortname.c_str(),(p->ctpf()->empty() ? "false" : "true"));
	}
	else if(!_currsymbol->ispred() && _currsymbol->nrSorts() == 1) { // constant
		printtab();
		fprintf(_out,"%s = ",shortname.c_str());
		print(p->ctpf());
		fprintf(_out,"\n");
	}
	else if(p->ctpf() == p->cfpt()) {
		if(p->ct() && p->cf()) { // impossible
			assert(false);
		}
		else if(p->ct()) { // p = ctpf
			printtab();
			fprintf(_out,"%s = { ",shortname.c_str());
			print(p->ctpf());
			fprintf(_out," }\n");
		}
		else if(p->cf()) { // p[cf] = cfpt and p[u] = comp(cfpt)
			PredTable* u = StructUtils::complement(p->cfpt(),_currsymbol->sorts(),_currstructure);
			printInter("cf","u",p->cfpt(),u);
			delete(u);
		}
		else { // p[ct] = {} and p[cf] = {}
			printInter("ct","cf",NULL,NULL);
		}
	}
	else {
		if(p->ct() && p->cf()) { // p[ct] = { ctpf } and p[cf] = { cfpt }
			printInter("ct","cf",p->ctpf(),p->cfpt());
		}
		else if(p->ct()) { // p[ct] = { ctpf } and p[u] = { cfpt \ ctpf }
			PredTable* u = TableUtils::difference(p->cfpt(),p->ctpf());
			printInter("ct","u",p->ctpf(),u);
			delete(u);
		}
		else if(p->cf()) { // p[cf] = { cfpt } and p[u] = { ctpf \ cfpt }
			PredTable* u = TableUtils::difference(p->ctpf(),p->cfpt());
			printInter("cf","u",p->cfpt(),u);
			delete(u);
		}
		else { // p[ct] = {} and p[cf] = {}
			printInter("ct","cf",NULL,NULL);
		}
	}
}

void IDPPrinter::visit(FuncInter* f) {
	//TODO Currently, function interpretation is handled as predicate interpretation
	f->predinter()->accept(this);	
}

/*******************
    Vocabularies
*******************/

void IDPPrinter::visit(Vocabulary* v) {
	fprintf(_out,"#vocabulary %s {\n",v->name().c_str());
	indent();
	traverse(v);
	unindent();
	fprintf(_out,"}\n");
}

void IDPPrinter::visit(Sort* s) {
	printtab();
	fprintf(_out,"type %s",s->name().c_str());
	if(s->parent())
		fprintf(_out," isa %s",s->parent()->name().c_str());
	fprintf(_out,"\n");
}

void IDPPrinter::visit(Predicate* p) {
	printtab();
	string shortname = p->name().substr(0,p->name().find('/'));
	fputs(shortname.c_str(),_out);
	if(p->arity() > 0) {
		fprintf(_out,"(%s",p->sort(0)->name().c_str());
		for(unsigned int n = 1; n < p->arity(); ++n)
			fprintf(_out,",%s",p->sort(n)->name().c_str());
		fprintf(_out,")");
	}
	fprintf(_out,"\n");
}

void IDPPrinter::visit(Function* f) {
	printtab();
	if(f->partial())
		fprintf(_out,"partial ");
	string shortname = f->name().substr(0,f->name().find('/'));
	fputs(shortname.c_str(),_out);
	if(f->arity() > 0) {
		fprintf(_out,"(%s",f->insort(0)->name().c_str());
		for(unsigned int n = 1; n < f->arity(); ++n)
			fprintf(_out,",%s",f->insort(n)->name().c_str());
		fprintf(_out,")");
	}
	fprintf(_out," : %s\n",f->outsort()->name().c_str());
}

