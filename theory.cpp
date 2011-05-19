/************************************
	theory.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cassert>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
using namespace std;

/**********************
	TheoryComponent
**********************/

string TheoryComponent::to_string(unsigned int spaces) const {
	stringstream sstr;
	put(sstr,spaces);
	return sstr.str();
}

/**************
	Formula
**************/

void Formula::setfvars() {
	_freevars.clear();
	for(vector<Term*>::const_iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		_freevars.insert((*it)->freevars().begin(),(*it)->freevars().end());
	}
	for(vector<Formula*>::const_iterator it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		_freevars.insert((*it)->freevars().begin(),(*it)->freevars().end());
	}
	for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		_freevars.erase(*it);
	}
}

void Formula::recursiveDelete() {
	for(vector<Term*>::iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(vector<Formula*>::iterator it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(set<Variable*>::iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		delete(*it);
	}
	delete(this);
}

bool Formula::contains(const Variable* v) const {
	for(set<Variable*>::const_iterator it = _freevars.begin(); it != _freevars.end(); ++it) {
		if(*it == v) return true;
	}
	for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		if(*it == v) return true;
	}
	for(vector<Term*>::const_iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		if((*it)->contains(v)) return true;
	}
	for(vector<Formula*>::const_iterator it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		if((*it)->contains(v)) return true;
	}
	return false;
}

class ContainmentChecker : public TheoryVisitor {
	private:
		const PFSymbol*	_symbol;
		bool			_result;

		void visit(const PredForm* pf) { 
			if(pf->symbol() == _symbol) {
				_result = true;
				return;
			}
			else traverse(pf);	
		}

		void visit(const FuncTerm* ft) { 
			if(ft->function() == _symbol) {
				_result = true;
				return;
			}
			else traverse(ft);
		}

	public:
		ContainmentChecker(const PFSymbol* s) : 
			TheoryVisitor(), _symbol(s) { }

		bool run(const Formula* f) { 
			_result = false;
			f->accept(this);
			return _result;
		}

};

bool Formula::contains(const PFSymbol* s) const {
	ContainmentChecker cc(s);
	return cc.run(this);
}

ostream& operator<<(ostream& output, const TheoryComponent& f) {
	return f.put(output);
}

ostream& operator<<(ostream& output, const Formula& f) {
	return f.put(output);
}

/***************
	PredForm
***************/

PredForm* PredForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

PredForm* PredForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> na;
	for(vector<Term*>::const_iterator it = subterms().begin(); it != subterms().end(); ++it) 
		na.push_back((*it)->clone(mvv));
	PredForm* pf = new PredForm(sign(),_symbol,na,pi().clone(mvv));
	return pf;
}

void PredForm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Formula* PredForm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& PredForm::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	if(!sign()) output << '~';
	output << *_symbol;
	if(typeid(*_symbol) == typeid(Predicate)) {
		if(!subterms().empty()) {
			output << '(' << *subterms()[0];
			for(unsigned int n = 1; n < subterms().size(); ++n) 
				output << ',' << *subterms()[n];
			output << ')';
		}
	}
	else {
		if(subterms().size() > 1) {
			output << '(' << *subterms()[0];
			for(unsigned int n = 1; n < subterms().size()-1; ++n) 
				output << ',' << *subterms()[n];
			output << ')';
		}
		output << " = " << *subterms().back();
	}
	return output;
}

/******************
	EqChainForm
******************/

EqChainForm* EqChainForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EqChainForm* EqChainForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> nt;
	for(vector<Term*>::const_iterator it = subterms().begin(); it != subterms().end(); ++it) 
		nt.push_back((*it)->clone(mvv));
	EqChainForm* ef = new EqChainForm(sign(),_conj,nt,_comps,pi().clone(mvv));
	return ef;
}

void EqChainForm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Formula* EqChainForm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& EqChainForm::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	if(!sign()) output << '~';
	output << '(' << *subterms()[0];
	for(unsigned int n = 0; n < _comps.size(); ++n) {
		switch(_comps[n]) {
			case CT_EQ: output << " = "; break;
			case CT_NEQ: output << " ~= "; break;
			case CT_LT: output << " < "; break;
			case CT_GT: output << " > "; break;
			case CT_LEQ: output << " =< "; break;
			case CT_GEQ: output << " >= "; break;
			default: assert(false);
		}
		output << *subterms()[n+1];
		if(!_conj && n+1 < _comps.size()) output << " | " << *subterms()[n+1];
	}
	output << ')';
	return output;
}

/****************
	EquivForm
****************/

EquivForm* EquivForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EquivForm* EquivForm::clone(const map<Variable*,Variable*>& mvv) const {
	Formula* nl = left()->clone(mvv);
	Formula* nr = right()->clone(mvv);
	EquivForm* ef =  new EquivForm(sign(),nl,nr,pi().clone(mvv));
	return ef;
}

void EquivForm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Formula* EquivForm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& EquivForm::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	output << '(' << *left() << " <=> " << *right() << ')';
	return output;
}

/***************
	BoolForm
***************/

BoolForm* BoolForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

