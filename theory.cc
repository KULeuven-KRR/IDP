/************************************
	theory.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theory.h"
#include "visitor.h"
#include <iostream>

extern string tabstring(unsigned int);

/*******************
	Constructors
*******************/

/** Cloning while keeping free variables **/

PredForm* PredForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EqChainForm* EqChainForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EquivForm* EquivForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

BoolForm* BoolForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

QuantForm* QuantForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

/** Cloning while substituting free variables **/

PredForm* PredForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> na(_args.size());
	for(unsigned int n = 0; n < _args.size(); ++n) na[n] = _args[n]->clone(mvv);
	PredForm* pf = new PredForm(_sign,_symb,na,new ParseInfo(_pi));
	return pf;
}

EqChainForm* EqChainForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> nt(_terms.size());
	for(unsigned int n = 0; n < _terms.size(); ++n) nt[n] = _terms[n]->clone(mvv);
	EqChainForm* ef = new EqChainForm(_sign,_conj,nt,_comps,_signs,new ParseInfo(_pi));
	return ef;
}

EquivForm* EquivForm::clone(const map<Variable*,Variable*>& mvv) const {
	Formula* nl = _left->clone(mvv);
	Formula* nr = _right->clone(mvv);
	EquivForm* ef =  new EquivForm(_sign,nl,nr,new ParseInfo(_pi));
	return ef;
}

BoolForm* BoolForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Formula*> ns(_subf.size());
	for(unsigned int n = 0; n < _subf.size(); ++n) ns[n] = _subf[n]->clone(mvv);
	BoolForm* bf = new BoolForm(_sign,_conj,ns,new ParseInfo(_pi));
	return bf;
}

QuantForm* QuantForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Variable*> nv(_vars.size());
	map<Variable*,Variable*> nmvv = mvv;
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		nv[n] = new Variable(_vars[n]->name(),_vars[n]->sort(),new ParseInfo(_vars[n]->pi()));
		nmvv[_vars[n]] = nv[n];
	}
	Formula* nf = _subf->clone(nmvv);
	QuantForm* qf = new QuantForm(_sign,_univ,nv,nf,new ParseInfo(_pi));
	return qf;
}


/*****************
	Destructors
*****************/

PredForm::~PredForm() {
	delete(_pi);
	for(unsigned int n = 0; n < _args.size(); ++n) delete(_args[n]);
}

EqChainForm::~EqChainForm() {
	delete(_pi);
	for(unsigned int n = 0; n < _terms.size(); ++n) delete(_terms[n]);
}

EquivForm::~EquivForm() {
	delete(_pi);
	delete(_left);
	delete(_right);
}

BoolForm::~BoolForm() {
	delete(_pi);
	for(unsigned int n = 0; n < _subf.size(); ++n) delete(_subf[n]);
}

QuantForm::~QuantForm() {
	delete(_pi) ;
	delete(_subf);
	for(unsigned int n = 0; n < _vars.size(); ++n) delete(_vars[n]);
}

Rule::~Rule() {
	delete(_pi);
	delete(_head);
	delete(_body);
	for(unsigned int n = 0; n < nrQvars(); ++n) delete(qvar(n));
}

Definition::~Definition() {
	for(unsigned int n = 0; n < _rules.size(); ++n) delete(_rules[n]);
}

FixpDef::~FixpDef() {
	for(unsigned int n = 0; n < _rules.size(); ++n) delete(_rules[n]);
	for(unsigned int n = 0; n < _defs.size(); ++n) delete(_defs[n]);
}

Theory::~Theory() {
	delete(_pi);
	for(unsigned int n = 0; n < _sentences.size(); ++n) delete(_sentences[n]);
	for(unsigned int n = 0; n < _definitions.size(); ++n) delete(_definitions[n]);
	for(unsigned int n = 0; n < _fixpdefs.size(); ++n) delete(_fixpdefs[n]);
}

/*******************************
	Computing free variables
*******************************/

void Formula::setfvars() {
	_fvars.clear();
	for(unsigned int n = 0; n < nrSubterms(); ++n) {
		Term* t = subterm(n);
		t->setfvars();
		for(unsigned int m = 0; m < t->nrFvars(); ++m) { 
			unsigned int k = 0;
			for(; k < nrQvars(); ++k) {
				if(qvar(k) == t->fvar(m)) break;
			}
			if(k == nrQvars()) _fvars.push_back(t->fvar(m));
		}
	}
	for(unsigned int n = 0; n < nrSubforms(); ++n) {
		Formula* f = subform(n);
		f->setfvars();
		for(unsigned int m = 0; m < f->nrFvars(); ++m) {
			unsigned int k = 0;
			for(; k < nrQvars(); ++k) {
				if(qvar(k) == f->fvar(m)) break;
			}
			if(k == nrQvars()) _fvars.push_back(f->fvar(m));
		}
	}
	VarUtils::sortunique(_fvars);
}

/***********************************
	Adding rules and definitions
***********************************/

void Definition::add(Rule* r) {
	_rules.push_back(r);
	add(r->head()->symb());
}

void FixpDef::add(Rule* r) {
	_rules.push_back(r);
	add(r->head()->symb());
}

void Definition::add(PFSymbol* p) {
	unsigned int n = 0;
	for(; n < _defsyms.size(); ++n) {
		if(p == _defsyms[n]) return;
	}
	_defsyms.push_back(p);
}

void FixpDef::add(PFSymbol* p) {
	unsigned int n = 0;
	for(; n < _defsyms.size(); ++n) {
		if(p == _defsyms[n]) return;
	}
	_defsyms.push_back(p);
}

