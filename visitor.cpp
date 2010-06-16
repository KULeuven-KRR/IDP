/************************************
	Visitor::visitor.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theory.hpp"

void Visitor::visit(PredForm* a)		{ traverse(a);	}
void Visitor::visit(EqChainForm* a)		{ traverse(a);	}
void Visitor::visit(EquivForm* a)		{ traverse(a);	}
void Visitor::visit(BoolForm* a)		{ traverse(a);	}
void Visitor::visit(QuantForm* a)		{ traverse(a);	}
void Visitor::visit(Rule* a)			{ traverse(a);	}
void Visitor::visit(Definition* a)		{ traverse(a);	}
void Visitor::visit(FixpDef* a)			{ traverse(a);	}
void Visitor::visit(VarTerm* a)			{ traverse(a);	}
void Visitor::visit(FuncTerm* a)		{ traverse(a);	}
void Visitor::visit(DomainTerm* a)		{ traverse(a);	}
void Visitor::visit(AggTerm* a)			{ traverse(a);	}
void Visitor::visit(EnumSetExpr* a)		{ traverse(a);	}
void Visitor::visit(QuantSetExpr* a)	{ traverse(a);	}
void Visitor::visit(Theory* t)			{ traverse(t);	}
void Visitor::visit(SortTable*)			{ }
void Visitor::visit(PredInter*)			{ }
void Visitor::visit(FuncInter*)			{ }
void Visitor::visit(Structure* s)		{ traverse(s);	}
void Visitor::visit(EcnfTheory*)		{ /* TODO */	}


void Visitor::traverse(Formula* f) {
	for(unsigned int n = 0; n < f->nrSubforms(); ++n) {
		f->subform(n)->accept(this);
	}
	for(unsigned int n = 0; n < f->nrSubterms(); ++n) {
		f->subterm(n)->accept(this);
	}
}

void Visitor::traverse(Term* t) {
	for(unsigned int n = 0; n < t->nrSubforms(); ++n) {
		t->subform(n)->accept(this);
	}
	for(unsigned int n = 0; n < t->nrSubterms(); ++n) {
		t->subterm(n)->accept(this);
	}
	for(unsigned int n = 0; n < t->nrSubsets(); ++n) {
		t->subset(n)->accept(this);
	}
}

void Visitor::traverse(Rule* r) {
	r->head()->accept(this);
	r->body()->accept(this);
}

void Visitor::traverse(Definition* d) {
	for(unsigned int n = 0; n < d->nrrules(); ++n) {
		visit(d->rule(n));
	}
}

void Visitor::traverse(FixpDef* d) {
	for(unsigned int n = 0; n < d->nrrules(); ++n) {
		visit(d->rule(n));
	}
	for(unsigned int n = 0; n < d->nrdefs(); ++n) {
		visit(d->def(n));
	}
}

void Visitor::traverse(SetExpr* s) {
	for(unsigned int n = 0; n < s->nrSubforms(); ++n) {
		s->subform(n)->accept(this);
	}
	for(unsigned int n = 0; n < s->nrSubterms(); ++n) {
		s->subterm(n)->accept(this);
	}
}

void Visitor::traverse(Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) 
		t->sentence(n)->accept(this);
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) 
		t->definition(n)->accept(this);
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) 
		t->fixpdef(n)->accept(this);
}

void Visitor::traverse(Structure* s) {
	for(unsigned int n = 0; n < s->vocabulary()->nrSorts(); ++n) {
		visit(s->sortinter(n));
	}
	for(unsigned int n = 0; n < s->vocabulary()->nrPreds(); ++n) {
		visit(s->predinter(n));
	}
	for(unsigned int n = 0; n < s->vocabulary()->nrFuncs(); ++n) {
		visit(s->funcinter(n));
	}
}

/******************
	Visit terms
******************/

void VarTerm::accept(Visitor* v) {
	v->visit(this);
}

void FuncTerm::accept(Visitor* v) {
	v->visit(this);
}