BoolForm* BoolForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Formula*> ns;
	for(vector<Formula*>::const_iterator it = subformulas().begin(); it != subformulas().end(); ++it) 
		ns.push_back((*it)->clone(mvv));
	BoolForm* bf = new BoolForm(sign(),_conj,ns,pi().clone(mvv));
	return bf;
}

void BoolForm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Formula* BoolForm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& BoolForm::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	if(subformulas().empty()) {
		if(sign() == _conj) output << "true";
		else output << "false";
	}
	else {
		if(!sign()) output << '~';
		output << '(' << *subformulas()[0];
		for(unsigned int n = 1; n < subformulas().size(); ++n) {
			if(_conj) output << " & ";
			else output <<  " | ";
			output << *subformulas()[n]; 
		}
		output << ')';
	}
	return output;
}

/****************
	QuantForm
****************/

QuantForm* QuantForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

QuantForm* QuantForm::clone(const map<Variable*,Variable*>& mvv) const {
	set<Variable*> nv;
	map<Variable*,Variable*> nmvv = mvv;
	for(set<Variable*>::const_iterator it = quantvars().begin(); it != quantvars().end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),pi());
		nv.insert(v);
		nmvv[*it] = v;
	}
	Formula* nf = subf()->clone(nmvv);
	QuantForm* qf = new QuantForm(sign(),_univ,nv,nf,pi().clone(mvv));
	return qf;
}

void QuantForm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Formula* QuantForm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& QuantForm::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	if(!sign()) output << '~';
	output << '(';
	output << (_univ ? '!' : '?');
	for(set<Variable*>::const_iterator it = quantvars().begin(); it != quantvars().end(); ++it) {
		output << ' ' << *(*it);
		if((*it)->sort()) output << '[' << (*it)->sort()->name() << ']';
	}
	output << " : " << *subf() << ')';
	return output;
}

/**************
	AggForm
**************/

AggForm::AggForm(bool sign, Term* l, CompType c, AggTerm* r, const FormulaParseInfo& pi) :
	Formula(sign,pi), _comp(c), _aggterm(r) { 
	addsubterm(l); 
	addsubterm(r); 
}

AggForm* AggForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

AggForm* AggForm::clone(const map<Variable*,Variable*>& mvv) const {
	Term* nl = left()->clone(mvv);
	AggTerm* nr = right()->clone(mvv);
	return new AggForm(sign(),nl,_comp,nr,pi().clone(mvv));
}

void AggForm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Formula* AggForm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& AggForm::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	if(!sign()) output << '~';
	output << '(' << *left();
	switch(_comp) {
		case CT_EQ: output << " = "; break;
		case CT_NEQ: output << " ~= "; break;
		case CT_LT: output << " < "; break;
		case CT_GT: output << " > "; break;
		case CT_LEQ: output << " =< "; break;
		case CT_GEQ: output << " >= "; break;
		default: assert(false);
	}
	output << *right() << ')';
	return output;
}

/***********
	Rule
***********/

Rule* Rule::clone() const {
	map<Variable*,Variable*> mvv;
	set<Variable*> newqv;
	for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		newqv.insert(v);
	}
	return new Rule(newqv,_head->clone(mvv),_body->clone(mvv),_pi);
}

void Rule::recursiveDelete() {
	_head->recursiveDelete();
	_body->recursiveDelete();
	for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) delete(*it);
	delete(this);
}

void Rule::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Rule* Rule::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}


ostream& Rule::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	if(!_quantvars.empty()) {
		output << "!";
		for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
			output << ' ' << (*it)->name();
			if((*it)->sort()) output << '[' << (*it)->sort()->name() << ']';
		}
		output << " : ";
	}
	output << *_head << " <- " << *_body << '.';
	return output;
}

string Rule::to_string(unsigned int spaces) const {
	stringstream sstr;
	put(sstr,spaces);
	return sstr.str();
}

ostream& operator<<(ostream& output, const Rule& r) {
	return r.put(output);
}

/******************
	Definitions
******************/

Definition* Definition::clone() const {
	Definition* newdef = new Definition();
	for(vector<Rule*>::const_iterator it = _rules.begin(); it != _rules.end(); ++it)
		newdef->add((*it)->clone());
	return newdef;
}

void Definition::recursiveDelete() {
	for(unsigned int n = 0; n < _rules.size(); ++n) _rules[n]->recursiveDelete();
	delete(this);
}

void Definition::add(Rule* r) {
	_rules.push_back(r);
	_defsyms.insert(r->head()->symbol());
}

void Definition::rule(unsigned int n, Rule* r) {
	_rules[n] = r;
	_defsyms.clear();
	for(vector<Rule*>::const_iterator it = _rules.begin(); it != _rules.end(); ++it) 
		_defsyms.insert((*it)->head()->symbol());
}

void Definition::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Definition* Definition::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& Definition::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	output << "{ ";
	if(!_rules.empty()) {
		output << *_rules[0];
		for(unsigned int n = 1; n < _rules.size(); ++n) {
			output << '\n';
			_rules[n]->put(output,spaces+2);
		}
	}
	output << '}';
	return output;
}

/***************************
	Fixpoint definitions
***************************/

