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

#include "TheoryVisitor.hpp"

#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"

#include "groundtheories/GroundPolicy.hpp"
#include "groundtheories/GroundTheory.hpp"

using namespace std;

void TheoryVisitor::traverse(const Formula* f) {
	for (size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterms()[n]->accept(this);
	}
	for (size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformulas()[n]->accept(this);
	}
}

void TheoryVisitor::traverse(const Term* t) {
	for (size_t n = 0; n < t->subterms().size(); ++n) {
		t->subterms()[n]->accept(this);
	}
	for (size_t n = 0; n < t->subsets().size(); ++n) {
		t->subsets()[n]->accept(this);
	}
}

void TheoryVisitor::traverse(const QuantSetExpr* s) {
	s->getTerm()->accept(this);
	s->getCondition()->accept(this);
}

void TheoryVisitor::traverse(const EnumSetExpr* s) {
	for (size_t n = 0; n < s->getSets().size(); ++n) {
		s->getSets()[n]->accept(this);
	}
}

void TheoryVisitor::traverse(const Theory* t) {
	for (auto it = t->sentences().cbegin(); it != t->sentences().cend(); ++it) {
		(*it)->accept(this);
	}
	for (auto it = t->definitions().cbegin(); it != t->definitions().cend(); ++it) {
		(*it)->accept(this);
	}
	for (auto it = t->fixpdefs().cbegin(); it != t->fixpdefs().cend(); ++it) {
		(*it)->accept(this);
	}
}

void DefaultTraversingTheoryVisitor::visit(const Theory* t) {
	traverse(t);
}

void DefaultTraversingTheoryVisitor::visit(const AbstractGroundTheory*) {
	throw notyetimplemented("Visiting of ground theories");
}

void DefaultTraversingTheoryVisitor::visit(const GroundTheory<GroundPolicy>*) {
	throw notyetimplemented("Visiting of ground theories");
}

void DefaultTraversingTheoryVisitor::visit(const PredForm* pf) {
	traverse(pf);
}

void DefaultTraversingTheoryVisitor::visit(const EqChainForm* ef) {
	traverse(ef);
}

void DefaultTraversingTheoryVisitor::visit(const EquivForm* ef) {
	traverse(ef);
}

void DefaultTraversingTheoryVisitor::visit(const BoolForm* bf) {
	traverse(bf);
}

void DefaultTraversingTheoryVisitor::visit(const QuantForm* qf) {
	traverse(qf);
}

void DefaultTraversingTheoryVisitor::visit(const AggForm* af) {
	traverse(af);
}

void DefaultTraversingTheoryVisitor::visit(const Rule* r) {
	r->head()->accept(this);
	r->body()->accept(this);
}

void DefaultTraversingTheoryVisitor::visit(const Definition* d) {
	for (auto rule : d->rules()) {
		rule->accept(this);
	}
}

void DefaultTraversingTheoryVisitor::visit(const FixpDef* fd) {
	for (size_t n = 0; n < fd->rules().size(); ++n) {
		fd->rules()[n]->accept(this);
	}
	for (size_t n = 0; n < fd->defs().size(); ++n) {
		fd->defs()[n]->accept(this);
	}
}

void DefaultTraversingTheoryVisitor::visit(const VarTerm* vt) {
	traverse(vt);
}

void DefaultTraversingTheoryVisitor::visit(const FuncTerm* ft) {
	traverse(ft);
}

void DefaultTraversingTheoryVisitor::visit(const DomainTerm* dt) {
	traverse(dt);
}

void DefaultTraversingTheoryVisitor::visit(const AggTerm* at) {
	traverse(at);
}

void DefaultTraversingTheoryVisitor::visit(const EnumSetExpr* es) {
	traverse(es);
}
void DefaultTraversingTheoryVisitor::visit(const QuantSetExpr* qs) {
	traverse(qs);
}

void DefaultTraversingTheoryVisitor::visit(const GroundDefinition* d) {
	for (auto rule : d->rules()) {
		rule.second->accept(this);
	}
}

void DefaultTraversingTheoryVisitor::visit(const AggGroundRule*) {
	throw notyetimplemented("Visiting of ground theories");
}

void DefaultTraversingTheoryVisitor::visit(const PCGroundRule*) {
	throw notyetimplemented("Visiting of ground theories");
}

void DefaultTraversingTheoryVisitor::visit(const GroundSet*) {
	throw notyetimplemented("Visiting of ground theories");
}

void DefaultTraversingTheoryVisitor::visit(const GroundAggregate*) {
	throw notyetimplemented("Visiting of ground theories");
}
