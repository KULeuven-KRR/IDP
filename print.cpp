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
#include "data.hpp"

/**************
    Printer
**************/

Printer::Printer() {
	// Set indentation level to zero
	_indent = 0;
}

//Printer::~Printer() {
//	if(! _options._outputfile.empty())
//		fclose(_out);
//}

Printer* Printer::create() {
	switch(_options._format) {
		case OF_TXT:
			return new SimplePrinter();
		case OF_IDP:
			return new IDPPrinter();
		default:
			assert(false);
	}
}

string Printer::print(Vocabulary* v) 	{ v->accept(this); return _out.str(); }
string Printer::print(Theory* t) 		{ t->accept(this); return _out.str(); }
string Printer::print(Structure* s) 	{ s->accept(this); return _out.str(); }

void Printer::indent() 		{ _indent++; }
void Printer::unindent()	{ _indent--; }
void Printer::printtab() {
	for(unsigned int n = 0; n < _indent; ++n)
		_out << "  ";
}

/*******************
    SimplePrinter
*******************/

void SimplePrinter::visit(Vocabulary* v) {
	_out << v->to_string();
}

void SimplePrinter::visit(Theory* t) {
	_out << t->to_string();
}

void SimplePrinter::visit(Structure* s) {
	_out << s->to_string();
}

/*****************
    IDPPrinter
*****************/

/** Theory **/

void IDPPrinter::visit(Theory* t) {
//	_out << "#theory " << t->name();
//	if(t->vocabulary()) {
//		_out << " : " << t->vocabulary()->name();
//		if(t->structure())
//			_out << " " << t->structure()->name();
//	}
//	_out << " {\n";
//	indent();
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		printtab();
		t->sentence(n)->accept(this);
		_out << ".\n";
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		t->definition(n)->accept(this);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		t->fixpdef(n)->accept(this);
	}
//	unindent();
//	_out << "}\n";
}

/** Formulas **/

void IDPPrinter::visit(PredForm* f) {
	if(! f->sign())
		_out << "~";
	string fullname = f->symb()->name();
	_out << fullname.substr(0,fullname.find('/'));
	if(f->nrSubterms()) {
		_out << "(";
		f->subterm(0)->accept(this);
		for(unsigned int n = 1; n < f->nrSubterms(); ++n) {
			_out << ",";
			f->subterm(n)->accept(this);
		}
		_out << ")";
	}
}

void IDPPrinter::visit(EqChainForm* f) {
	if(! f->sign())
		_out << "~";
	_out << "(";
	f->subterm(0)->accept(this);
	for(unsigned int n = 0; n < f->nrComps(); ++n) {
		switch(f->comp(n)) {
			case '=':
				if(f->compsign(n)) _out << " = ";
				else _out << " ~= ";
				break;
			case '<':
				if(f->compsign(n)) _out << " < ";
				else _out << " >= ";
				break;
			case '>':
				if(f->compsign(n)) _out << " > ";
				else _out << " =< ";
				break;
		}
		f->subterm(n+1)->accept(this);
		if(! f->conj() && n+1 < f->nrComps()) {
			_out << " | ";
			f->subterm(n+1)->accept(this);
		}
	}
	_out << ")";
}

void IDPPrinter::visit(EquivForm* f) {
	_out << "(";
	f->left()->accept(this);
	_out << " <=> ";
	f->right()->accept(this);
	_out << ")";
}

void IDPPrinter::visit(BoolForm* f) {
	if(! f->nrSubforms()) {
		if(f->sign() == f->conj())
			_out << "true";
		else
			_out << "false";
	} else {
		if(! f->sign())
			_out << "~";
		_out << "(";
		f->subform(0)->accept(this);
		for(unsigned int n = 1; n < f->nrSubforms(); ++n) {
			if(f->conj())
				_out << " & ";
			else
				_out << " | ";
			f->subform(n)->accept(this);
		}
		_out << ")";
	}
}

void IDPPrinter::visit(QuantForm* f) {
	if(! f->sign())
		_out << "~";
	_out << "(";
	if(f->univ())
		_out << "!";
	else
		_out << "?";
	for(unsigned int n = 0; n < f->nrQvars(); ++n) {
		_out << " ";
		_out << f->qvar(n)->name();
		if(f->qvar(n)->sort())
			_out << "[" << f->qvar(n)->sort()->name() << "]";
	}
	_out << " : ";
	f->subform(0)->accept(this);
	_out << ")";
}

/** Definitions **/

void IDPPrinter::visit(Rule* r) {
	printtab();
	if(r->nrQvars()) {
		_out << "!";
		for(unsigned int n = 0; n < r->nrQvars(); ++n) {
			_out << " " << r->qvar(n)->name();
			if(r->qvar(n)->sort())
				_out << "[" << r->qvar(n)->sort()->name() << "]";
		}
		_out << " : ";
	}
	r->head()->accept(this);
	_out << " <- ";
	r->body()->accept(this);
	_out << ".";
}

void IDPPrinter::visit(Definition* d) {
	printtab();
	_out << "{\n";
	indent();
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		d->rule(n)->accept(this);
		_out << "\n";
	}
	unindent();
	printtab();
	_out << "}\n";
}

void IDPPrinter::visit(FixpDef* d) {
	printtab();
	_out << (d->lfp() ? "LFD" : "GFD") << " [\n";
	indent();
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		d->rule(n)->accept(this);
		_out << "\n";
	}
	for(unsigned int n = 0; n < d->nrDefs(); ++n) {
		d->def(n)->accept(this);
	}
	unindent();
	printtab();
	_out << "]\n";
}

/** Terms **/