FixpDef* FixpDef::clone() const {
	FixpDef* newfd = new FixpDef(_lfp);
	for(vector<FixpDef*>::const_iterator it = _defs.begin(); it != _defs.end(); ++it) 
		newfd->add((*it)->clone());
	for(vector<Rule*>::const_iterator it = _rules.begin(); it != _rules.end(); ++it) 
		newfd->add((*it)->clone());
	return newfd;
}

void FixpDef::recursiveDelete() {
	for(vector<FixpDef*>::const_iterator it = _defs.begin(); it != _defs.end(); ++it) 
		delete(*it);
	for(vector<Rule*>::const_iterator it = _rules.begin(); it != _rules.end(); ++it) 
		delete(*it);
	delete(this);
}

void FixpDef::add(Rule* r) {
	_rules.push_back(r);
	_defsyms.insert(r->head()->symbol());
}

void FixpDef::rule(unsigned int n, Rule* r) {
	_rules[n] = r;
	_defsyms.clear();
	for(vector<Rule*>::const_iterator it = _rules.begin(); it != _rules.end(); ++it) 
		_defsyms.insert((*it)->head()->symbol());
}

void FixpDef::accept(TheoryVisitor* v) const {
	v->visit(this);
}

FixpDef* FixpDef::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& FixpDef::put(ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	output << (_lfp ? "LFD [  " : "GFD [  ");
	if(!_rules.empty()) {
		output << *_rules[0];
		for(unsigned int n = 1; n < _rules.size(); ++n) {
			output << '\n';
			_rules[n]->put(output,spaces+2);
		}
	}
	for(vector<FixpDef*>::const_iterator it = _defs.begin(); it != _defs.end(); ++it) {
		output << '\n';
		(*it)->put(output,spaces+2);
	}
	output << " ]";
	return output;
}

/***************
	Theories
***************/

Theory* Theory::clone() const {
	Theory* newtheory = new Theory(_name,_vocabulary,ParseInfo());
	for(vector<Formula*>::const_iterator it = _sentences.begin(); it != _sentences.end(); ++it)
		newtheory->add((*it)->clone());
	for(vector<Definition*>::const_iterator it = _definitions.begin(); it != _definitions.end(); ++it)
		newtheory->add((*it)->clone());
	for(vector<FixpDef*>::const_iterator it = _fixpdefs.begin(); it != _fixpdefs.end(); ++it)
		newtheory->add((*it)->clone());
	return newtheory;
}

void Theory::addTheory(AbstractTheory* ) {
	// TODO
}

void Theory::recursiveDelete() {
	for(vector<Formula*>::iterator it = _sentences.begin(); it != _sentences.end(); ++it)
		(*it)->recursiveDelete();
	for(vector<Definition*>::iterator it = _definitions.begin(); it != _definitions.end(); ++it)
		(*it)->recursiveDelete();
	for(vector<FixpDef*>::iterator it = _fixpdefs.begin(); it != _fixpdefs.end(); ++it)
		(*it)->recursiveDelete();
	delete(this);
}

set<TheoryComponent*> Theory::components() const {
	set<TheoryComponent*> stc;
	for(vector<Formula*>::const_iterator it = _sentences.begin(); it != _sentences.end(); ++it) stc.insert(*it);
	for(vector<Definition*>::const_iterator it = _definitions.begin(); it != _definitions.end(); ++it) stc.insert(*it);
	for(vector<FixpDef*>::const_iterator it = _fixpdefs.begin(); it != _fixpdefs.end(); ++it) stc.insert(*it);
	return stc;
}

void Theory::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Theory* Theory::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

std::ostream& Theory::put(std::ostream& output, unsigned int spaces) const {
	printtabs(output,spaces);
	output << "#theory " <<  _name;
	if(_vocabulary) {
		output << " : " << _vocabulary->name();
	}
	output << " {\n";
	for(vector<Formula*>::const_iterator it = _sentences.begin(); it != _sentences.end(); ++it)
		(*it)->put(output,spaces+2);
	for(vector<Definition*>::const_iterator it = _definitions.begin(); it != _definitions.end(); ++it)
		(*it)->put(output,spaces+2);
	for(vector<FixpDef*>::const_iterator it = _fixpdefs.begin(); it != _fixpdefs.end(); ++it)
		(*it)->put(output,spaces+2);
	output << "}\n";
	return output;
}


/********************
	Formula utils
********************/

class NegationPush : public TheoryMutatingVisitor {

	public:
		NegationPush()	: TheoryMutatingVisitor() { }

		Formula*	visit(EqChainForm*);
		Formula* 	visit(EquivForm*);
		Formula* 	visit(BoolForm*);
		Formula* 	visit(QuantForm*);

		Formula*	traverse(Formula*);
		Term*		traverse(Term*);

};

Formula* NegationPush::traverse(Formula* f) {
	for(vector<Formula*>::const_iterator it = f->subformulas().begin(); it != f->subformulas().end(); ++it)
		(*it)->accept(this);
	for(vector<Term*>::const_iterator it = f->subterms().begin(); it != f->subterms().end(); ++it)
		(*it)->accept(this);
	return f;
}

Term* NegationPush::traverse(Term* t) {
	for(vector<Term*>::const_iterator it = t->subterms().begin(); it != t->subterms().end(); ++it)
		(*it)->accept(this);
	for(vector<SetExpr*>::const_iterator it = t->subsets().begin(); it != t->subsets().end(); ++it)
		(*it)->accept(this);
	return t;
}

