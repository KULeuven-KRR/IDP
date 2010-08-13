/************************************
	ground.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ground.hpp"
#include <typeinfo>
#include <iostream>

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

int NaiveTranslator::translate(PFSymbol* s, const vector<string>& args) {
	map<PFSymbol*,map<vector<string>,int> >::iterator it = _table.find(s);
	if(it != _table.end()) {
		map<vector<string>,int>::iterator jt = (it->second).find(args);
		if(jt != (it->second).end()) return jt->second;
	}
	_table[s][args] = _nextnumber;
	_backsymbtable.push_back(s);
	_backargstable.push_back(args);
	return _nextnumber++;
}


/********************************************************
	Basic top-down, non-optimized grounding algorithm
********************************************************/

void NaiveGrounder::visit(VarTerm* vt) {
	assert(_varmapping.find(vt->var()) != _varmapping.end());
	TypedElement te = _varmapping[vt->var()];
	Element e = te._element;
	_returnTerm = new DomainTerm(vt->sort(),te._type,e,ParseInfo());
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
	_returnTerm = new FuncTerm(ft->func(),vt,ParseInfo());
}

void NaiveGrounder::visit(AggTerm* at) {
	_returnSet = 0;
	at->set()->accept(this);
	assert(_returnSet);
	_returnTerm = new AggTerm(_returnSet,at->type(),ParseInfo());
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
	_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
}

void NaiveGrounder::visit(QuantSetExpr* s) {
	SortTableTupleIterator stti(s->qvars(),_structure);
	vector<SortTable*> tables;
	vector<Formula*> vf(0);
	vector<Term*> vt(0);
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[s->qvar(n)] = e;
	}
	if(stti.empty()) {
		_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
	}
	else {
		ElementType weighttype = tables[0]->type();
		do {
			Element weight = stti.value(0);
			for(unsigned int n = 0; n < s->nrQvars(); ++n) {
				_varmapping[s->qvar(n)]._element = stti.value(n);
			}
			_returnFormula = 0;
			s->subf()->accept(this);
			assert(_returnFormula);
			vf.push_back(_returnFormula);
			vt.push_back(new DomainTerm(s->qvar(0)->sort(),weighttype,weight,ParseInfo()));
		} while(stti.nextvalue());
		_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
	}
}

void NaiveGrounder::visit(PredForm* pf) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		_returnTerm = 0;
		pf->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new PredForm(pf->sign(),pf->symb(),vt,ParseInfo());
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
	_returnFormula = new EquivForm(ef->sign(),nl,nr,ParseInfo());
}

void NaiveGrounder::visit(EqChainForm* ef) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		_returnTerm = 0;
		ef->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new EqChainForm(ef->sign(),ef->conj(),vt,ef->comps(),ef->compsigns(),ParseInfo());
}

void NaiveGrounder::visit(BoolForm* bf) {
	vector<Formula*> vf;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		_returnFormula = 0;
		bf->subform(n)->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	}
	_returnFormula = new BoolForm(bf->sign(),bf->conj(),vf,ParseInfo());
}

void NaiveGrounder::visit(QuantForm* qf) {
	SortTableTupleIterator stti(qf->qvars(),_structure);
	vector<Formula*> vf(0);
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[qf->qvar(n)] = e;
	}
	if(stti.empty()) {
		_returnFormula = new BoolForm(qf->sign(),qf->univ(),vf,ParseInfo());
	}
	else {
		do {
			for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
				_varmapping[qf->qvar(n)]._element = stti.value(n);
			}
			_returnFormula = 0;
			qf->subf()->accept(this);
			assert(_returnFormula);
			vf.push_back(_returnFormula);
		} while(stti.nextvalue());
		_returnFormula = new BoolForm(qf->sign(),qf->univ(),vf,ParseInfo());
	}
}

