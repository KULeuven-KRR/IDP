/************************************
	ground.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ground.h"
#include <typeinfo>

extern string itos(int);
extern bool nexttuple(vector<unsigned int>&, const vector<unsigned int>&);

/********************************************************
	Basic top-down, non-optimized grounding algorithm
********************************************************/

void NaiveGrounder::visit(VarTerm* vt) {
	assert(_varmapping.find(vt->var()) != _varmapping.end());
	TypedElement te = _varmapping[vt->var()];
	Element e = ElementUtil::clone(te);
	_returnTerm = new DomainTerm(vt->sort(),te._type,e,0);
}

void NaiveGrounder::visit(DomainTerm* dt) {
	_returnTerm = dt->clone();
}

void NaiveGrounder::visit(FuncTerm* ft) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ft->nrSubterms(); ++n) {
		_returnTerm = 0;
		ft->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnTerm = new FuncTerm(ft->func(),vt,0);
}

void NaiveGrounder::visit(AggTerm* at) {
	_returnSet = 0;
	at->set()->accept(this);
	assert(_returnSet);
	_returnTerm = new AggTerm(_returnSet,at->type(),0);
}

void NaiveGrounder::visit(EnumSetExpr* s) {
	vector<Formula*> vf;
	for(unsigned int n = 0; n < s->nrSubforms(); ++n) {
		_returnFormula = 0;
		s->subform(n)->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	}
	vector<Term*> vt;
	for(unsigned int n = 0; n < s->nrSubterms(); ++n) {
		_returnTerm = 0;
		s->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnSet = new EnumSetExpr(vf,vt,0);
}

void NaiveGrounder::visit(QuantSetExpr* s) {
	vector<unsigned int> limits;
	vector<SortTable*> tables;
	vector<Formula*> vf(0);
	vector<Term*> vt(0);
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		SortTable* st = _structure->inter(s->qvar(n)->sort());
		assert(st);
		assert(st->finite());
		if(st->empty()) {
			_returnSet = new EnumSetExpr(vf,vt,0);
			return;
		}
		else {
			limits.push_back(st->size());
			tables.push_back(st);
			TypedElement e; 
			e._type = st->type(); 
			_varmapping[s->qvar(n)] = e;
		}
	}

	vector<unsigned int> tuple(limits.size(),0);
	assert(!tables.empty());
	ElementType weighttype = tables[0]->type();
	do {
		Element weight = ElementUtil::clone(tables[0]->element(tuple[0]),weighttype);
		for(unsigned int n = 0; n < tuple.size(); ++n) {
			_varmapping[s->qvar(n)]._element = tables[n]->element(tuple[n]);
		}
		_returnFormula = 0;
		s->subf()->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
		vt.push_back(new DomainTerm(s->qvar(0)->sort(),weighttype,weight,0));
	} while(nexttuple(tuple,limits));
	_returnSet = new EnumSetExpr(vf,vt,0);
}

void NaiveGrounder::visit(PredForm* pf) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		_returnTerm = 0;
		pf->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new PredForm(pf->sign(),pf->symb(),vt,0);
}

void NaiveGrounder::visit(EquivForm* ef) {
	_returnFormula = 0;
	ef->left()->accept(this);
	assert(_returnFormula);
	Formula* nl = _returnFormula;
	_returnFormula = 0;
	ef->right()->accept(this);
	assert(_returnFormula);
	Formula* nr = _returnFormula;
	_returnFormula = new EquivForm(ef->sign(),nl,nr,0);
}

void NaiveGrounder::visit(EqChainForm* ef) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		_returnTerm = 0;
		ef->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new EqChainForm(ef->sign(),ef->conj(),vt,ef->comps(),ef->compsigns(),0);
}

void NaiveGrounder::visit(BoolForm* bf) {
	vector<Formula*> vf;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		_returnFormula = 0;
		bf->subform(n)->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	}
	_returnFormula = new BoolForm(bf->sign(),bf->conj(),vf,0);
}

