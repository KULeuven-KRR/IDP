/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "TheoryVisitor.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "error.hpp"

#include "groundtheories/GroundPolicy.hpp"
#include "groundtheories/GroundTheory.hpp"

using namespace std;

void TheoryVisitor::visit(const Theory* t) {
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

void TheoryVisitor::visit(const AbstractGroundTheory*) {
	throw notyetimplemented("Visiting of ground theories");
}

void TheoryVisitor::visit(const GroundTheory<GroundPolicy>*) {
	throw notyetimplemented("Visiting of ground theories");
}

class DefaultFormulaVisitor: TheoryVisitor {
	VISITORFRIENDS()
protected:
	void visit(const Formula* f);
};

void TheoryVisitor::traverse(const Formula* f) {
	for (size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterms()[n]->accept(this);
	}
	for (size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformulas()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const PredForm* pf) {
	traverse(pf);
}

void TheoryVisitor::visit(const EqChainForm* ef) {
	traverse(ef);
}

void TheoryVisitor::visit(const EquivForm* ef) {
	traverse(ef);
}

void TheoryVisitor::visit(const BoolForm* bf) {
	traverse(bf);
}

void TheoryVisitor::visit(const QuantForm* qf) {
	traverse(qf);
}

void TheoryVisitor::visit(const AggForm* af) {
	traverse(af);
}

void TheoryVisitor::visit(const Rule* r) {
	r->body()->accept(this);
}

void TheoryVisitor::visit(const Definition* d) {
	for (size_t n = 0; n < d->rules().size(); ++n) {
		d->rules()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const FixpDef* fd) {
	for (size_t n = 0; n < fd->rules().size(); ++n) {
		fd->rules()[n]->accept(this);
	}
	for (size_t n = 0; n < fd->defs().size(); ++n) {
		fd->defs()[n]->accept(this);
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

void TheoryVisitor::visit(const VarTerm* vt) {
	traverse(vt);
}

void TheoryVisitor::visit(const FuncTerm* ft) {
	traverse(ft);
}

void TheoryVisitor::visit(const DomainTerm* dt) {
	traverse(dt);
}

void TheoryVisitor::visit(const AggTerm* at) {
	traverse(at);
}

void TheoryVisitor::traverse(const SetExpr* s) {
	for (size_t n = 0; n < s->subterms().size(); ++n) {
		s->subterms()[n]->accept(this);
	}
	for (size_t n = 0; n < s->subformulas().size(); ++n) {
		s->subformulas()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const EnumSetExpr* es) {
	traverse(es);
}
void TheoryVisitor::visit(const QuantSetExpr* qs) {
	traverse(qs);
}

void TheoryVisitor::visit(const GroundDefinition* d) {
	for (auto it = d->begin(); it != d->end(); ++it) {
		(*it).second->accept(this);
	}
}

void TheoryVisitor::visit(const AggGroundRule*) {
	throw notyetimplemented("Visiting of ground theories");
}

void TheoryVisitor::visit(const PCGroundRule*) {
	throw notyetimplemented("Visiting of ground theories");
}

void TheoryVisitor::visit(const GroundSet*) {
	throw notyetimplemented("Visiting of ground theories");
}

void TheoryVisitor::visit(const GroundAggregate*) {
	throw notyetimplemented("Visiting of ground theories");
}
