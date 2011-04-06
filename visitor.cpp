/************************************
	Visitor::visitor.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "visitor.hpp"

#include <iostream>

#include "namespace.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "ecnf.hpp"

using namespace std;

/**************
	Visitor
**************/

/** Standard visiting behavior **/

void Visitor::visit(const Namespace* s)			{ traverse(s);	}

void Visitor::visit(const Theory* t)			{ traverse(t);	}
void Visitor::visit(const Definition* a)		{ traverse(a);	}
void Visitor::visit(const Rule* a)				{ traverse(a);	}
void Visitor::visit(const FixpDef* a)			{ traverse(a);	}

void Visitor::visit(const PredForm* a)			{ traverse(a);	}
void Visitor::visit(const BracketForm* a)		{ traverse(a);	}
void Visitor::visit(const EqChainForm* a)		{ traverse(a);	}
void Visitor::visit(const EquivForm* a)			{ traverse(a);	}
void Visitor::visit(const BoolForm* a)			{ traverse(a);	}
void Visitor::visit(const QuantForm* a)			{ traverse(a);	}
void Visitor::visit(const AggForm* a)			{ traverse(a);	}

void Visitor::visit(const VarTerm* a)			{ traverse(a);	}
void Visitor::visit(const FuncTerm* a)			{ traverse(a);	}
void Visitor::visit(const DomainTerm* a)		{ traverse(a);	}
void Visitor::visit(const AggTerm* a)			{ traverse(a);	}
void Visitor::visit(const EnumSetExpr* a)		{ traverse(a);	}
void Visitor::visit(const QuantSetExpr* a)		{ traverse(a);	}

void Visitor::visit(const Structure* s)			{ traverse(s);	}
void Visitor::visit(const SortTable*)			{ }
void Visitor::visit(const PredInter*)			{ }
void Visitor::visit(const FuncInter*)			{ }

void Visitor::visit(const Vocabulary* v)		{ traverse(v);	}
void Visitor::visit(const Sort*)				{ }
void Visitor::visit(const Predicate*)			{ }
void Visitor::visit(const Function*)			{ }

void Visitor::visit(const GroundTheory* t)		{ traverse(t);	}
void Visitor::visit(const SolverTheory* t)		{ traverse(t);	}
void Visitor::visit(const GroundDefinition* d) 	{ traverse(d);	}
void Visitor::visit(const PCGroundRuleBody*)	{ }
void Visitor::visit(const AggGroundRuleBody*)	{ }
void Visitor::visit(const GroundAggregate*)		{ }
void Visitor::visit(const GroundSet*)			{ }

void Visitor::visit(const CPReification*)		{ }
void Visitor::visit(const CPSumTerm*)			{ }
void Visitor::visit(const CPWSumTerm*)			{ }
void Visitor::visit(const CPVarTerm*)			{ }

/** Standard traversing behavior **/

void Visitor::traverse(const Namespace* s) {
	for(unsigned int n = 0; n < s->nrVocs(); ++n)
		s->vocabulary(n)->accept(this);
	for(unsigned int n = 0; n < s->nrTheos(); ++n)
		s->theory(n)->accept(this);
	for(unsigned int n = 0; n < s->nrStructs(); ++n)
		s->structure(n)->accept(this);
	for(unsigned int n = 0; n < s->nrSubs(); ++n)
		s->subspace(n)->accept(this);
	//TODO traverse procedures and options
}

void Visitor::traverse(const Formula* f) {
	for(unsigned int n = 0; n < f->nrSubforms(); ++n)
		f->subform(n)->accept(this);
	for(unsigned int n = 0; n < f->nrSubterms(); ++n)
		f->subterm(n)->accept(this);
}

void Visitor::traverse(const Term* t) {
	for(unsigned int n = 0; n < t->nrSubforms(); ++n)
		t->subform(n)->accept(this);
	for(unsigned int n = 0; n < t->nrSubterms(); ++n)
		t->subterm(n)->accept(this);
	for(unsigned int n = 0; n < t->nrSubsets(); ++n)
		t->subset(n)->accept(this);
}

