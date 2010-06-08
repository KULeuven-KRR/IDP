/************************************
	Visitor::visitor.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "visitor.h"

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