void NaiveGrounder::visit(QuantForm* qf) {
	vector<unsigned int> limits;
	vector<SortTable*> tables;
	vector<Formula*> vf(0);
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		SortTable* st = _structure->inter(qf->qvar(n)->sort());
		assert(st);
		assert(st->finite());
		if(st->empty()) {
			_returnFormula = new BoolForm(qf->sign(),qf->univ(),vf,0);
			return;
		}
		else {
			limits.push_back(st->size());
			tables.push_back(st);
			TypedElement e; 
			e._type = st->type(); 
			_varmapping[qf->qvar(n)] = e;
		}
	}

	vector<unsigned int> tuple(limits.size(),0);
	do {
		for(unsigned int n = 0; n < tuple.size(); ++n) {
			_varmapping[qf->qvar(n)]._element = tables[n]->element(tuple[n]);
		}
		_returnFormula = 0;
		qf->subf()->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	} while(nexttuple(tuple,limits));
	_returnFormula = new BoolForm(qf->sign(),qf->univ(),vf,0);
}

void NaiveGrounder::visit(Rule* r) {
	vector<unsigned int> limits;
	vector<SortTable*> tables;
	Definition* d = new Definition();
	for(unsigned int n = 0; n < r->nrQvars(); ++n) {
		SortTable* st = _structure->inter(r->qvar(n)->sort());
		assert(st);
		assert(st->finite());
		if(st->empty()) {
			_returnDef = d;
			return;
		}
		else {
			limits.push_back(st->size());
			tables.push_back(st);
			TypedElement e; 
			e._type = st->type(); 
			_varmapping[r->qvar(n)] = e;
		}
	}

	vector<unsigned int> tuple(limits.size(),0);
	do {
		for(unsigned int n = 0; n < tuple.size(); ++n) {
			_varmapping[r->qvar(n)]._element = tables[n]->element(tuple[n]);
		}
		Formula* nb;
		PredForm* nh;

		_returnFormula = 0;
		r->head()->accept(this);
		assert(_returnFormula);
		assert(typeid(*_returnFormula) == typeid(PredForm));
		nh = dynamic_cast<PredForm*>(_returnFormula);

		_returnFormula = 0;
		r->body()->accept(this);
		assert(_returnFormula);
		nb = _returnFormula;

		d->add(new Rule(vector<Variable*>(0),nh,nb,0));

	} while(nexttuple(tuple,limits));
	_returnDef = d;
}

void NaiveGrounder::visit(Definition* d) {
	Definition* grounddef = new Definition();
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		_returnDef = 0;
		visit(d->rule(n));
		assert(_returnDef);
		for(unsigned int m = 0; m < _returnDef->nrRules(); ++m) {
			grounddef->add(_returnDef->rule(m));
		}
		delete(_returnDef);
	}
	_returnDef = grounddef;
}

void NaiveGrounder::visit(FixpDef* d) {
	FixpDef* grounddef = new FixpDef(d->lfp());
	for(unsigned int n = 0; n < d->nrDefs(); ++n) {
		_returnFixpDef = 0;
		visit(d->def(n));
		assert(_returnFixpDef);
		grounddef->add(_returnFixpDef);
	}
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		_returnDef = 0;
		visit(d->rule(n));
		assert(_returnDef);
		for(unsigned int m = 0; m < _returnDef->nrRules(); ++m) {
			grounddef->add(_returnDef->rule(m));
		}
		delete(_returnDef);
	}
	_returnFixpDef = grounddef;
}

void NaiveGrounder::visit(Theory* t) {
	Theory* grounding = new Theory("",t->vocabulary(),_structure,0);
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_returnDef = 0;
		t->definition(n)->accept(this);
		assert(_returnDef);
		grounding->add(_returnDef);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_returnFixpDef = 0;
		t->fixpdef(n)->accept(this);
		assert(_returnFixpDef);
		grounding->add(_returnFixpDef);
	}
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_returnFormula = 0;
		t->sentence(n)->accept(this);
		assert(_returnFormula);
		grounding->add(_returnFormula);
	}
	Theory* structtheo = StructUtils::convert_to_theory(_structure);
	grounding->add(structtheo);
	delete(structtheo);

	_returnTheory = grounding;
}