void Visitor::traverse(const Rule* r) {
	r->head()->accept(this);
	r->body()->accept(this);
}

void Visitor::traverse(const Definition* d) {
	for(unsigned int n = 0; n < d->nrRules(); ++n)
		visit(d->rule(n));
}

void Visitor::traverse(const FixpDef* d) {
	for(unsigned int n = 0; n < d->nrRules(); ++n)
		visit(d->rule(n));
	for(unsigned int n = 0; n < d->nrDefs(); ++n)
		visit(d->def(n));
}

void Visitor::traverse(const SetExpr* s) {
	for(unsigned int n = 0; n < s->nrSubforms(); ++n)
		s->subform(n)->accept(this);
	for(unsigned int n = 0; n < s->nrSubterms(); ++n)
		s->subterm(n)->accept(this);
}

void Visitor::traverse(const Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) 
		t->sentence(n)->accept(this);
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) 
		t->definition(n)->accept(this);
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) 
		t->fixpdef(n)->accept(this);
}

void Visitor::traverse(const Structure* s) {
	for(unsigned int n = 0; n < s->vocabulary()->nrNBSorts(); ++n)
		visit(s->inter(s->vocabulary()->nbsort(n)));
	for(unsigned int n = 0; n < s->vocabulary()->nrNBPreds(); ++n)
		visit(s->inter(s->vocabulary()->nbpred(n)));
	for(unsigned int n = 0; n < s->vocabulary()->nrNBFuncs(); ++n)
		visit(s->inter(s->vocabulary()->nbfunc(n)));
}

void Visitor::traverse(const Vocabulary* v) {
	for(unsigned int n = 0; n < v->nrNBSorts(); ++n)
		visit(v->nbsort(n));
	for(unsigned int n = 0; n < v->nrNBPreds(); ++n)
		visit(v->nbpred(n));
	for(unsigned int n = 0; n < v->nrNBFuncs(); ++n)
		visit(v->nbfunc(n));
}

void Visitor::traverse(const GroundTheory* t) {
	//TODO Visit clauses?
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n)
		t->definition(n)->accept(this);
	for(unsigned int n = 0; n < t->nrSets(); ++n)
		t->set(n)->accept(this);
	for(unsigned int n = 0; n < t->nrAggregates(); ++n)
		t->aggregate(n)->accept(this);
	//TODO: repeat above for fixpoint definitions
	for(unsigned int n = 0; n < t->nrCPReifications(); ++n)
		t->cpreification(n)->accept(this);
}

void Visitor::traverse(const SolverTheory*) {
	//TODO
	assert(false);
}

void Visitor::traverse(const GroundDefinition* d) {
	for(GroundDefinition::const_ruleiterator it = d->begin(); it != d->end(); ++it)
		(it->second)->accept(this);
}

/** Standard visitor acceptance behavior **/

void Namespace::accept(Visitor* v) 			const { v->visit(this);	}

void Theory::accept(Visitor* v) 			const { v->visit(this);	}
void Definition::accept(Visitor* v) 		const { v->visit(this);	}
void Rule::accept(Visitor* v) 				const { v->visit(this);	}
void FixpDef::accept(Visitor* v) 			const { v->visit(this);	}

void PredForm::accept(Visitor* v) 			const { v->visit(this);	}
void BracketForm::accept(Visitor* v) 		const { v->visit(this); }
void EquivForm::accept(Visitor* v) 			const { v->visit(this);	}
void EqChainForm::accept(Visitor* v) 		const { v->visit(this);	}
void BoolForm::accept(Visitor* v) 			const { v->visit(this);	}
void QuantForm::accept(Visitor* v) 			const { v->visit(this);	}
void AggForm::accept(Visitor* v) 			const { v->visit(this);	}

void VarTerm::accept(Visitor* v) 			const { v->visit(this);	}
void FuncTerm::accept(Visitor* v) 			const { v->visit(this); }
void DomainTerm::accept(Visitor* v)			const { v->visit(this);	}
void AggTerm::accept(Visitor* v)			const { v->visit(this);	}
void QuantSetExpr::accept(Visitor* v)		const { v->visit(this);	}
void EnumSetExpr::accept(Visitor* v)		const { v->visit(this); }

