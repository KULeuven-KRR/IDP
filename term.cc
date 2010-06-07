/************************************
	term.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "namespace.h"
#include "builtin.h"

extern string itos(int);
extern string dtos(double);


/*******************
	Constructors
*******************/

VarTerm::VarTerm(Variable* v, ParseInfo* pi) : Term(pi), _var(v) {
	setfvars();
}

FuncTerm::FuncTerm(Function* f, const vector<Term*>& a, ParseInfo* pi) : Term(pi), _func(f), _args(a) { 
	setfvars();
}

/******************
	Destructors
******************/

FuncTerm::~FuncTerm() {
	delete(_pi);
	for(unsigned int n = 0; n < _args.size(); ++n) delete(_args[n]);
}

EnumSetExpr::~EnumSetExpr() {
	delete(_pi);
	for(unsigned int n = 0; n < _subf.size(); ++n) delete(_subf[n]);
	for(unsigned int n = 0; n < _weights.size(); ++n) delete(_weights[n]);
}

QuantSetExpr::~QuantSetExpr() {
	delete(_pi);
	delete(_subf);
	for(unsigned int n = 0; n < _vars.size(); ++n) delete(_vars[n]);
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
	if(_type == AGGCARD) return Builtin::intsort();
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
			return dtos(*(_value._double));
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