Formula* NegationPush::visit(EqChainForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->conj(!f->conj());
		for(unsigned int n = 0; n < f->comps().size(); ++n) 
			f->comp(n,negatecomp(f->comps()[n]));
	}
	return traverse(f);
}

Formula* NegationPush::visit(EquivForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->right()->swapsign();
	}
	return traverse(f);
}

Formula* NegationPush::visit(BoolForm* f) {
	if(!f->sign()) {
		f->swapsign();
		for(vector<Formula*>::const_iterator it = f->subformulas().begin(); it != f->subformulas().end(); ++it) 
			(*it)->swapsign();
		f->conj(!f->conj());
	}
	return traverse(f);
}

Formula* NegationPush::visit(QuantForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->subf()->swapsign();
		f->univ(!f->univ());
	}
	return traverse(f);
}

class EquivRemover : public TheoryMutatingVisitor {

	public:
		EquivRemover()	: TheoryMutatingVisitor() { }

		BoolForm* visit(EquivForm*);

};

BoolForm* EquivRemover::visit(EquivForm* ef) {
	Formula* nl = ef->left()->accept(this);
	Formula* nr = ef->right()->accept(this);
	vector<Formula*> vf1(2);
	vector<Formula*> vf2(2);
	vf1[0] = nl; vf1[1] = nr;
	vf2[0] = nl->clone(); vf2[1] = nr->clone();
	vf1[0]->swapsign(); vf2[1]->swapsign();
	BoolForm* bf1 = new BoolForm(true,false,vf1,ef->pi());
	BoolForm* bf2 = new BoolForm(true,false,vf2,ef->pi());
	vector<Formula*> vf(2); vf[0] = bf1; vf[1] = bf2;
	BoolForm* bf = new BoolForm(ef->sign(),true,vf,ef->pi());
	delete(ef);
	return bf;
}

class Flattener : public TheoryMutatingVisitor {

	public:
		Flattener() : TheoryMutatingVisitor() { }

		Formula*	visit(BoolForm*);
		Formula* 	visit(QuantForm*);

};

Formula* Flattener::visit(BoolForm* bf) {
	vector<Formula*> newsubf;
	traverse(bf);
	for(vector<Formula*>::const_iterator it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
		if(typeid(*(*it)) == typeid(BoolForm)) {
			BoolForm* sbf = dynamic_cast<BoolForm*>(*it);
			if((bf->conj() == sbf->conj()) && sbf->sign()) {
				for(vector<Formula*>::const_iterator jt = sbf->subformulas().begin(); jt != sbf->subformulas().end(); ++jt)
					newsubf.push_back(*jt);
				delete(sbf);
			}
			else newsubf.push_back(*it);
		}
		else newsubf.push_back(*it);
	}
	bf->subformulas(newsubf);
	return bf;
}

Formula* Flattener::visit(QuantForm* qf) {
	traverse(qf);	
	if(typeid(*(qf->subf())) == typeid(QuantForm)) {
		QuantForm* sqf = dynamic_cast<QuantForm*>(qf->subf());
		if((qf->univ() == sqf->univ()) && sqf->sign()) {
			qf->subf(sqf->subf());
			for(set<Variable*>::const_iterator it = sqf->quantvars().begin(); it != sqf->quantvars().end(); ++it) 
				qf->add(*it);
			delete(sqf);
		}
	}
	return qf;
}

class EqChainRemover : public TheoryMutatingVisitor {

	private:
		Vocabulary* _vocab;

	public:
		EqChainRemover() : TheoryMutatingVisitor(), _vocab(0)	{ }
		EqChainRemover(Vocabulary* v) : TheoryMutatingVisitor(), _vocab(v) { }

		Formula* visit(EqChainForm*);

};

Formula* EqChainRemover::visit(EqChainForm* ef) {
	for(vector<Term*>::const_iterator it = ef->subterms().begin(); it != ef->subterms().end(); ++it) 
		(*it)->accept(this);
	vector<Formula*> vf;
	unsigned int n = 0;
	for(vector<CompType>::const_iterator it = ef->comps().begin(); it != ef->comps().end(); ++it, ++n) {
		Predicate* p = 0;
		switch(*it) {
			case CT_EQ : case CT_NEQ : p = Vocabulary::std()->pred("=/2"); break;
			case CT_LT : case CT_GEQ : p = Vocabulary::std()->pred("</2"); break;
			case CT_GT : case CT_LEQ : p = Vocabulary::std()->pred(">/2"); break;
			default: assert(false);
		}
		bool sign = (*it == CT_EQ || *it == CT_LT || *it == CT_GT); 
		vector<Sort*> vs(2); vs[0] = ef->subterms()[n]->sort(); vs[1] = ef->subterms()[n+1]->sort();
		p = p->disambiguate(vs,_vocab);
		assert(p);
		vector<Term*> vt(2); 
		if(n) vt[0] = ef->subterms()[n]->clone();
		else vt[0] = ef->subterms()[n];
		vt[1] = ef->subterms()[n+1];
		PredForm* pf = new PredForm(sign,p,vt,ef->pi());
		vf.push_back(pf);
	}
	if(vf.size() == 1) {
		if(!ef->sign()) vf[0]->swapsign();
		delete(ef);
		return vf[0];
	}
	else {
		BoolForm* bf = new BoolForm(ef->sign(),ef->conj(),vf,ef->pi());
		delete(ef);
		return bf;
	}
}

