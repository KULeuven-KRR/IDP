/************************************
	print.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "print.hpp"
#include "theory.hpp"
#include "ecnf.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "namespace.hpp"

/**************
    Printer
**************/

Printer::Printer() {
	// Set indentation level to zero
	_indent = 0;
}

Printer* Printer::create(InfOptions* opts) {
	switch(opts->_format) {
		case OF_TXT:
			return new SimplePrinter();
		case OF_IDP:
			return new IDPPrinter(opts->_printtypes);
		default:
			assert(false);
	}
}

string Printer::print(const Vocabulary* v)			{ v->accept(this); return _out.str(); }
string Printer::print(const AbstractTheory* t)		{ t->accept(this); return _out.str(); }
string Printer::print(const AbstractStructure* s)	{ s->accept(this); return _out.str(); }

void Printer::indent() 		{ _indent++; }
void Printer::unindent()	{ _indent--; }
void Printer::printtab() {
	for(unsigned int n = 0; n < _indent; ++n)
		_out << "  ";
}

/*******************
    SimplePrinter
*******************/

void SimplePrinter::visit(const Vocabulary* v) {
	_out << v->to_string();
}

void SimplePrinter::visit(const AbstractTheory* t) {
	_out << t->to_string();
}

void SimplePrinter::visit(const AbstractStructure* s) {
	_out << s->to_string();
}

/*****************
    IDPPrinter
*****************/

/** Theory **/

void IDPPrinter::visit(const Theory* t) {
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
}

void IDPPrinter::visit(const EcnfTheory* et) {
	_out << et->to_string();
}

/** Formulas **/

void IDPPrinter::visit(const PredForm* f) {
	if(! f->sign())
		_out << "~";
#ifndef NDEBUG
	_out << f->symb()->to_string();
#else
	string fullname = f->symb()->name();
	_out << fullname.substr(0,fullname.find('/'));
#endif
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

void IDPPrinter::visit(const EqChainForm* f) {
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

void IDPPrinter::visit(const EquivForm* f) {
	_out << "(";
	f->left()->accept(this);
	_out << " <=> ";
	f->right()->accept(this);
	_out << ")";
}

void IDPPrinter::visit(const BoolForm* f) {
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

void IDPPrinter::visit(const QuantForm* f) {
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

void IDPPrinter::visit(const Rule* r) {
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

void IDPPrinter::visit(const Definition* d) {
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

void IDPPrinter::visit(const FixpDef* d) {
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

void IDPPrinter::visit(const VarTerm* t) {
	_out << t->var()->name();
}

void IDPPrinter::visit(const FuncTerm* t) {
#ifndef NDEBUG
	_out << t->func()->to_string();
#else
	string fullname = t->func()->name();
	_out << fullname.substr(0,fullname.find('/'));
#endif
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

void IDPPrinter::visit(const DomainTerm* t) {
	_out << ElementUtil::ElementToString(t->value(),t->type());
}

void IDPPrinter::visit(const AggTerm* t) {
	string AggTypeNames[5] = { "#", "sum", "prod", "min", "max" };
	_out << AggTypeNames[t->type()];
	t->set()->accept(this);
}

/** Sets **/

void IDPPrinter::visit(const EnumSetExpr* s) {
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

void IDPPrinter::visit(const QuantSetExpr* s) {
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

void IDPPrinter::visit(const Structure* s) {
	_currstructure = s;
	Vocabulary* v = s->vocabulary();
//	_out << "#structure " << s->name();
//	if(s->vocabulary())
//		_out << " : " << v->name();
//	_out << " {\n";
//	indent();
	for(unsigned int n = 0; n < v->nrNBPreds(); ++n) {
		_currsymbol = v->nbpred(n);
		if(_printtypes || _currsymbol->nrSorts() != 1 || _currsymbol != _currsymbol->sort(0)->pred()) {
			s->inter(v->nbpred(n))->accept(this);
		}
	}
	for(unsigned int n = 0; n < v->nrNBFuncs(); ++n) {
		_currsymbol = v->nbfunc(n);
		s->inter(v->nbfunc(n))->accept(this);
	}
//	unindent();
//	_out << "}\n";
}

void IDPPrinter::visit(const SortTable* t) {
	for(unsigned int n = 0; n < t->size(); ++n) {
		_out << ElementUtil::ElementToString(t->element(n),t->type());
		if(n < t->size()-1)
			_out << ";";
	}
}

void IDPPrinter::print(const PredTable* t) {
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

void IDPPrinter::printInter(const char* pt1name,const char* pt2name,const PredTable* pt1,const PredTable* pt2) {
	string fullname = _currsymbol->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	printtab();
	_out << shortname << "<" << pt1name << "> = { ";
	if(pt1)
		print(pt1);
	_out << " }\n";
	printtab();
	_out << shortname << "<" << pt2name << "> = { ";
	if(pt2)
		print(pt2);
	_out << " }\n";
}

void IDPPrinter::visit(const PredInter* p) {
	string fullname = _currsymbol->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	if(_currsymbol->nrSorts() == 0) { // proposition
		printtab();
		_out << shortname << " = " << (p->ctpf()->empty() ? (p->cfpt()->empty() ? "unknown" : "false") : "true") << "\n";
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

void IDPPrinter::visit(const FuncInter* f) {
	//TODO Currently, function interpretation is handled as predicate interpretation
	f->predinter()->accept(this);	
}

/*******************
    Vocabularies
*******************/

void IDPPrinter::visit(const Vocabulary* v) {
//	_out << "#vocabulary " << v->name() << " {\n";
//	indent();
	traverse(v);
//	unindent();
//	_out << "}\n";
}

void IDPPrinter::visit(const Sort* s) {
//	printtab();
	_out << "type " << s->name();
	if(s->nrParents() > 0)
		_out << " isa " << s->parent(0)->name();
		for(unsigned int n = 1; n < s->nrParents(); ++n)
			_out << "," << s->parent(n)->name();
	_out << "\n";
}

void IDPPrinter::visit(const Predicate* p) {
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

void IDPPrinter::visit(const Function* f) {
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