void Structure::accept(Visitor* v) 			const { v->visit(this);	}
void SortTable::accept(Visitor* v) 			const { v->visit(this);	}
void PredInter::accept(Visitor* v) 			const { v->visit(this);	}
void FuncInter::accept(Visitor* v) 			const { v->visit(this);	}

void Vocabulary::accept(Visitor* v) 		const { v->visit(this);	}
void Sort::accept(Visitor* v) 				const { v->visit(this);	}
void Predicate::accept(Visitor* v) 			const { v->visit(this);	}
void Function::accept(Visitor* v) 			const { v->visit(this);	}

void SolverTheory::accept(Visitor* v)		const { v->visit(this);	}
void GroundTheory::accept(Visitor* v)		const { v->visit(this);	}
void GroundDefinition::accept(Visitor* v)	const { v->visit(this);	}
void PCGroundRuleBody::accept(Visitor* v) 	const { v->visit(this);	}
void AggGroundRuleBody::accept(Visitor* v) 	const { v->visit(this);	}
void GroundAggregate::accept(Visitor* v)	const { v->visit(this);	}
void GroundSet::accept(Visitor* v)			const { v->visit(this);	}

void CPReification::accept(Visitor* v)		const { v->visit(this);	}
void CPSumTerm::accept(Visitor* v)			const { v->visit(this);	}
void CPWSumTerm::accept(Visitor* v)			const { v->visit(this);	}
void CPVarTerm::accept(Visitor* v)			const { v->visit(this);	}

/***********************
	Mutating visitor
***********************/

Formula* MutatingVisitor::visit(PredForm* pf) {
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		Term* nt = pf->subterm(n)->accept(this);
		pf->arg(n,nt);
	}
	pf->setfvars();
	return pf;
}

Formula* MutatingVisitor::visit(BracketForm* bf) {
	Formula* f = bf->subf()->accept(this);
	bf->subf(f);
	bf->setfvars();
	return bf;
}

Formula* MutatingVisitor::visit(EqChainForm* ef) {
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		Term* nt = ef->subterm(n)->accept(this);
		ef->term(n,nt);
	}
	ef->setfvars();
	return ef;
}

Formula* MutatingVisitor::visit(EquivForm* ef) {
	Formula* nl = ef->left()->accept(this);
	Formula* nr = ef->right()->accept(this);
	ef->left(nl);
	ef->right(nr);
	ef->setfvars();
	return ef;
}

Formula* MutatingVisitor::visit(BoolForm* bf) {
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		Formula* nf = bf->subform(n)->accept(this);
		bf->subf(n,nf);
	}
	bf->setfvars();
	return bf;
}

Formula* MutatingVisitor::visit(QuantForm* qf) {
	Formula* ns = qf->subf()->accept(this);
	qf->subf(ns);
	qf->setfvars();
	return qf;
}

Formula* MutatingVisitor::visit(AggForm* af) {
	Term* nl = af->left()->accept(this);
	af->left(nl);
	SetExpr* s = af->right()->set()->accept(this);
	af->right()->set(s);
	af->setfvars();
	return af;
}

Rule* MutatingVisitor::visit(Rule* r) {
	Formula* nb = r->body()->accept(this);
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
		nb = new QuantForm(true,false,vv,nb,nb->pi());
	}
	// Replace the body
	r->body(nb);
	return r;
}

Definition* MutatingVisitor::visit(Definition* d) {
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		Rule* r = d->rule(n)->accept(this);
		d->rule(n,r);
	}
	d->defsyms();
	return d;
}

FixpDef* MutatingVisitor::visit(FixpDef* fd) {
	for(unsigned int n = 0; n < fd->nrRules(); ++n) {
		Rule* r = fd->rule(n)->accept(this);
		fd->rule(n,r);
	}
	for(unsigned int n = 0; n < fd->nrDefs(); ++n) {
		FixpDef* d = fd->def(n)->accept(this);
		fd->def(n,d);
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
		ft->arg(n,nt);
	}
	ft->setfvars();
	return ft;
}