class QuantMover : public TheoryMutatingVisitor {

	public:
		QuantMover()	: TheoryMutatingVisitor() { }

		Formula* visit(QuantForm*);

};

Formula* QuantMover::visit(QuantForm* qf) {
	if(typeid(*(qf->subf())) == typeid(BoolForm)) {
		BoolForm* bf = dynamic_cast<BoolForm*>(qf->subf());
		bool u = (qf->univ() == qf->sign());
		bool c = (bf->conj() == bf->sign());
		if(u == c) {
			bool s = (qf->sign() == bf->sign());
			vector<Formula*> vf;
			for(vector<Formula*>::const_iterator it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
				QuantForm* nqf = new QuantForm(s,u,qf->quantvars(),*it,FormulaParseInfo());
				vf.push_back(nqf->clone());
				delete(nqf);
			}
			qf->subf()->recursiveDelete();
			BoolForm* nbf = new BoolForm(true,c,vf,(qf->pi()).clone());
			delete(qf);
			return nbf->accept(this);
		}
	}
	return TheoryMutatingVisitor::visit(qf);
}

/*
Formula* AggMover::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	return nf;
}
*/

class ThreeValuedTermMover : public TheoryMutatingVisitor {
	private:
		AbstractStructure*			_structure;
		bool						_poscontext;
		vector<Formula*>			_termgraphs;
		set<Variable*>				_variables;
		bool						_keepterm;
		bool						_cpsupport;
		const set<const PFSymbol*> 	_cpsymbols;
		bool isCPSymbol(const PFSymbol*) const;

	public:
		ThreeValuedTermMover(AbstractStructure* str, bool posc, bool cps=false, const set<const PFSymbol*>& cpsymbols=set<const PFSymbol*>()):
			_structure(str), _poscontext(posc), _termgraphs(0), _variables(), _cpsupport(cps), _cpsymbols(cpsymbols) { }
		Formula*	visit(PredForm* pf);
		Formula*	visit(AggForm* af);
		Term*		visit(FuncTerm* ft);
		Term*		visit(AggTerm*	at);
};

bool ThreeValuedTermMover::isCPSymbol(const PFSymbol* symbol) const {
	return (VocabularyUtils::isComparisonPredicate(symbol)) || (_cpsymbols.find(symbol) != _cpsymbols.end());
}

Term* ThreeValuedTermMover::visit(FuncTerm* functerm) {
	// Get the function and its interpretation
	Function* func = functerm->function();
	FuncInter* funcinter = _structure->inter(func);

	if(funcinter->approxtwovalued() || (_cpsupport && _keepterm && isCPSymbol(func))) {
		// The function is two-valued or we want to pass it to the constraint solver. Leave as is, just visit its children.
		for(unsigned int n = 0; n < functerm->subterms().size(); ++n) {
			Term* nterm = functerm->subterms()[n]->accept(this);
			functerm->subterm(n,nterm);
		}
		return functerm;
	}
	else {
		// The function is three-valued. Move it: create a new variable and an equation.
		Variable* var = new Variable(func->outsort());
		VarTerm* varterm = new VarTerm(var,TermParseInfo());
		vector<Term*> args;
		for(unsigned int n = 0; n < func->arity(); ++n)
			args.push_back(functerm->subterms()[n]);
		args.push_back(varterm);
		PredForm* predform = new PredForm(true,func,args,FormulaParseInfo());
		_termgraphs.push_back(predform);
		_variables.insert(var);
		delete(functerm);
		return varterm->clone();
	}
}

Term* ThreeValuedTermMover::visit(AggTerm* aggterm) {
	bool twovalued = SetUtils::approxTwoValued(aggterm->set(),_structure);
	if(twovalued || (_cpsupport && _keepterm)) return aggterm;
	else {
		Variable* var = new Variable(aggterm->sort());
		VarTerm* varterm = new VarTerm(var,TermParseInfo());
		AggTerm* newaggterm = new AggTerm(aggterm->set(),aggterm->function(),TermParseInfo());
		AggForm* aggform = new AggForm(true,varterm,CT_EQ,newaggterm,FormulaParseInfo());
		_termgraphs.push_back(aggform);
		_variables.insert(var);
		delete(aggterm);
		return varterm->clone();
	}
};