void NaiveGrounder::visit(Rule* r) {
	SortTableTupleIterator stti(r->qvars(),_structure);
	Definition* d = new Definition();
	for(unsigned int n = 0; n < r->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[r->qvar(n)] = e;
	}
	if(!stti.empty()) {
		do {
			for(unsigned int n = 0; n < r->nrQvars(); ++n) {
				_varmapping[r->qvar(n)]._element = stti.value(n);
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

			d->add(new Rule(vector<Variable*>(0),nh,nb,ParseInfo()));

		} while(stti.nextvalue());
	}
	_returnDef = d;
}

void NaiveGrounder::visit(Definition* d) {
	Definition* grounddef = new Definition();
	for(unsigned int n = 0; n < d->nrrules(); ++n) {
		_returnDef = 0;
		visit(d->rule(n));
		assert(_returnDef);
		for(unsigned int m = 0; m < _returnDef->nrrules(); ++m) {
			grounddef->add(_returnDef->rule(m));
		}
		delete(_returnDef);
	}
	_returnDef = grounddef;
}

void NaiveGrounder::visit(FixpDef* d) {
	FixpDef* grounddef = new FixpDef(d->lfp());
	for(unsigned int n = 0; n < d->nrdefs(); ++n) {
		_returnFixpDef = 0;
		visit(d->def(n));
		assert(_returnFixpDef);
		grounddef->add(_returnFixpDef);
	}
	for(unsigned int n = 0; n < d->nrrules(); ++n) {
		_returnDef = 0;
		visit(d->rule(n));
		assert(_returnDef);
		for(unsigned int m = 0; m < _returnDef->nrrules(); ++m) {
			grounddef->add(_returnDef->rule(m));
		}
		delete(_returnDef);
	}
	_returnFixpDef = grounddef;
}

void NaiveGrounder::visit(Theory* t) {
	Theory* grounding = new Theory("",t->vocabulary(),ParseInfo());

	// Ground the theory
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
	
	// Add the structure
	AbstractTheory* structtheo = StructUtils::convert_to_theory(_structure);
	grounding->add(dynamic_cast<Theory*>(structtheo));	// TODO: remove the dynamic cast
	delete(structtheo);

	// Add the function constraints
	Vocabulary* v = t->vocabulary();
	for(unsigned int n = 0; n < v->nrFuncs(); ++n) {
		Function* f = v->func(n);
		vector<unsigned int> intuple(f->arity(),0);
		vector<unsigned int> outtuple1(1,0);
		vector<unsigned int> outtuple2(1,0);
		vector<unsigned int> inlimits(f->arity());
		vector<unsigned int> outlimit(1);
		vector<SortTable*>	 intables(f->arity());
		SortTable*			 outtable;
		unsigned int m = 0;
		for(; m < f->arity(); ++m) {
			intables[m] = _structure->inter(f->insort(m));
			inlimits[m] = intables[m]->size();
			if(inlimits[m] == 0) break;
		}
		if(m == f->arity()) {
			outtable = _structure->inter(f->outsort());
			outlimit[0] = outtable->size();
			if(outlimit[0] == 0) {
				// TODO
			}
			else {
				do {
					vector<Formula*> existvector;
					outtuple1[0] = 0;
					do {
						vector<Term*> vt(f->nrsorts());
						for(unsigned int a = 0; a < f->arity(); ++a) {
							ElementType tp = intables[a]->type();
							vt[a] = new DomainTerm(f->insort(a),tp,intables[a]->element(intuple[a]),ParseInfo());
						}
						vt.back() = new DomainTerm(f->outsort(),outtable->type(),outtable->element(outtuple1[0]),ParseInfo());
						existvector.push_back(new PredForm(true,f,vt,ParseInfo()));
						outtuple2[0] = outtuple1[0];
						while(nexttuple(outtuple2,outlimit)) {
							Formula* f1 = (existvector.back())->clone(); f1->swapsign();
							vector<Term*> vt2(f->nrsorts());
							for(unsigned int a = 0; a < f->arity(); ++a) {
								ElementType tp = intables[a]->type();
								vt2[a] = new DomainTerm(f->insort(a),tp,intables[a]->element(intuple[a]),ParseInfo());
							}
							vt2.back() = new DomainTerm(f->outsort(),outtable->type(),outtable->element(outtuple2[0]),ParseInfo());
							Formula* f2 = new PredForm(false,f,vt2,ParseInfo());
							vector<Formula*> vf(2); vf[0] = f1; vf[1] = f2;
							BoolForm* bf = new BoolForm(true,false,vf,ParseInfo());
							grounding->add(bf);
						} 
					} while(nexttuple(outtuple1,outlimit));
					BoolForm* bf = new BoolForm(true,false,existvector,ParseInfo());
					grounding->add(bf);
				} while(nexttuple(intuple,inlimits));
			}
		}
	}

	_returnTheory = grounding;
}