/***************************
	Containment checking
***************************/

bool Formula::contains(Variable* v) const {
	for(unsigned int n = 0; n < nrQvars(); ++n) {
		if(qvar(n) == v) return true;
	}
	for(unsigned int n = 0; n < nrSubterms(); ++n) {
		if(subterm(n)->contains(v)) return true;
	}
	for(unsigned int n = 0; n < nrSubforms(); ++n) {
		if(subform(n)->contains(v)) return true;
	}
	return false;
}

/****************
	Debugging
****************/

string PredForm::to_string() const {
	string s;
	if(!_sign) s = s + '~';
	s = s + _symb->to_string();
	if(_args.size()) {
		s = s + '(' + _args[0]->to_string();
		for(unsigned int n = 1; n < _args.size(); ++n) {
			s = s + ',' + _args[n]->to_string();
		}
		s = s + ')';
	}
	return s;
}

string EqChainForm::to_string() const {
	string s;
	if(!_sign) s = s + '~';
	s = s + "(" + _terms[0]->to_string();
	for(unsigned int n = 0; n < _comps.size(); ++n) {
		switch(_comps[n]) {
			case '=':
				if(_signs[n]) s = s + " = ";
				else s = s + " ~= ";
				break;
			case '<':
				if(_signs[n]) s = s + " < ";
				else s = s + " >= ";
				break;
			case '>':
				if(_signs[n]) s = s + " > ";
				else s = s + " =< ";
				break;
		}
		s = s + _terms[n+1]->to_string();
		if(!_conj && n+1 < _comps.size()) s = s + " | " + _terms[n+1]->to_string();
	}
	s = s + ")";
	return s;
}

string EquivForm::to_string() const {
	string s = '(' + _left->to_string() + " <=> " + _right->to_string() + ')';
	return s;
}

string BoolForm::to_string() const {
	string s;
	if(_subf.empty()) {
		if(_sign == _conj) return "True";
		else return "False";
	}
	if(!_sign) s = '~';
	s = s + '(' + _subf[0]->to_string();
	for(unsigned int n = 1; n < _subf.size(); ++n) {
		if(_conj) s = s + " & ";
		else s = s +  " | ";
		s = s + _subf[n]->to_string(); 
	}
	return s + ')';
}

string QuantForm::to_string() const {
	string s;
	if(!_sign) s = s + '~';
	s = s + '(';
	if(_univ) s = s + '!';
	else s = s + '?';
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		s = s + ' ' +  _vars[n]->to_string();
		if(_vars[n]->sort()) s = s + '[' + _vars[n]->sort()->name() + ']';
	}
	s = s + " : " + _subf->to_string();
	return s + ')';
}

string Rule::to_string() const {
	string s;
	if(!_vars.empty()) {
		s = "!";
		for(unsigned int n = 0; n < _vars.size(); ++n) {
			s = s + ' ' + _vars[n]->name();
			if(_vars[n]->sort()) s = s + '[' + _vars[n]->sort()->name() + ']';
		}
		s = s + " : ";
	}
	s = s + _head->to_string() + " <- " + _body->to_string() + '.';
	return s;
}

string Definition::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "{\n";
	for(unsigned int n = 0; n < _rules.size(); ++n) {
		s = s + tab + "  " + _rules[n]->to_string() + '\n';
	}
	s = s + tab + "}\n";
	return s;
}

string FixpDef::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + (_lfp ? "LFD " : "GFD ") + "[\n";
	for(unsigned int n = 0; n < _rules.size(); ++n) {
		s = s + tab + "  " + _rules[n]->to_string() + '\n';
	}
	for(unsigned int n = 0; n < _defs.size(); ++n) {
		s = s + _defs[n]->to_string(spaces+2);
	}
	s = s + tab + "]\n";
	return s;
}

string Theory::to_string() const {
	string s = "#theory " + _name;
	if(_vocabulary) {
		s = s + " : " + _vocabulary->name();
		if(_structure) s = s + _structure->name();
	}
	s = s + " {\n";
	for(unsigned int n = 0; n < _sentences.size(); ++n) {
		s = s + "   " + _sentences[n]->to_string() + ".\n";
	}
	for(unsigned int n = 0; n < _definitions.size(); ++n) {
		s = s + _definitions[n]->to_string(3);
	}
	for(unsigned int n = 0; n < _fixpdefs.size(); ++n) {
		s = s + _fixpdefs[n]->to_string(3);
	}
	return s + "}\n";
}

/*************************
	Rewriting theories
*************************/

/** Push negations inside **/

class NegationPush : public Visitor {

	public:
		NegationPush(Theory* t)	: Visitor() { traverse(t);	}

		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(BoolForm*);
		void visit(QuantForm*);

};

void NegationPush::visit(EqChainForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->conj(!f->conj());
		for(unsigned int n = 0; n < f->nrSubterms()-1; ++n)  f->compsign(n,!f->compsign(n));
	}
	traverse(f);
}

void NegationPush::visit(EquivForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->right()->swapsign();
	}
	traverse(f);
}

void NegationPush::visit(BoolForm* f) {
	if(!f->sign()) {
		f->swapsign();
		for(unsigned int n = 0; n < f->nrSubforms(); ++n) 
			f->subform(n)->swapsign();
		f->conj(!f->conj());
	}
	traverse(f);
}

void NegationPush::visit(QuantForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->subf()->swapsign();
		f->univ(!f->univ());
	}
	traverse(f);
}

namespace TheoryUtils {
	void push_negations(Theory* t) { NegationPush np(t);	}
}