Formula* ThreeValuedTermMover::visit(PredForm* predform) {
	// Handle built-in predicates
	string symbname = predform->symbol()->name();
	if(not _cpsupport) {
		if(symbname == "=/2") {
			Term* left = predform->subterms()[0];
			Term* right = predform->subterms()[1];
			if(typeid(*left) == typeid(FuncTerm)) {
				FuncTerm* functerm = dynamic_cast<FuncTerm*>(left);
				if(!_structure->inter(functerm->function())->approxtwovalued()) { 
					Formula* newpredform = FormulaUtils::graph_functions(predform);
					return newpredform->accept(this);
				}
			}
			else if(typeid(*right) == typeid(FuncTerm)) {
				FuncTerm* functerm = dynamic_cast<FuncTerm*>(right);
				if(!_structure->inter(functerm->function())->approxtwovalued()) { 
					Formula* newpredform = FormulaUtils::graph_functions(predform);
					return newpredform->accept(this);
				}
			}
		}
		if(symbname == "=/2" || symbname == "</2" || symbname == ">/2") {
			Term* left = predform->subterms()[0];
			Term* right = predform->subterms()[1];
			//TODO: Check whether handled correctly when both sides are AggTerms!!
			CompType comp;
			if(symbname == "=/2") comp = CT_EQ;
			else if(symbname == "</2") comp = CT_LT;
			else comp = CT_GT;
			if(typeid(*left) == typeid(AggTerm)) {
				AggTerm* aggterm = dynamic_cast<AggTerm*>(left);
				comp = invertcomp(comp);
				AggForm* aggform = new AggForm(predform->sign(),right,comp,aggterm,FormulaParseInfo());
				delete(predform);
				return aggform->accept(this);
			}
			else if(typeid(*right) == typeid(AggTerm)) {
				AggTerm* aggterm = dynamic_cast<AggTerm*>(right);
				AggForm* aggform = new AggForm(predform->sign(),left,comp,aggterm,FormulaParseInfo());
				delete(predform);
				return aggform->accept(this);
			}
		}
	}

	// Visit the subterms
	_keepterm = false;
	for(unsigned int n = 0; n < predform->subterms().size(); ++n) {
		_keepterm += (typeid(*(predform->symbol())) == typeid(Function) && (n == predform->subterms().size()-1));
		_keepterm += isCPSymbol(predform->symbol());
		Term* newterm = predform->subterms()[n]->accept(this);
		predform->subterm(n,newterm);
	}

	if(_termgraphs.empty()) {	// No rewriting was needed, simply return the given atom
		return predform;
	}
	else {	// Rewriting is needed
		// Negate equations in a positive context
		if(_poscontext) {
			for(vector<Formula*>::const_iterator it = _termgraphs.begin(); it != _termgraphs.end(); ++it)
				(*it)->swapsign();
		}

		// Memory management for the original atom
		PredForm* newpredform = new PredForm(predform->sign(),predform->symbol(),predform->args(),FormulaParseInfo());
		_termgraphs.push_back(newpredform);

		// Create and return the rewriting
		BoolForm* boolform = new BoolForm(true,!_poscontext,_termgraphs,FormulaParseInfo());
		QuantForm* quantform = new QuantForm(true,_poscontext,_variables,boolform,FormulaParseInfo());
		delete(predform);
		return quantform;
	}
}

Formula* ThreeValuedTermMover::visit(AggForm* af) {
	_keepterm = true;
	af->left(af->left()->accept(this));
	if(_termgraphs.empty()) {	// No rewriting was needed, simply return the given atom
		return af;
	}
	else {
		// In a positive context, the equations are negated
		if(_poscontext) {
			for(unsigned int n = 0; n < _termgraphs.size(); ++n)
				_termgraphs[n]->swapsign();
		}
		_termgraphs.push_back(af);

		// Create and return the rewriting
		BoolForm* bf = new BoolForm(true,!_poscontext,_termgraphs,FormulaParseInfo());
		QuantForm* qf = new QuantForm(true,_poscontext,_variables,bf,(af->pi()).clone());
		return qf;
	}
}

class FuncGrapher : public TheoryMutatingVisitor {
	public:
		FuncGrapher() { }
		Formula*	visit(PredForm* pf);
		Formula*	visit(EqChainForm* ef);
};

Formula* FuncGrapher::visit(PredForm* pf) {
	PredForm* newpf = 0;
	assert(pf->symbol()->name() == "=/2");
	if(typeid(*(pf->subterms()[0])) == typeid(FuncTerm)) {
		FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterms()[0]);
		vector<Term*> vt;
		for(vector<Term*>::const_iterator it = ft->subterms().begin(); it != ft->subterms().end(); ++it) 
			vt.push_back(*it);
		vt.push_back(pf->subterms()[1]);
		newpf = new PredForm(pf->sign(),ft->function(),vt,pf->pi().clone());
		delete(ft);
		delete(pf);
	}
	else if(typeid(*(pf->subterms()[1])) == typeid(FuncTerm)) {
		FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterms()[1]);
		vector<Term*> vt;
		for(vector<Term*>::const_iterator it = ft->subterms().begin(); it != ft->subterms().end(); ++it) 
			vt.push_back(*it);
		vt.push_back(pf->subterms()[0]);
		newpf = new PredForm(pf->sign(),ft->function(),vt,pf->pi().clone());
		delete(ft);
		delete(pf);
	}
	else newpf = pf;
	return newpf;
}

Formula* FuncGrapher::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	return nf;
}


namespace FormulaUtils {

	BoolForm* trueform() {
		return new BoolForm(true,true,vector<Formula*>(0),FormulaParseInfo());
	}

	BoolForm* falseform() {
		return new BoolForm(true,false,vector<Formula*>(0),FormulaParseInfo());
	}