Term* MutatingVisitor::visit(DomainTerm* dt) {
	return dt;
}

Term* MutatingVisitor::visit(AggTerm* at) {
	SetExpr* nst = at->set()->accept(this);
	at->set(nst);
	at->setfvars();
	return at;
}

SetExpr* MutatingVisitor::visit(EnumSetExpr* es) {
	for(unsigned int n = 0; n < es->nrSubterms(); ++n) {
		Term* nt = es->subterm(n)->accept(this);
		es->weight(n,nt);
	}
	for(unsigned int n = 0; n < es->nrSubforms(); ++n) {
		Formula* nf = es->subform(n)->accept(this);
		es->subf(n,nf);
	}
	es->setfvars();
	return es;
}

SetExpr* MutatingVisitor::visit(QuantSetExpr* qs) {
	Formula* ns = qs->subf()->accept(this);
	qs->subf(ns);
	qs->setfvars();
	return qs;
}

Theory* MutatingVisitor::visit(Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		Formula* f = t->sentence(n)->accept(this);
		vector<Variable*> vv;
		for(unsigned int m = 0; m < f->nrFvars(); ++m) {
			vv.push_back(f->fvar(m));
		}
		if(!vv.empty()) {
			f = new QuantForm(true,true,vv,f,f->pi());
		}
		t->sentence(n,f);
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		Definition* d = t->definition(n)->accept(this);
		t->definition(n,d);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		FixpDef* fd = t->fixpdef(n)->accept(this);
		t->fixpdef(n,fd);
	}
	return t;
}

AbstractTheory* MutatingVisitor::visit(GroundTheory* t) {
	// TODO
	return t;
}

AbstractTheory* MutatingVisitor::visit(SolverTheory* t) {
	// TODO
	return t;
}

AbstractDefinition* MutatingVisitor::visit(GroundDefinition* d) {
	//TODO
	return d;
}

/** Standard mutating visitor acceptance behavior **/

Theory* Theory::accept(MutatingVisitor* v) 							{ return v->visit(this); }
Definition* Definition::accept(MutatingVisitor* v) 					{ return v->visit(this); }
Rule* Rule::accept(MutatingVisitor* v) 								{ return v->visit(this); }
FixpDef* FixpDef::accept(MutatingVisitor* v) 						{ return v->visit(this); }

Formula* PredForm::accept(MutatingVisitor* v) 						{ return v->visit(this); }
Formula* BracketForm::accept(MutatingVisitor* v) 					{ return v->visit(this); }
Formula* EquivForm::accept(MutatingVisitor* v) 						{ return v->visit(this); }
Formula* EqChainForm::accept(MutatingVisitor* v) 					{ return v->visit(this); }
Formula* BoolForm::accept(MutatingVisitor* v) 						{ return v->visit(this); }
Formula* QuantForm::accept(MutatingVisitor* v) 						{ return v->visit(this); }
Formula* AggForm::accept(MutatingVisitor* v) 						{ return v->visit(this); }

Term* VarTerm::accept(MutatingVisitor* v) 							{ return v->visit(this); }
Term* FuncTerm::accept(MutatingVisitor* v) 							{ return v->visit(this); }
Term* DomainTerm::accept(MutatingVisitor* v) 						{ return v->visit(this); }
Term* AggTerm::accept(MutatingVisitor* v) 							{ return v->visit(this); }

SetExpr* QuantSetExpr::accept(MutatingVisitor* v)					{ return v->visit(this); }
SetExpr* EnumSetExpr::accept(MutatingVisitor* v) 					{ return v->visit(this); }

AbstractTheory* SolverTheory::accept(MutatingVisitor* v)			{ return v->visit(this); }
AbstractTheory* GroundTheory::accept(MutatingVisitor* v)			{ return v->visit(this); }
AbstractDefinition* GroundDefinition::accept(MutatingVisitor* v)	{ return v->visit(this); }