void IDPPrinter::visit(VarTerm* t) {
	_out << t->var()->name();
}

void IDPPrinter::visit(FuncTerm* t) {
	string fullname = t->func()->name();
	_out << fullname.substr(0,fullname.find('/'));
	if(t->nrSubterms()) {
		_out << "(";
		t->arg(0)->accept(this);
		for(unsigned int n = 1; n < t->nrSubterms(); ++n) {
			_out << ",";
			t->arg(n)->accept(this);
		}
		_out << ")";
	}
}

void IDPPrinter::visit(DomainTerm* t) {
	_out << ElementUtil::ElementToString(t->value(),t->type());
}

void IDPPrinter::visit(AggTerm* t) {
	string AggTypeNames[5] = { "#", "sum", "prod", "min", "max" };
	_out << AggTypeNames[t->type()];
	t->set()->accept(this);
}

/** Sets **/

void IDPPrinter::visit(EnumSetExpr* s) {
	_out << "[ ";
	if(s->nrSubforms()) {
		s->subform(0)->accept(this);
		for(unsigned int n = 1; n < s->nrSubforms(); ++n) {
			_out << ",";
			s->subform(n)->accept(this);
		}
	}
	_out << " ]";
}

void IDPPrinter::visit(QuantSetExpr* s) {
	_out << "{";
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		_out << " ";
		_out << s->qvar(n)->name();
		if(s->firstargsort())
			_out << "[" << s->qvar(n)->sort()->name() << "]";
	}
	_out << ": ";
	s->subform(0)->accept(this);
	_out << " }";
}

/*****************
    Structures
*****************/

void IDPPrinter::visit(Structure* s) {
	_currstructure = s;
	Vocabulary* v = s->vocabulary();
//	_out << "#structure " << s->name();
//	if(s->vocabulary())
//		_out << " : " << v->name();
//	_out << " {\n";
//	indent();
	for(unsigned int n = 0; n < v->nrNBPreds(); ++n) {
		_currsymbol = v->nbpred(n);
		if(s->hasInter(v->nbpred(n)))
			s->inter(v->nbpred(n))->accept(this);
	}
	for(unsigned int n = 0; n < v->nrNBFuncs(); ++n) {
		_currsymbol = v->nbfunc(n);
		if(s->hasInter(v->nbfunc(n)))
			s->inter(v->nbfunc(n))->accept(this);
	}
//	unindent();
//	_out << "}\n";
}

void IDPPrinter::visit(SortTable* t) {
	for(unsigned int n = 0; n < t->size(); ++n) {
		_out << ElementUtil::ElementToString(t->element(n),t->type());
		if(n < t->size()-1)
			_out << ";";
	}
}

void IDPPrinter::print(PredTable* t) {
	for(unsigned int r = 0; r < t->size(); ++r) {
		for(unsigned int c = 0; c < t->arity(); ++c) {
			_out << ElementUtil::ElementToString(t->element(r,c),t->type(c));
			if(c < t->arity()-1) {
				if(not(_currsymbol->ispred()) && c == t->arity()-2)
					_out << "->";
				else
					_out << ",";
			}
		}
		if(r < t->size()-1)
			_out << "; ";
	}
}

void IDPPrinter::printInter(const char* pt1name,const char* pt2name,PredTable* pt1,PredTable* pt2) {
	string fullname = _currsymbol->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	printtab();
	_out << shortname << "[" << pt1name << "] = { ";
	if(pt1)
		print(pt1);
	_out << " }\n";
	printtab();
	_out << shortname << "[" << pt2name << "] = { ";
	if(pt2)
		print(pt2);
	_out << " }\n";
}

void IDPPrinter::visit(PredInter* p) {
	string fullname = _currsymbol->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	if(_currsymbol->nrSorts() == 0) { // proposition
		printtab();
		_out << shortname << " = " << (p->ctpf()->empty() ? "false" : "true") << "\n";
	}
	else if(!_currsymbol->ispred() && _currsymbol->nrSorts() == 1) { // constant
		printtab();
		_out << shortname << " = ";
		print(p->ctpf());
		_out << "\n";
	}
	else if(p->ctpf() == p->cfpt()) {
		if(p->ct() && p->cf()) { // impossible
			assert(false);
		}
		else if(p->ct()) { // p = ctpf
			printtab();
			_out << shortname << " = { ";
			print(p->ctpf());
			_out << " }\n";
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
//	_out << "#vocabulary " << v->name() << " {\n";
//	indent();
	traverse(v);
//	unindent();
//	_out << "}\n";
}

void IDPPrinter::visit(Sort* s) {
//	printtab();
	_out << "type " << s->name();
	if(s->nrParents() > 0)
		_out << " isa " << s->parent(0)->name();
		for(unsigned int n = 1; n < s->nrParents(); ++n)
			_out << "," << s->parent(n)->name();
	_out << "\n";
}

void IDPPrinter::visit(Predicate* p) {
//	printtab();
	_out << p->name().substr(0,p->name().find('/'));
	if(p->arity() > 0) {
		_out << "(" << p->sort(0)->name();
		for(unsigned int n = 1; n < p->arity(); ++n)
			_out << "," << p->sort(n)->name();
		_out << ")";
	}
	_out << "\n";
}

void IDPPrinter::visit(Function* f) {
//	printtab();
	if(f->partial())
		_out << "partial ";
	_out << f->name().substr(0,f->name().find('/'));
	if(f->arity() > 0) {
		_out << "(" << f->insort(0)->name();
		for(unsigned int n = 1; n < f->arity(); ++n)
			_out << "," << f->insort(n)->name();
		_out << ")";
	}
	_out << " : " << f->outsort()->name() << "\n";
}