	Formula* remove_eqchains(Formula* f, Vocabulary* v) {
		EqChainRemover ecr(v);
		Formula* newf = f->accept(&ecr);
		return newf;
	}

	Formula* graph_functions(Formula* f) {
		FuncGrapher fg;
		Formula* newf = f->accept(&fg);
		return newf;
	}

	/** 
	 *		Non-recursively moves terms that are three-valued according to a given structure
	 *		outside a given atom. The applied rewriting depends on the given context:
	 *			- positive context:
	 *				P(t) becomes	! x : t = x => P(x).
	 *			- negative context:
	 *				P(t) becomes	? x : t = x & P(x).
	 *		The fact that the rewriting is non-recursive means that in the above example, term t 
	 *		can still contain terms that are three-valued according to the structure.
	 * 
	 *		\param pf			the given atom
	 *		\param str			the given structure
	 *		\param poscontext	true iff we are in a positive context
	 *		\param usingcp
	 *
	 *		\return The rewritten formula. If no rewriting was needed, it is the same pointer as pf.
	 *		If rewriting was needed, pf can be deleted, but not recursively.
	 *		
	 */
	Formula* moveThreeValuedTerms(Formula* f, AbstractStructure* str, bool poscontext, bool cpsupport, const set<const PFSymbol*> cpsymbols) {
		ThreeValuedTermMover tvtm(str,poscontext,cpsupport,cpsymbols);
		Formula* rewriting = f->accept(&tvtm);
		return rewriting;
	}

