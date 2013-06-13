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

#include "TheoryMutatingVisitor.hpp"

#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"

using namespace std;

Theory* TheoryMutatingVisitor::visit(Theory* t) {
	for (auto it = t->sentences().begin(); it != t->sentences().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for (auto it = t->definitions().begin(); it != t->definitions().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for (auto it = t->fixpdefs().begin(); it != t->fixpdefs().end(); ++it) {
		*it = (*it)->accept(this);
	}
	return t;
}

AbstractGroundTheory* TheoryMutatingVisitor::visit(AbstractGroundTheory* t) {
	throw notyetimplemented("Visiting of ground theories");
	return t;
}

Formula* TheoryMutatingVisitor::traverse(Formula* f) {
	for (size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterm(n, f->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformula(n, f->subformulas()[n]->accept(this));
	}
	return f;
}

Formula* TheoryMutatingVisitor::visit(PredForm* pf) {
	return traverse(pf);
}

Formula* TheoryMutatingVisitor::visit(EqChainForm* ef) {
	return traverse(ef);
}

Formula* TheoryMutatingVisitor::visit(EquivForm* ef) {
	return traverse(ef);
}

Formula* TheoryMutatingVisitor::visit(BoolForm* bf) {
	return traverse(bf);
}

Formula* TheoryMutatingVisitor::visit(QuantForm* qf) {
	return traverse(qf);
}

Formula* TheoryMutatingVisitor::visit(AggForm* af) {
	return traverse(af);
}

Rule* TheoryMutatingVisitor::visit(Rule* r) {
	auto newhead = r->head()->accept(this);
	Assert(isa<PredForm>(*newhead));
	r->head(dynamic_cast<PredForm*>(newhead));
	r->body(r->body()->accept(this));
	return r;
}

Definition* TheoryMutatingVisitor::visit(Definition* d) {
	ruleset newset;
	for (auto rule : d->rules()) {
		newset.insert(rule->accept(this));
	}
	d->rules(newset);
	return d;
}

FixpDef* TheoryMutatingVisitor::visit(FixpDef* fd) {
	for (size_t n = 0; n < fd->rules().size(); ++n) {
		fd->rule(n, fd->rules()[n]->accept(this));
	}
	for (size_t n = 0; n < fd->defs().size(); ++n) {
		fd->def(n, fd->defs()[n]->accept(this));
	}
	return fd;
}

Term* TheoryMutatingVisitor::traverse(Term* t) {
	for (size_t n = 0; n < t->subterms().size(); ++n) {
		t->subterm(n, t->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < t->subsets().size(); ++n) {
		t->subset(n, t->subsets()[n]->accept(this));
	}
	return t;
}

Term* TheoryMutatingVisitor::visit(VarTerm* vt) {
	return traverse(vt);
}

Term* TheoryMutatingVisitor::visit(FuncTerm* ft) {
	return traverse(ft);
}

Term* TheoryMutatingVisitor::visit(DomainTerm* dt) {
	return traverse(dt);
}

Term* TheoryMutatingVisitor::visit(AggTerm* at) {
	return traverse(at);
}

QuantSetExpr* TheoryMutatingVisitor::traverse(QuantSetExpr* s) {
	s->setTerm(s->getTerm()->accept(this));
	s->setCondition(s->getCondition()->accept(this));
	return s;
}

EnumSetExpr* TheoryMutatingVisitor::traverse(EnumSetExpr* s) {
	for (size_t n = 0; n < s->getSets().size(); ++n) {
		s->setSet(n, s->getSets()[n]->accept(this));
	}
	return s;
}

EnumSetExpr* TheoryMutatingVisitor::visit(EnumSetExpr* es) {
	return traverse(es);
}

QuantSetExpr* TheoryMutatingVisitor::visit(QuantSetExpr* qs) {
	return traverse(qs);
}

GroundDefinition* TheoryMutatingVisitor::visit(GroundDefinition* d) {
	for (auto rule : d->rules()) {
		rule.second = rule.second->accept(this);
	}
	return d;
}

GroundRule* TheoryMutatingVisitor::visit(AggGroundRule* r) {
	throw notyetimplemented("Visiting of ground theories");
	return r;
}

GroundRule* TheoryMutatingVisitor::visit(PCGroundRule* r) {
	throw notyetimplemented("Visiting of ground theories");
	return r;
}
