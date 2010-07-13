/************************************
	term.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "namespace.hpp"
#include "builtin.hpp"
#include "visitor.hpp"

extern string itos(int);
extern string dtos(double);


/*******************
	Constructors
*******************/

VarTerm::VarTerm(Variable* v, const ParseInfo& pi) : Term(pi), _var(v) {
	setfvars();
}

FuncTerm::FuncTerm(Function* f, const vector<Term*>& a, const ParseInfo& pi) : Term(pi), _func(f), _args(a) { 
	setfvars();
}


/** Cloning while keeping free variables **/

VarTerm* VarTerm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

FuncTerm* FuncTerm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

DomainTerm* DomainTerm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

AggTerm* AggTerm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EnumSetExpr* EnumSetExpr::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

QuantSetExpr* QuantSetExpr::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

/** Cloning while substituting free variables **/

VarTerm* VarTerm::clone(const map<Variable*,Variable*>& mvv) const {
	map<Variable*,Variable*>::const_iterator it = mvv.find(_var);
	if(it != mvv.end()) return new VarTerm(it->second,_pi);
	else return new VarTerm(_var,_pi);
}

FuncTerm* FuncTerm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> na(_args.size());
	for(unsigned int n = 0; n < _args.size(); ++n) na[n] = _args[n]->clone(mvv);
	return new FuncTerm(_func,na,_pi);
}

DomainTerm* DomainTerm::clone(const map<Variable*,Variable*>& mvv) const {
	Element ne;
	switch(_type) {
		case ELINT: ne._int = _value._int; break;
		case ELDOUBLE: ne._double = _value._double; break;
		case ELSTRING: ne._string = _value._string; break;
		default: assert(false);
	}
	return new DomainTerm(_sort,_type,ne,_pi);
}

AggTerm* AggTerm::clone(const map<Variable*,Variable*>& mvv) const {
	SetExpr* ns = _set->clone(mvv);
	return new AggTerm(ns,_type,_pi);
}

EnumSetExpr* EnumSetExpr::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Formula*> nf(_subf.size());
	vector<Term*> nt(_weights.size());
	for(unsigned int n = 0; n < _subf.size(); ++n) nf[n] = _subf[n]->clone(mvv);
	for(unsigned int n = 0; n < _weights.size(); ++n) nt[n] = _weights[n]->clone(mvv);
	return new EnumSetExpr(nf,nt,_pi);
}

QuantSetExpr* QuantSetExpr::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Variable*> nv(_vars.size());
	map<Variable*,Variable*> nmvv = mvv;
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		nv[n] = new Variable(_vars[n]->name(),_vars[n]->sort(),_vars[n]->pi());
		nmvv[_vars[n]] = nv[n];
	}
	Formula* nf = _subf->clone(nmvv);
	return new QuantSetExpr(nv,nf,_pi);
}

/******************
	Destructors
******************/

void FuncTerm::recursiveDelete() {
	for(unsigned int n = 0; n < _args.size(); ++n) _args[n]->recursiveDelete();
	delete(this);
}

void DomainTerm::recursiveDelete() {
	delete(this);
}

void EnumSetExpr::recursiveDelete() {
	for(unsigned int n = 0; n < _subf.size(); ++n) _subf[n]->recursiveDelete();
	for(unsigned int n = 0; n < _weights.size(); ++n) _weights[n]->recursiveDelete();
	delete(this);
}

void QuantSetExpr::recursiveDelete() {
	_subf->recursiveDelete();
	for(unsigned int n = 0; n < _vars.size(); ++n) delete(_vars[n]);
	delete(this);
}

/*******************************
	Computing free variables
*******************************/

/** Compute free variables  **/

void Term::setfvars() {
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
	for(unsigned int n = 0; n < nrSubsets(); ++n) {
		SetExpr* s = subset(n);
		s->setfvars();
		for(unsigned int m = 0; m < s->nrFvars(); ++m) {
			unsigned int k = 0;
			for(; k < nrQvars(); ++k) {
				if(qvar(k) == s->fvar(m)) break;
			}
			if(k == nrQvars()) _fvars.push_back(s->fvar(m));
		}
	}
	VarUtils::sortunique(_fvars);
}

