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
	return new DomainTerm(_sort,_type,_value,_pi);
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

class SetEvaluator : public Visitor {

	private:
		vector<SortTable*>			_truevalues;
		vector<SortTable*>			_unknvalues;
		AbstractStructure*			_structure;
		map<Variable*,TypedElement>	_varmapping;

	public:
		SetEvaluator(SetExpr* e, AbstractStructure* s, const map<Variable*,TypedElement> m) :
			Visitor(), _structure(s), _varmapping(m) { e->accept(this);	}

		const vector<SortTable*>& truevalues()	const	{ return _truevalues;	}
		const vector<SortTable*>& unknvalues()	const	{ return _unknvalues;	}

		void visit(EnumSetExpr*);
		void visit(QuantSetExpr*);

};

void SetEvaluator::visit(EnumSetExpr* e) {
	for(unsigned int n = 0; n < e->nrSubforms(); ++n) {
		TruthValue tv = FormulaUtils::evaluate(e->subform(n),_structure,_varmapping);
		switch(tv) {
			case TV_TRUE:
			{
				TermEvaluator te(e->subterm(n),_structure,_varmapping);
				FiniteSortTable* st = te.returnvalue();
				if(st->empty()) delete(st);
				else _truevalues.push_back(st);
				break;
			}
			case TV_FALSE:
				break;
			case TV_UNKN:
			{
				TermEvaluator te(e->subterm(n),_structure,_varmapping);
				FiniteSortTable* st = te.returnvalue();
				if(st->empty()) delete(st);
				else _unknvalues.push_back(st);
				break;
			}
		}
	}
}

void SetEvaluator::visit(QuantSetExpr* e)	{
	SortTableTupleIterator vti(e->qvars(),_structure);
	if(!vti.empty()) {
		for(unsigned int n; n < e->nrQvars(); ++n) {
			TypedElement te; 
			te._type = vti.type(n); 
			_varmapping[e->qvar(n)] = te;
		}
		do {
			for(unsigned int n = 0; n < e->nrQvars(); ++n) 
				_varmapping[e->qvar(n)]._element = vti.value(n);
			SortTable* sst = TableUtils::singletonSort(_varmapping[e->qvar(0)]);
			TruthValue tv = FormulaUtils::evaluate(e->subf(),_structure,_varmapping);
			switch(tv) {
				case TV_TRUE:
					_truevalues.push_back(sst);
					break;
				case TV_FALSE:
					delete(sst);
					break;
				case TV_UNKN:
					_unknvalues.push_back(sst);
					break;
			}
		} while(vti.nextvalue());
	}
}

TermEvaluator::TermEvaluator(AbstractStructure* s,const map<Variable*,TypedElement> m) : 
	Visitor(), _structure(s), _varmapping(m) { }
TermEvaluator::TermEvaluator(Term* t,AbstractStructure* s,const map<Variable*,TypedElement> m) : 
	Visitor(), _structure(s), _varmapping(m) { t->accept(this);	}

void TermEvaluator::visit(VarTerm* vt) {
	assert(_varmapping.find(vt->var()) != _varmapping.end());
	_returnvalue = TableUtils::singletonSort(_varmapping[vt->var()]);
}

void TermEvaluator::visit(DomainTerm* dt) {
	_returnvalue = TableUtils::singletonSort(dt->value(),dt->type());
}