void DomainTerm::accept(Visitor* v) {
	v->visit(this);
}

void AggTerm::accept(Visitor* v) {
	v->visit(this);
}

void QuantSetExpr::accept(Visitor* v) {
	v->visit(this);
}

void EnumSetExpr::accept(Visitor* v) {
	v->visit(this);
}

/********************
	Visit theory
********************/

void PredForm::accept(Visitor* v) {
	v->visit(this);
}

void EquivForm::accept(Visitor* v) {
	v->visit(this);
}

void EqChainForm::accept(Visitor* v) {
	v->visit(this);
}

void BoolForm::accept(Visitor* v) {
	v->visit(this);
}

void QuantForm::accept(Visitor* v) {
	v->visit(this);
}

void Rule::accept(Visitor* v) {
	v->visit(this);
}

void Definition::accept(Visitor* v) {
	v->visit(this);
}

void FixpDef::accept(Visitor* v) {
	v->visit(this);
}

void Theory::accept(Visitor* v) {
	v->visit(this);
}


/***********************
	Mutating visitor
***********************/

Formula* MutatingVisitor::visit(PredForm* pf) {
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		Term* nt = pf->subterm(n)->accept(this);
		if(nt != pf->subterm(n)) {
			delete(pf->subterm(n));
			pf->arg(n,nt);
		}
	}
	pf->setfvars();
	return pf;
}

Formula* MutatingVisitor::visit(EqChainForm* ef) {
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		Term* nt = ef->subterm(n)->accept(this);
		if(nt != ef->subterm(n)) {
			delete(ef->subterm(n));
			ef->term(n,nt);
		}
	}
	ef->setfvars();
	return ef;
}

Formula* MutatingVisitor::visit(EquivForm* ef) {
	Formula* nl = ef->left()->accept(this);
	Formula* nr = ef->right()->accept(this);
	if(nl != ef->left()) {
		delete(ef->left());
		ef->left(nl);
	}
	if(nr != ef->right()) {
		delete(ef->right());
		ef->right(nr);
	}
	ef->setfvars();
	return ef;
}

Formula* MutatingVisitor::visit(BoolForm* bf) {
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		Formula* nf = bf->subform(n)->accept(this);
		if(nf != bf->subform(n)) {
			delete(bf->subform(n));
			bf->subf(n,nf);
		}
	}
	bf->setfvars();
	return bf;
}

Formula* MutatingVisitor::visit(QuantForm* qf) {
	Formula* ns = qf->subf()->accept(this);
	if(ns != qf->subf()) {
		delete(qf->subf());
		qf->subf(ns);
	}
	qf->setfvars();
	return qf;
}

Rule* MutatingVisitor::visit(Rule* r) {
	Formula* nb = r->body()->accept(this);
	if(nb != r->body()) {
		// Check if the new body has free variables that do not occur in the head
		vector<Variable*> vv;
		for(unsigned int n = 0; n < nb->nrFvars(); ++n) {
			unsigned int m = 0; 
			for(; m < r->nrQvars(); ++m) {
				if(r->qvar(m) == nb->fvar(n)) break;
			}
			if(m == r->nrQvars()) vv.push_back(nb->fvar(n));
		}
		if(!vv.empty()) {
			ParseInfo* npi = 0; 
			if(nb->pi()) npi = new ParseInfo(nb->pi());
			nb = new QuantForm(true,false,vv,nb,npi);
		}
		// Replace the body
		delete(r->body());
		r->body(nb);
	}
	return r;
}

Definition* MutatingVisitor::visit(Definition* d) {
	for(unsigned int n = 0; n < d->nrrules(); ++n) {
		Rule* r = d->rule(n)->accept(this);
		if(r != d->rule(n)) {
			delete(d->rule(n));
			d->rule(n,r);
		}
	}
	d->defsyms();
	return d;
}