void VarTerm::setfvars() {
	_fvars.clear();
	_fvars = vector<Variable*>(1,_var);
}

void SetExpr::setfvars() {
	_fvars.clear();
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

/************************
	Sort of aggregates
************************/

Sort* EnumSetExpr::firstargsort() const {
	if(_weights.empty()) return 0;
	else return _weights[0]->sort();
}

Sort* QuantSetExpr::firstargsort() const {
	if(_vars.empty()) return 0;
	else return _vars[0]->sort();
}

Sort* AggTerm::sort() const {
	if(_type == AGGCARD) return stdbuiltin()->sort("nat");
	else return _set->firstargsort();
}

/***************************
	Containment checking
***************************/

bool Term::contains(Variable* v) const {
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

string FuncTerm::to_string() const {
	string s = _func->to_string();
	if(_args.size()) {
		s = s + '(' + _args[0]->to_string();
		for(unsigned int n = 1; n < _args.size(); ++n) {
			s = s + ',' + _args[n]->to_string();
		}
		s = s + ')';
	}
	return s;
}

string DomainTerm::to_string() const {
	switch(_type) {
		case ELINT:
			return itos(_value._int);
		case ELDOUBLE:
			return dtos(_value._double);
		case ELSTRING:
			return *(_value._string);
		default:
			assert(false);
	}
	return "";
}

string EnumSetExpr::to_string() const {
	string s = "[ ";
	if(_subf.size()) {
		s = s + _subf[0]->to_string();
		for(unsigned int n = 1; n < _subf.size(); ++n) {
			s = s + ',' + _subf[n]->to_string();
		}
	}
	s = s + " ]";
	return s;
}

string QuantSetExpr::to_string() const {
	string s = "{";
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		s = s + ' ' + _vars[n]->to_string();
		if(_vars[n]->sort()) s = s + '[' + _vars[n]->sort()->name() + ']';
	}
	s = s + ": " + _subf->to_string() + " }";
	return s;
}

string AggTypeNames[5] = { "#", "sum", "prod", "min", "max" };
string makestring(const AggType& t) {
	return AggTypeNames[t];
}

string AggTerm::to_string() const {
	string s = makestring(_type) + _set->to_string();
	return s;
}


/*****************
	Term utils
*****************/

TermEvaluator::TermEvaluator(Structure* s,const map<Variable*,TypedElement> m) : 
	Visitor(), _structure(s), _varmapping(m) { }
TermEvaluator::TermEvaluator(Term* t,Structure* s,const map<Variable*,TypedElement> m) : 
	Visitor(), _structure(s), _varmapping(m) { t->accept(this);	}

void TermEvaluator::visit(VarTerm* vt) {
	assert(_varmapping.find(vt->var()) != _varmapping.end());
	_returnvalue = _varmapping[vt->var()];
}

void TermEvaluator::visit(FuncTerm* ft) {
	vector<TypedElement> argvalues;
	for(unsigned int n = 0; n < ft->nrSubterms(); ++n) {
		ft->subterm(n)->accept(this);
		if(ElementUtil::exists(_returnvalue)) argvalues.push_back(_returnvalue);
		else return;
	}
	FuncInter* fi = _structure->inter(ft->func());
	assert(fi);
	_returnvalue._element = (*(fi->functable()))[argvalues];
	_returnvalue._type = fi->functable()->type(fi->functable()->arity());
}

void TermEvaluator::visit(DomainTerm* dt) {
	_returnvalue._element = dt->value();
	_returnvalue._type = dt->type();
}

void TermEvaluator::visit(AggTerm* at) {
	assert(false); // TODO: not yet implemented
}

namespace TermUtils {
	TypedElement evaluate(Term* t,Structure* s,const map<Variable*,TypedElement> m) {
		TermEvaluator te(t,s,m);
		return te.returnvalue();
	}
}