void TermEvaluator::visit(FuncTerm* ft) {
	// Calculate the value of the subterms
	vector<SortTable*> argvalues;
	for(unsigned int n = 0; n < ft->nrSubterms(); ++n) {
		ft->subterm(n)->accept(this);
		if(_returnvalue->empty()) {
			for(unsigned int m = 0; m < argvalues.size(); ++m) delete(argvalues[m]);
			return;
		}
		else argvalues.push_back(_returnvalue);
	}
	// Calculate the value of the function
	SortTableTupleIterator stti(argvalues);
	Function* f = ft->func();
	FuncInter* fi = _structure->inter(f);
	assert(fi);
	_returnvalue = new EmptySortTable();
	if(fi->functable()) {	// Two-valued function
		FuncTable* fut = fi->functable();
		ElementType et = fut->type(fut->arity());
		do {
			vector<TypedElement> vet(f->arity());
			for(unsigned int n = 0; n < f->arity(); ++n) { 
				vet[n]._element = stti.value(n);
				vet[n]._type = stti.type(n);
			}
			Element e = (*fut)[vet];
			if(ElementUtil::exists(e,et)) {
				FiniteSortTable* st = _returnvalue->add(e,et);
				if(st != _returnvalue) { 
					delete(_returnvalue);	
					_returnvalue = st;
				}
			}
		} while(stti.nextvalue());
	}
	else {	// Three-valued function
		PredInter* pi = fi->predinter();
		PredTable* pt = pi->cfpt();
		vector<bool> vb(pt->arity(),true); vb.back() = false;
		do {
			vector<TypedElement> vet(pt->arity());
			for(unsigned int n = 0; n < f->arity(); ++n) {
				vet[n]._element = stti.value(n);
				vet[n]._type = stti.type(n);
			}
			PredTable* ppt = TableUtils::project(pt,vet,vb);
			if(pi->cf()) {
				PredTable* ippt = StructUtils::complement(ppt,vector<Sort*>(1,f->outsort()),_structure);
				delete(ppt);
				ppt = ippt;
			}
			assert(ppt->finite());
			for(unsigned int n = 0; n < ppt->size(); ++n) {
				FiniteSortTable* st = _returnvalue->add(ppt->element(n,0),ppt->type(0));
				if(st != _returnvalue) {
					delete(_returnvalue);
					_returnvalue = st;
				}
			}
		} while(stti.nextvalue());
	}
	_returnvalue->sortunique();
	for(unsigned int n = 0; n < argvalues.size(); ++n) delete(argvalues[n]);
}

void TermEvaluator::visit(AggTerm* at) {
	SetEvaluator sev(at->set(),_structure,_varmapping);
	if(at->type() == AGGCARD) {
		int tv = sev.truevalues().size();
		int uv = tv + sev.unknvalues().size();
		_returnvalue = new RanSortTable(tv,uv);
		return;
	}
	else {
		_returnvalue = new EmptySortTable();
		SortTableTupleIterator ittrue(sev.truevalues());
		vector<double> tvs(sev.truevalues().size());
		assert(!ittrue.empty());
		do {
			// compute the value of the true part
			for(unsigned int n = 0; n < tvs.size(); ++n) 
				tvs[n] = (ElementUtil::convert(ittrue.value(n),ittrue.type(n),ELDOUBLE))._double;
			double tv = AggUtils::compute(at->type(),tvs);
			// choose which unknown values will be treated as true
			vector<unsigned int> limits(sev.unknvalues().size(),1);
			vector<unsigned int> tuple(limits.size(),0);
			do {
				vector<SortTable*> vst;
				for(unsigned int n = 0; n < tuple.size(); ++n) {
					if(tuple[n]) vst.push_back((sev.unknvalues())[n]);
				}
				vector<double> uvs(vst.size()+1);
				uvs[0] = tv;
				SortTableTupleIterator itunkn(vst);
				assert(!itunkn.empty());
				do {
					for(unsigned int n = 1; n < uvs.size(); ++n) 
						uvs[n] = (ElementUtil::convert(itunkn.value(n-1),itunkn.type(n-1),ELDOUBLE))._double;
					double tuv = AggUtils::compute(at->type(),uvs);
					FiniteSortTable* temp = _returnvalue->add(tuv);
					if(temp != _returnvalue) {
						delete(_returnvalue);
						_returnvalue = temp;
					}
				} while(itunkn.nextvalue());
			} while(nexttuple(tuple,limits));
		} while(ittrue.nextvalue());
	}
	// delete tables
	for(unsigned int n = 0; n < sev.truevalues().size(); ++n) delete((sev.truevalues())[n]);
	for(unsigned int n = 0; n < sev.unknvalues().size(); ++n) delete((sev.unknvalues())[n]);
}

namespace TermUtils {
	FiniteSortTable* evaluate(Term* t,AbstractStructure* s,const map<Variable*,TypedElement> m) {
		TermEvaluator te(t,s,m);
		return te.returnvalue();
	}
}

namespace AggUtils {

	double compute(AggType agg, const vector<double>& args) {
		double d;
		switch(agg) {
			case AGGCARD:
				d = double(args.size());
				break;
			case AGGSUM:
				d = 0;
				for(unsigned int n = 0; n < args.size(); ++n) d += args[n];
				break;
			case AGGPROD:
				d = 1;
				for(unsigned int n = 0; n < args.size(); ++n) d = d * args[n];
				break;
			case AGGMIN:
				d = MAX_DOUBLE;
				for(unsigned int n = 0; n < args.size(); ++n) d = (d <= args[n] ? d : args[n]);
				break;
			case AGGMAX:
				d = MIN_DOUBLE;
				for(unsigned int n = 0; n < args.size(); ++n) d = (d >= args[n] ? d : args[n]);
				break;
		}
		return d;
	}

}