FixpDef* MutatingVisitor::visit(FixpDef* fd) {
	for(unsigned int n = 0; n < fd->nrrules(); ++n) {
		Rule* r = fd->rule(n)->accept(this);
		if(r != fd->rule(n)) {
			delete(fd->rule(n));
			fd->rule(n,r);
		}
	}
	for(unsigned int n = 0; n < fd->nrdefs(); ++n) {
		FixpDef* d = fd->def(n)->accept(this);
		if(d != fd->def(n)) {
			delete(fd->def(n));
			fd->def(n,d);
		}
	}
	fd->defsyms();
	return fd;
}

Term* MutatingVisitor::visit(VarTerm* vt) {
	return vt;
}

Term* MutatingVisitor::visit(FuncTerm* ft) {
	for(unsigned int n = 0; n < ft->nrSubterms(); ++n) {
		Term* nt = ft->subterm(n)->accept(this);
		if(nt != ft->subterm(n)) {
			delete(ft->subterm(n));
			ft->arg(n,nt);
		}
	}
	ft->setfvars();
	return ft;
}

Term* MutatingVisitor::visit(DomainTerm* dt) {
	return dt;
}

Term* MutatingVisitor::visit(AggTerm* at) {
	SetExpr* nst = at->set()->accept(this);
	if(nst != at->set()) {
		delete(at->set());
		at->set(nst);
	}
	at->setfvars();
	return at;
}

SetExpr* MutatingVisitor::visit(EnumSetExpr* es) {
	for(unsigned int n = 0; n < es->nrSubterms(); ++n) {
		Term* nt = es->subterm(n)->accept(this);
		if(nt != es->subterm(n)) {
			delete(es->subterm(n));
			es->weight(n,nt);
		}
	}
	for(unsigned int n = 0; n < es->nrSubforms(); ++n) {
		Formula* nf = es->subform(n)->accept(this);
		if(nf != es->subform(n)) {
			delete(es->subform(n));
			es->subf(n,nf);
		}
	}
	es->setfvars();
	return es;
}

SetExpr* MutatingVisitor::visit(QuantSetExpr* qs) {
	Formula* ns = qs->subf()->accept(this);
	if(ns != qs->subf()) {
		delete(qs->subf());
		qs->subf(ns);
	}
	qs->setfvars();
	return qs;
}

Theory* MutatingVisitor::visit(Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		Formula* f = t->sentence(n)->accept(this);
		if(f != t->sentence(n)) {
			vector<Variable*> vv;
			for(unsigned int m = 0; m < f->nrFvars(); ++m) {
				vv.push_back(f->fvar(m));
			}
			if(!vv.empty()) {
				ParseInfo* npi = 0;
				if(f->pi()) npi = new ParseInfo(f->pi());
				f = new QuantForm(true,true,vv,f,npi);
			}
			delete(t->sentence(n));
			t->sentence(n,f);
		}
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		Definition* d = t->definition(n)->accept(this);
		if(d != t->definition(n)) {
			delete(t->definition(n));
			t->definition(n,d);
		}
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		FixpDef* fd = t->fixpdef(n)->accept(this);
		if(fd != t->fixpdef(n)) {
			delete(t->fixpdef(n));
			t->fixpdef(n,fd);
		}
	}
	return t;
}

EcnfTheory* MutatingVisitor::visit(EcnfTheory* t) {
	// TODO
	return t;
}

Term* VarTerm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Term* FuncTerm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Term* DomainTerm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Term* AggTerm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

SetExpr* QuantSetExpr::accept(MutatingVisitor* v) {
	return v->visit(this);
}

SetExpr* EnumSetExpr::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Formula* PredForm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Formula* EquivForm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Formula* EqChainForm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Formula* BoolForm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Formula* QuantForm::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Rule* Rule::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Definition* Definition::accept(MutatingVisitor* v) {
	return v->visit(this);
}

FixpDef* FixpDef::accept(MutatingVisitor* v) {
	return v->visit(this);
}

Theory* Theory::accept(MutatingVisitor* v) {
	return v->visit(this);
}