	bool monotone(const AggForm* af) {
		switch(af->comp()) {
			case CT_EQ: case CT_NEQ: return false;
			case CT_LT: case CT_LEQ: {
				switch(af->right()->function()) {
					case AGG_CARD : case AGG_MAX: return af->sign();
					case AGG_MIN : return !af->sign();
					case AGG_SUM : return af->sign(); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AGG_PROD : return af->sign();//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			case CT_GT: case CT_GEQ: { 
				switch(af->right()->function()) {
					case AGG_CARD : case AGG_MAX: return !af->sign();
					case AGG_MIN : return af->sign();
					case AGG_SUM : return !af->sign(); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AGG_PROD : return !af->sign();//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			default : assert(false);
		}
		return false;
	}

	bool antimonotone(const AggForm* af) {
		switch(af->comp()) {
			case CT_EQ: case CT_NEQ: return false;
			case CT_LT: case CT_LEQ: { 
				switch(af->right()->function()) {
					case AGG_CARD : case AGG_MAX: return !af->sign();
					case AGG_MIN : return af->sign();
					case AGG_SUM : return !af->sign(); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AGG_PROD : return !af->sign();//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			case CT_GT: case CT_GEQ: {
				switch(af->right()->function()) {
					case AGG_CARD : case AGG_MAX: return af->sign();
					case AGG_MIN : return !af->sign();
					case AGG_SUM : return af->sign(); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AGG_PROD : return af->sign();//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			default : assert(false);
		}
		return false;
	}
}

namespace TheoryUtils {
	void push_negations(AbstractTheory* t)		{ NegationPush np; t->accept(&np);			}
	void remove_equiv(AbstractTheory* t)		{ EquivRemover er; t->accept(&er);			}
	void flatten(AbstractTheory* t)				{ Flattener f; t->accept(&f);				}
	void remove_eqchains(AbstractTheory* t)		{ EqChainRemover er; t->accept(&er);		}
	void move_quantifiers(AbstractTheory* t)	{ QuantMover qm; t->accept(&qm);			}
}

/***************
	Visitors
***************/

void TheoryVisitor::visit(const Theory* t) {
	for(vector<Formula*>::const_iterator it = t->sentences().begin(); it != t->sentences().end(); ++it) {
		(*it)->accept(this);
	}
	for(vector<Definition*>::const_iterator it = t->definitions().begin(); it != t->definitions().end(); ++it) {
		(*it)->accept(this);
	}
	for(vector<FixpDef*>::const_iterator it = t->fixpdefs().begin(); it != t->fixpdefs().end(); ++it) {
		(*it)->accept(this);
	}
}

void TheoryVisitor::visit(const GroundTheory* ) {
	// TODO
}

void TheoryVisitor::visit(const SolverTheory* ) {
	// TODO
}

void TheoryVisitor::traverse(const Formula* f) {
	for(unsigned int n = 0; n < f->subterms().size(); ++n) {
		f->subterms()[n]->accept(this);
	}
	for(unsigned int n = 0; n < f->subformulas().size(); ++n) {
		f->subformulas()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const PredForm* pf)		{ traverse(pf); } 
void TheoryVisitor::visit(const EqChainForm* ef)	{ traverse(ef); } 
void TheoryVisitor::visit(const EquivForm* ef)	{ traverse(ef); } 
void TheoryVisitor::visit(const BoolForm* bf)		{ traverse(bf); } 
void TheoryVisitor::visit(const QuantForm* qf)	{ traverse(qf); } 
void TheoryVisitor::visit(const AggForm* af)		{ traverse(af); } 

void TheoryVisitor::visit(const Rule* r) {
	r->body()->accept(this);
}

void TheoryVisitor::visit(const Definition* d) {
	for(unsigned int n = 0; n < d->rules().size(); ++n) {
		d->rules()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const FixpDef* fd) {
	for(unsigned int n = 0; n < fd->rules().size(); ++n) {
		fd->rules()[n]->accept(this);
	}
	for(unsigned int n = 0; n < fd->defs().size(); ++n) {
		fd->defs()[n]->accept(this);
	}
}

void TheoryVisitor::traverse(const Term* t) {
	for(unsigned int n = 0; n < t->subterms().size(); ++n) {
		t->subterms()[n]->accept(this);
	}
	for(unsigned int n = 0; n < t->subsets().size(); ++n) {
		t->subsets()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const VarTerm* vt)		{ traverse(vt); } 
void TheoryVisitor::visit(const FuncTerm* ft)		{ traverse(ft); } 
void TheoryVisitor::visit(const DomainTerm* dt)	{ traverse(dt); } 
void TheoryVisitor::visit(const AggTerm* at)		{ traverse(at); } 

void TheoryVisitor::traverse(const SetExpr* s) {
	for(unsigned int n = 0; n < s->subterms().size(); ++n) {
		s->subterms()[n]->accept(this);
	}
	for(unsigned int n = 0; n < s->subformulas().size(); ++n) {
		s->subformulas()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const EnumSetExpr* es)	{ traverse(es); }
void TheoryVisitor::visit(const QuantSetExpr* qs) { traverse(qs); }

Theory* TheoryMutatingVisitor::visit(Theory* t) {
	for(vector<Formula*>::iterator it = t->sentences().begin(); it != t->sentences().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for(vector<Definition*>::iterator it = t->definitions().begin(); it != t->definitions().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for(vector<FixpDef*>::iterator it = t->fixpdefs().begin(); it != t->fixpdefs().end(); ++it) {
		*it = (*it)->accept(this);
	}
	return t;
}

GroundTheory* TheoryMutatingVisitor::visit(GroundTheory* t) {
	// TODO
	return t;
}

SolverTheory* TheoryMutatingVisitor::visit(SolverTheory* t) {
	// TODO
	return t;
}

Formula* TheoryMutatingVisitor::traverse(Formula* f) {
	for(unsigned int n = 0; n < f->subterms().size(); ++n) {
		f->subterm(n,f->subterms()[n]->accept(this));
	}
	for(unsigned int n = 0; n < f->subformulas().size(); ++n) {
		f->subformula(n,f->subformulas()[n]->accept(this));
	}
	return f;
}

Formula* TheoryMutatingVisitor::visit(PredForm* pf)		{ return traverse(pf); } 
Formula* TheoryMutatingVisitor::visit(EqChainForm* ef)	{ return traverse(ef); } 
Formula* TheoryMutatingVisitor::visit(EquivForm* ef)	{ return traverse(ef); } 
Formula* TheoryMutatingVisitor::visit(BoolForm* bf)		{ return traverse(bf); } 
Formula* TheoryMutatingVisitor::visit(QuantForm* qf)	{ return traverse(qf); } 
Formula* TheoryMutatingVisitor::visit(AggForm* af)		{ return traverse(af); } 

Rule* TheoryMutatingVisitor::visit(Rule* r) {
	r->body(r->body()->accept(this));
	return r;
}

Definition* TheoryMutatingVisitor::visit(Definition* d) {
	for(unsigned int n = 0; n < d->rules().size(); ++n) {
		d->rule(n,d->rules()[n]->accept(this));
	}
	return d;
}

FixpDef* TheoryMutatingVisitor::visit(FixpDef* fd) {
	for(unsigned int n = 0; n < fd->rules().size(); ++n) {
		fd->rule(n,fd->rules()[n]->accept(this));
	}
	for(unsigned int n = 0; n < fd->defs().size(); ++n) {
		fd->def(n,fd->defs()[n]->accept(this));
	}
	return fd;
}

Term* TheoryMutatingVisitor::traverse(Term* t) {
	for(unsigned int n = 0; n < t->subterms().size(); ++n) {
		t->subterm(n,t->subterms()[n]->accept(this));
	}
	for(unsigned int n = 0; n < t->subsets().size(); ++n) {
		t->subset(n,t->subsets()[n]->accept(this));
	}
	return t;
}

Term* TheoryMutatingVisitor::visit(VarTerm* vt)		{ return traverse(vt); } 
Term* TheoryMutatingVisitor::visit(FuncTerm* ft)	{ return traverse(ft); } 
Term* TheoryMutatingVisitor::visit(DomainTerm* dt)	{ return traverse(dt); } 
Term* TheoryMutatingVisitor::visit(AggTerm* at)		{ return traverse(at); } 

SetExpr* TheoryMutatingVisitor::traverse(SetExpr* s) {
	for(unsigned int n = 0; n < s->subterms().size(); ++n) {
		s->subterm(n,s->subterms()[n]->accept(this));
	}
	for(unsigned int n = 0; n < s->subformulas().size(); ++n) {
		s->subformula(n,s->subformulas()[n]->accept(this));
	}
	return s;
}

SetExpr* TheoryMutatingVisitor::visit(EnumSetExpr* es)	{ return traverse(es); }
SetExpr* TheoryMutatingVisitor::visit(QuantSetExpr* qs) { return traverse(qs); }

