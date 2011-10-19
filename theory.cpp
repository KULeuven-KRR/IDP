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
#include "error.hpp"
#include "fobdd.hpp"
using namespace std;

/**********************
	TheoryComponent
**********************/

string TheoryComponent::toString(unsigned int spaces) const {
	stringstream sstr;
	put(sstr,spaces);
	return sstr.str();
}

/**************
	Formula
**************/

void Formula::setFreeVars() {
	_freevars.clear();
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		_freevars.insert((*it)->freeVars().begin(),(*it)->freeVars().end());
	}
	for(auto it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		_freevars.insert((*it)->freeVars().begin(),(*it)->freeVars().end());
	}
	for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		_freevars.erase(*it);
	}
}

void Formula::recursiveDelete() {
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(auto it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		delete(*it);
	}
	delete(this);
}

bool Formula::contains(const Variable* v) const {
	for(auto it = _freevars.begin(); it != _freevars.end(); ++it) {
		if(*it == v) return true;
	}
	for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		if(*it == v) return true;
	}
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		if((*it)->contains(v)) return true;
	}
	for(auto it = _subformulas.begin(); it != _subformulas.end(); ++it) {
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
	for(auto it = subterms().begin(); it != subterms().end(); ++it) 
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

ostream& PredForm::put(ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	if(isNeg(sign())){
		output << '~';
	}
	_symbol->put(output,longnames);
	if(typeid(*_symbol) == typeid(Predicate)) {
		if(not subterms().empty()) {
			output << '(';
			for(size_t n = 0; n < subterms().size(); ++n) {
				subterms()[n]->put(output,longnames);
				if(n < subterms().size()-1) { output << ','; }
			}
			output << ')';
		}
	}
	else {
		assert(typeid(*_symbol) == typeid(Function));
		if(subterms().size() > 1) {
			output << '(';
			for(size_t n = 0; n < subterms().size()-1; ++n) { 
				subterms()[n]->put(output,longnames);
				if(n+1 < subterms().size()-1) { output << ','; }
			}
			output << ')';
		}
		output << " = ";
		subterms().back()->put(output,longnames);
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
	for(auto it = subterms().begin(); it != subterms().end(); ++it) { 
		nt.push_back((*it)->clone(mvv));
	}
	EqChainForm* ef = new EqChainForm(sign(),_conj,nt,_comps,pi().clone(mvv));
	return ef;
}

void EqChainForm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Formula* EqChainForm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& EqChainForm::put(ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	if(isNeg(sign())) output << '~';
	output << '(';
	subterms()[0]->put(output,longnames);
	for(unsigned int n = 0; n < _comps.size(); ++n) {
		output << ' ' << comps()[n] << ' ';
		subterms()[n+1]->put(output,longnames);
		if(not _conj && n+1 < _comps.size()){
			output << " | ";
			subterms()[n+1]->put(output,longnames);
		}
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

ostream& EquivForm::put(ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	output << '('; left()->put(output,longnames);
	output << " <=> "; right()->put(output,longnames);
	output << ')';
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
	for(auto it = subformulas().begin(); it != subformulas().end(); ++it) 
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

ostream& BoolForm::put(ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	if(subformulas().empty()) {
		if(isConjWithSign()) output << "true";
		else output << "false";
	}
	else {
		if(isNeg(sign())) output << '~';
		output << '(';
		for(size_t n = 0; n < subformulas().size(); ++n) {
			subformulas()[n]->put(output,longnames);
			if(n < subformulas().size()-1) {
				output << (_conj ? " & " : " | ");
			}
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
	for(auto it = quantVars().begin(); it != quantVars().end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),pi());
		nv.insert(v);
		nmvv[*it] = v;
	}
	Formula* nf = subformula()->clone(nmvv);
	QuantForm* qf = new QuantForm(sign(),quant(),nv,nf,pi().clone(mvv));
	return qf;
}

void QuantForm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Formula* QuantForm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& QuantForm::put(ostream& output, bool longnames,  unsigned int spaces) const {
	printTabs(output,spaces);
	if(isNeg(sign())) output << '~';
	output << '(';
	output << (isUniv()? '!' : '?');
	for(auto it = quantVars().begin(); it != quantVars().end(); ++it) {
		output << ' '; (*it)->put(output,longnames);
	}
	output << " : ";
	subformula()->put(output,longnames);
	output << ')';
	return output;
}

/**************
	AggForm
**************/

AggForm::AggForm(SIGN sign, Term* l, CompType c, AggTerm* r, const FormulaParseInfo& pi) :
	Formula(sign,pi), _comp(c), _aggterm(r) { 
	addSubterm(l); 
	addSubterm(r); 
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

ostream& AggForm::put(ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	if(isNeg(sign())) output << '~';
	output << '(';
	left()->put(output,longnames);
	output << ' ' << _comp << ' ';
	right()->put(output,longnames);
	output << ')';
	return output;
}

/***********
	Rule
***********/

Rule* Rule::clone() const {
	map<Variable*,Variable*> mvv;
	set<Variable*> newqv;
	for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		newqv.insert(v);
	}
	return new Rule(newqv,_head->clone(mvv),_body->clone(mvv),_pi);
}

void Rule::recursiveDelete() {
	_head->recursiveDelete();
	_body->recursiveDelete();
	for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) delete(*it);
	delete(this);
}

void Rule::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Rule* Rule::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}


ostream& Rule::put(ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	if(not _quantvars.empty()) {
		output << "!";
		for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) {
			output << ' '; (*it)->put(output,longnames);
		}
		output << " : ";
	}
	_head->put(output,longnames);
	output << " <- ";
	_body->put(output,longnames);
	output << '.';
	return output;
}

string Rule::toString(unsigned int spaces) const {
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
	for(auto it = _rules.begin(); it != _rules.end(); ++it)
		newdef->add((*it)->clone());
	return newdef;
}

void Definition::recursiveDelete() {
	for(size_t n = 0; n < _rules.size(); ++n) _rules[n]->recursiveDelete();
	delete(this);
}

void Definition::add(Rule* r) {
	_rules.push_back(r);
	_defsyms.insert(r->head()->symbol());
}

void Definition::rule(unsigned int n, Rule* r) {
	_rules[n] = r;
	_defsyms.clear();
	for(auto it = _rules.begin(); it != _rules.end(); ++it) 
		_defsyms.insert((*it)->head()->symbol());
}

void Definition::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Definition* Definition::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& Definition::put(ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	output << "{ ";
	if(not _rules.empty()) {
		_rules[0]->put(output,longnames);
		for(size_t n = 1; n < _rules.size(); ++n) {
			output << '\n';
			_rules[n]->put(output,longnames,spaces+2);
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
	for(auto it = _defs.begin(); it != _defs.end(); ++it) 
		newfd->add((*it)->clone());
	for(auto it = _rules.begin(); it != _rules.end(); ++it) 
		newfd->add((*it)->clone());
	return newfd;
}

void FixpDef::recursiveDelete() {
	for(auto it = _defs.begin(); it != _defs.end(); ++it) 
		delete(*it);
	for(auto it = _rules.begin(); it != _rules.end(); ++it) 
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
	for(auto it = _rules.begin(); it != _rules.end(); ++it) 
		_defsyms.insert((*it)->head()->symbol());
}

void FixpDef::accept(TheoryVisitor* v) const {
	v->visit(this);
}

FixpDef* FixpDef::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& FixpDef::put(ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	output << (_lfp ? "LFD [  " : "GFD [  ");
	if(not _rules.empty()) {
		_rules[0]->put(output,longnames);
		for(size_t n = 1; n < _rules.size(); ++n) {
			output << '\n';
			_rules[n]->put(output,longnames,spaces+2);
		}
	}
	for(auto it = _defs.begin(); it != _defs.end(); ++it) {
		output << '\n';
		(*it)->put(output,longnames,spaces+2);
	}
	output << " ]";
	return output;
}

/***************
	Theories
***************/

Theory* Theory::clone() const {
	Theory* newtheory = new Theory(_name,_vocabulary,ParseInfo());
	for(auto it = _sentences.begin(); it != _sentences.end(); ++it)
		newtheory->add((*it)->clone());
	for(auto it = _definitions.begin(); it != _definitions.end(); ++it)
		newtheory->add((*it)->clone());
	for(auto it = _fixpdefs.begin(); it != _fixpdefs.end(); ++it)
		newtheory->add((*it)->clone());
	return newtheory;
}

void Theory::addTheory(AbstractTheory* ) {
	// TODO
}

void Theory::recursiveDelete() {
	for(auto it = _sentences.begin(); it != _sentences.end(); ++it)
		(*it)->recursiveDelete();
	for(auto it = _definitions.begin(); it != _definitions.end(); ++it)
		(*it)->recursiveDelete();
	for(auto it = _fixpdefs.begin(); it != _fixpdefs.end(); ++it)
		(*it)->recursiveDelete();
	delete(this);
}

set<TheoryComponent*> Theory::components() const {
	set<TheoryComponent*> stc;
	for(auto it = _sentences.begin(); it != _sentences.end(); ++it) stc.insert(*it);
	for(auto it = _definitions.begin(); it != _definitions.end(); ++it) stc.insert(*it);
	for(auto it = _fixpdefs.begin(); it != _fixpdefs.end(); ++it) stc.insert(*it);
	return stc;
}

void Theory::remove(Definition* d) {
	auto it = _definitions.begin();
	for(; it != _definitions.end(); ++it) {
		if(*it == d) break;
	}
	if(it != _definitions.end()) _definitions.erase(it);
}

void Theory::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Theory* Theory::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

std::ostream& Theory::put(std::ostream& output, bool longnames, unsigned int spaces) const {
	printTabs(output,spaces);
	output << "theory " << name();
	if(_vocabulary) {
		output << " : " << vocabulary()->name();
	}
	output << " {\n";
	for(auto it = _sentences.begin(); it != _sentences.end(); ++it)
		(*it)->put(output,longnames,spaces+2);
	for(auto it = _definitions.begin(); it != _definitions.end(); ++it)
		(*it)->put(output,longnames,spaces+2);
	for(auto it = _fixpdefs.begin(); it != _fixpdefs.end(); ++it)
		(*it)->put(output,longnames,spaces+2);
	output << "}\n";
	return output;
}


/********************
	Formula utils
********************/

class NegationPush : public TheoryMutatingVisitor {
	public:
		NegationPush()	: TheoryMutatingVisitor() { }

		Formula*	visit(PredForm*);
		Formula*	visit(EqChainForm*);
		Formula* 	visit(EquivForm*);
		Formula* 	visit(BoolForm*);
		Formula* 	visit(QuantForm*);

		Formula*	traverse(Formula*);
		Term*		traverse(Term*);
};

Formula* NegationPush::visit(PredForm* pf) {
	if(isPos(pf->sign())){
		return traverse(pf);
	}
	if(safetypeid<Predicate>(*(pf->symbol()))) {
		Predicate* p = dynamic_cast<Predicate*>(pf->symbol());
		if(p->type() != ST_NONE) {
			Predicate* newsymbol = NULL;
			switch(p->type()) {
				case ST_CT: newsymbol = pf->symbol()->derivedSymbol(ST_PF); break;
				case ST_CF: newsymbol = pf->symbol()->derivedSymbol(ST_PT); break;
				case ST_PT: newsymbol = pf->symbol()->derivedSymbol(ST_CF); break;
				case ST_PF: newsymbol = pf->symbol()->derivedSymbol(ST_CT); break;
				case ST_NONE: assert(false); break;
			}
			PredForm* newpf = new PredForm(SIGN::POS,newsymbol,pf->subterms(),pf->pi().clone());
			delete(pf);
			pf = newpf;
		}
	}
	return traverse(pf);
}

Formula* NegationPush::traverse(Formula* f) {
	for(auto it = f->subformulas().begin(); it != f->subformulas().end(); ++it) {
		(*it)->accept(this);
	}
	for(auto it = f->subterms().begin(); it != f->subterms().end(); ++it) {
		(*it)->accept(this);
	}
	return f;
}

Term* NegationPush::traverse(Term* t) {
	for(auto it = t->subterms().begin(); it != t->subterms().end(); ++it) {
		(*it)->accept(this);
	}
	for(auto it = t->subsets().begin(); it != t->subsets().end(); ++it) {
		(*it)->accept(this);
	}
	return t;
}

Formula* NegationPush::visit(EqChainForm* f) {
	if(isNeg(f->sign())) {
		f->negate();
		f->conj(!f->conj());
		for(size_t n = 0; n < f->comps().size(); ++n) {
			f->comp(n,negateComp(f->comps()[n]));
		}
	}
	return traverse(f);
}

Formula* NegationPush::visit(EquivForm* f) {
	if(isNeg(f->sign())) {
		f->negate();
		f->right()->negate();
	}
	return traverse(f);
}

Formula* NegationPush::visit(BoolForm* f) {
	if(isNeg(f->sign())) {
		f->negate();
		for(auto it = f->subformulas().begin(); it != f->subformulas().end(); ++it) 
			(*it)->negate();
		f->conj(!f->conj());
	}
	return traverse(f);
}

Formula* NegationPush::visit(QuantForm* f) {
	if(isNeg(f->sign())) {
		f->negate();
		f->subformula()->negate();
		f->quant(not f->quant());
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
	vf1[0]->negate(); vf2[1]->negate();
	BoolForm* bf1 = new BoolForm(SIGN::POS,false,vf1,ef->pi());
	BoolForm* bf2 = new BoolForm(SIGN::POS,false,vf2,ef->pi());
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
	for(auto it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
		if(typeid(*(*it)) == typeid(BoolForm)) {
			BoolForm* sbf = dynamic_cast<BoolForm*>(*it);
			if((bf->conj() == sbf->conj()) && isPos(sbf->sign())) {
				for(auto jt = sbf->subformulas().begin(); jt != sbf->subformulas().end(); ++jt){
					newsubf.push_back(*jt);
				}
				delete(sbf);
			}
			else { newsubf.push_back(*it); }
		}
		else { newsubf.push_back(*it); }
	}
	bf->subformulas(newsubf);
	return bf;
}

Formula* Flattener::visit(QuantForm* qf) {
	traverse(qf);	
	if(typeid(*(qf->subformula())) == typeid(QuantForm)) {
		QuantForm* sqf = dynamic_cast<QuantForm*>(qf->subformula());
		if((qf->quant() == sqf->quant()) && isPos(sqf->sign())) {
			qf->subformula(sqf->subformula());
			for(auto it = sqf->quantVars().begin(); it != sqf->quantVars().end(); ++it){
				qf->add(*it);
			}
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
	for(auto it = ef->subterms().begin(); it != ef->subterms().end(); ++it) { 
		(*it)->accept(this);
	}
	vector<Formula*> vf;
	size_t n = 0;
	for(auto it = ef->comps().begin(); it != ef->comps().end(); ++it, ++n) {
		Predicate* p = 0;
		switch(*it) {
			case CompType::EQ : case CompType::NEQ : p = Vocabulary::std()->pred("=/2"); break;
			case CompType::LT : case CompType::GEQ : p = Vocabulary::std()->pred("</2"); break;
			case CompType::GT : case CompType::LEQ : p = Vocabulary::std()->pred(">/2"); break;
			default: assert(false);
		}
		SIGN sign = (*it == CompType::EQ || *it == CompType::LT || *it == CompType::GT)?SIGN::POS:SIGN::NEG;
		vector<Sort*> vs(2); vs[0] = ef->subterms()[n]->sort(); vs[1] = ef->subterms()[n+1]->sort();
		p = p->disambiguate(vs,_vocab);
		assert(p);
		vector<Term*> vt(2); 
		if(n) { vt[0] = ef->subterms()[n]->clone(); }
		else { vt[0] = ef->subterms()[n]; }
		vt[1] = ef->subterms()[n+1];
		PredForm* pf = new PredForm(sign,p,vt,ef->pi());
		vf.push_back(pf);
	}
	if(vf.size() == 1) {
		if(isNeg(ef->sign())) vf[0]->negate();
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
	if(typeid(*(qf->subformula())) == typeid(BoolForm)) {
		BoolForm* bf = dynamic_cast<BoolForm*>(qf->subformula());
		QUANT u = qf->isUnivWithSign()?QUANT::UNIV:QUANT::EXIST;
		bool c = bf->isConjWithSign();
		if((u==QUANT::UNIV && bf->isConjWithSign()) || (u==QUANT::EXIST && not bf->isConjWithSign())) {
			SIGN s = (qf->sign() == bf->sign())?SIGN::POS:SIGN::NEG;
			vector<Formula*> vf;
			for(auto it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
				QuantForm* nqf = new QuantForm(s,u,qf->quantVars(),*it,FormulaParseInfo());
				vf.push_back(nqf->clone());
				delete(nqf);
			}
			qf->subformula()->recursiveDelete();
			BoolForm* nbf = new BoolForm(SIGN::POS,c,vf,(qf->pi()).clone());
			delete(qf);
			return nbf->accept(this);
		}
	}
	return TheoryMutatingVisitor::visit(qf);
}


class Completer : public TheoryMutatingVisitor {
	private:
		vector<Formula*>					_result;
		map<PFSymbol*,vector<Variable*>	>	_headvars;
		map<PFSymbol*,vector<Formula*> >	_interres;

	public:
		Completer() { }

		Theory*			visit(Theory*);
		Definition*		visit(Definition*);
		Rule*			visit(Rule*);
};

Theory* Completer::visit(Theory* theory) {
	for(auto it = theory->definitions().begin(); it != theory->definitions().end(); ++it) {
		(*it)->accept(this);
		for(auto jt = _result.begin(); jt != _result.end(); ++jt) 
			theory->add(*jt);
		(*it)->recursiveDelete();
	}
	theory->definitions().clear();
	return theory;
}

Definition* Completer::visit(Definition* def) {
	_headvars.clear();
	_interres.clear();
	_result.clear();
	for(auto it = def->defsymbols().begin(); it != def->defsymbols().end(); ++it) {
		vector<Variable*> vv;
		for(auto jt = (*it)->sorts().begin(); jt != (*it)->sorts().end(); ++jt) {
			vv.push_back(new Variable(*jt));
		}
		_headvars[*it] = vv;
	}
	for(auto it = def->rules().begin(); it != def->rules().end(); ++it) {
		(*it)->accept(this);
	}

	for(auto it = _interres.begin(); it != _interres.end(); ++it) {
		assert(!it->second.empty());
		Formula* b = it->second[0];
		if(it->second.size() > 1) b = new BoolForm(SIGN::POS,false,it->second,FormulaParseInfo());
		PredForm* h = new PredForm(SIGN::POS,it->first,TermUtils::makeNewVarTerms(_headvars[it->first]),FormulaParseInfo());
		EquivForm* ev = new EquivForm(SIGN::POS,h,b,FormulaParseInfo());
		if(it->first->sorts().empty()) {
			_result.push_back(ev);
		}
		else {
			set<Variable*> qv(_headvars[it->first].begin(),_headvars[it->first].end());
			QuantForm* qf = new QuantForm(SIGN::POS,QUANT::UNIV,qv,ev,FormulaParseInfo());
			_result.push_back(qf);
		}
	}

	return def;
}

Rule* Completer::visit(Rule* rule) {
	vector<Formula*> vf;
	vector<Variable*> vv = _headvars[rule->head()->symbol()];
	set<Variable*> freevars = rule->quantVars();
	map<Variable*,Variable*> mvv;

	for(size_t n = 0; n < rule->head()->subterms().size(); ++n) {
		Term* t = rule->head()->subterms()[n];
		if(typeid(*t) != typeid(VarTerm)) {
			VarTerm* bvt = new VarTerm(vv[n],TermParseInfo());
			vector<Term*> args; args.push_back(bvt); args.push_back(t->clone());
			Predicate* p = Vocabulary::std()->pred("=/2")->resolve(vector<Sort*>(2,vv[n]->sort()));
			PredForm* pf = new PredForm(SIGN::POS,p,args,FormulaParseInfo());
			vf.push_back(pf);
		}
		else {
			Variable* v = *(t->freeVars().begin());
			if(mvv.find(v) == mvv.end()) {
				mvv[v] = vv[n];
				freevars.erase(v);
			}
			else {
				VarTerm* bvt1 = new VarTerm(vv[n],TermParseInfo());
				VarTerm* bvt2 = new VarTerm(mvv[v],TermParseInfo());
				vector<Term*> args; args.push_back(bvt1); args.push_back(bvt2);
				Predicate* p = Vocabulary::std()->pred("=/2")->resolve(vector<Sort*>(2,v->sort()));
				PredForm* pf = new PredForm(SIGN::POS,p,args,FormulaParseInfo());
				vf.push_back(pf);
			}
		}
	}
	Formula* b = rule->body();
	if(!vf.empty()) {
		vf.push_back(b);
		b = new BoolForm(SIGN::POS,true,vf,FormulaParseInfo());
	}
	if(!freevars.empty()) {
		b = new QuantForm(SIGN::POS,QUANT::EXIST,freevars,b,FormulaParseInfo());
	}
	b = b->clone(mvv);
	_interres[rule->head()->symbol()].push_back(b);

	return rule;
}


/************************************
	Move terms outside predicates
************************************/

/**
 * Base class for term moving visitors
 * If not subclassed, it removes all nested terms
 */
class TermMover : public TheoryMutatingVisitor {
	protected:
		Vocabulary*			_vocabulary;	//!< Used to do type derivation during rewrites
		Context			_context;		//!< Keeps track of the current context where terms are moved
		bool				_movecontext;	//!< true iff terms in the current context may be moved
		vector<Formula*>	_equalities;	//!< used to temporarily store the equalities generated when moving terms
		set<Variable*>		_variables;		//!< used to temporarily store the freshly introduced variables

	public:
		
		TermMover(Context context = Context::POSITIVE, Vocabulary* v = 0) :
			_vocabulary(v), _context(context), _movecontext(false) { }

		void contextProblem(Term* t) {
			if(t->pi().original()) {
				if(TermUtils::isPartial(t)) { Warning::ambigpartialterm(t->pi().original()->toString(),t->pi()); }
			}
		}

		/** 
		 * Returns true is the term should be moved 
		 * (this is the most important method to overwrite in subclasses)
		 */
		virtual bool shouldMove(Term* t) {
			assert(t->type() != TT_VAR);
			return _movecontext;
		}

		/**
		 * Create a variable and an equation for the given term
		 */
		VarTerm* move(Term* term) {
			if(_context == Context::BOTH) contextProblem(term);

			Variable* introduced_var = new Variable(term->sort());

			VarTerm* introduced_subst_term = new VarTerm(introduced_var,TermParseInfo(term->pi()));
			VarTerm* introduced_eq_term = new VarTerm(introduced_var,TermParseInfo(term->pi()));
			vector<Term*> equality_args(2);
			equality_args[0] = introduced_eq_term;
			equality_args[1] = term;
			Predicate* equalpred = VocabularyUtils::equal(term->sort());
			PredForm* equalatom = new PredForm(SIGN::POS,equalpred,equality_args,FormulaParseInfo());

			_equalities.push_back(equalatom);
			_variables.insert(introduced_var);
			return introduced_subst_term;
		}

		/**
		 * Rewrite the given formula with the current equalities
		 */
		Formula* rewrite(Formula* formula) {
			const FormulaParseInfo& origpi = formula->pi();
			bool univ_and_disj = false;
			if(_context == Context::POSITIVE) {
				univ_and_disj = true;
				for(auto it = _equalities.begin(); it != _equalities.end(); ++it) { 
					(*it)->negate();
				}
			}
			if(not _equalities.empty()) {
				_equalities.push_back(formula);
				if(!_variables.empty()){
					formula = new BoolForm(SIGN::POS,!univ_and_disj,_equalities,origpi);
				} else {
					formula = new BoolForm(SIGN::POS,!univ_and_disj,_equalities,FormulaParseInfo());
				}
				_equalities.clear();
			}
			if(not _variables.empty()) {
				formula = new QuantForm(SIGN::POS,univ_and_disj?QUANT::UNIV:QUANT::EXIST,_variables,formula,origpi);
				_variables.clear();
			}
			return formula;
		}
		
		/**
		 *	Visit all parts of the theory, assuming positive context for sentences
		 */
		Theory* visit(Theory* theory) {
			for(unsigned int n = 0; n < theory->sentences().size(); ++n) {
				_context = Context::POSITIVE;
				_movecontext = false;
				theory->sentence(n,theory->sentences()[n]->accept(this));
			}
			for(auto it = theory->definitions().begin(); 
				it != theory->definitions().end(); ++it) {
				(*it)->accept(this);
			}
			for(auto it = theory->fixpdefs().begin(); 
				it != theory->fixpdefs().end(); ++it) {
				(*it)->accept(this);
			}
			return theory;
		}

		/**
		 *	Visit a rule, assuming negative context for body.
		 *	May move terms from rule head.
		 */
		virtual Rule* visit(Rule* rule) {
			// Visit head
			for(size_t termposition = 0; termposition < rule->head()->subterms().size(); ++termposition) {
				Term* term = rule->head()->subterms()[termposition];
				if(shouldMove(term)) {
					VarTerm* new_head_term = move(term);
					rule->head()->subterm(termposition,new_head_term);
				}
			}
			if(not _equalities.empty()) {
				for(auto it = _variables.begin(); it != _variables.end(); ++it) {
					rule->addvar(*it);
				}
				
				_equalities.push_back(rule->body());
				rule->body(new BoolForm(SIGN::POS,true,_equalities,FormulaParseInfo()));

				_equalities.clear();
				_variables.clear();
			}

			// Visit body
			_context = Context::NEGATIVE;
			_movecontext = false;
			rule->body(rule->body()->accept(this));
			return rule;
		}

		virtual Formula* traverse(Formula* f) {
			Context savecontext = _context;
			bool savemovecontext = _movecontext;
			if(isNeg(f->sign())){
				_context = not _context;
			}
			for(unsigned int n = 0; n < f->subterms().size(); ++n) {
				f->subterm(n,f->subterms()[n]->accept(this));
			}
			for(size_t n = 0; n < f->subformulas().size(); ++n) {
				f->subformula(n,f->subformulas()[n]->accept(this));
			}
			_context = savecontext;
			_movecontext = savemovecontext;
			return f;
		}

		virtual Formula* traverse(PredForm* f) {
			//TODO Very ugly static cast!! XXX This need to be done differently!! FIXME
			return traverse(static_cast<Formula*>(f));
		}

		virtual Formula* visit(EquivForm* ef) {
			Context savecontext = _context;
			_context = Context::BOTH;
			Formula* f = traverse(ef);
			_context = savecontext;
			return f;
		}

		virtual Formula* visit(AggForm* af) {
			traverse(af);
			Formula* rewrittenformula = rewrite(af);
			if(rewrittenformula == af) { return af; } 
			else { return rewrittenformula->accept(this); }
		}

		virtual Formula* visit(EqChainForm* ef) {
			if(ef->comps().size() == 1) {	// Rewrite to an normal atom
				SIGN atomsign = ef->sign();
				Sort* atomsort = SortUtils::resolve(ef->subterms()[0]->sort(),ef->subterms()[1]->sort(),_vocabulary);
				Predicate* comppred;
				switch(ef->comps()[0]) {
					case CompType::EQ:
						comppred = VocabularyUtils::equal(atomsort);
						break;
					case CompType::LT:
						comppred = VocabularyUtils::lessThan(atomsort);
						break;
					case CompType::GT:
						comppred = VocabularyUtils::greaterThan(atomsort);
						break;
					case CompType::NEQ:
						comppred = VocabularyUtils::equal(atomsort);
						atomsign = not atomsign;
						break;
					case CompType::LEQ:
						comppred = VocabularyUtils::greaterThan(atomsort);
						atomsign = not atomsign;
						break;
					case CompType::GEQ:
						comppred = VocabularyUtils::lessThan(atomsort);
						atomsign = not atomsign;
						break;
				}
				vector<Term*> atomargs(2);
				atomargs[0] = ef->subterms()[0];
				atomargs[1] = ef->subterms()[1];
				PredForm* atom = new PredForm(atomsign,comppred,atomargs,ef->pi());
				return atom->accept(this);
			}
			else {	// Simple recursive call
				bool savemovecontext = _movecontext; _movecontext = true;
				Formula* rewrittenformula = TheoryMutatingVisitor::traverse(ef);
				_movecontext = savemovecontext;
				if(rewrittenformula == ef) { return ef; } 
				else { return rewrittenformula->accept(this); }
			}
		}

		virtual Formula* visit(PredForm* predform) {
			bool savemovecontext = _movecontext;
			// Special treatment for (in)equalities: possibly only one side needs to be moved
			bool moveonlyleft = false;
			bool moveonlyright = false;
			string symbolname = predform->symbol()->name();
			if(symbolname == "=/2" || symbolname == "</2" || symbolname == ">/2") {
				Term* leftterm = predform->subterms()[0];
				Term* rightterm = predform->subterms()[1];
				if(leftterm->type() == TT_AGG) {
					moveonlyright = true;
				} else if(rightterm->type() == TT_AGG) {
					moveonlyleft = true;
				} else if(symbolname == "=/2") {
					moveonlyright = (leftterm->type() != TT_VAR) && (rightterm->type() != TT_VAR);
				} else {
					_movecontext = true;
				}
			} else {
				_movecontext = true;
			}
			// Traverse the atom
			if(moveonlyleft) {
				predform->subterm(1,predform->subterms()[1]->accept(this));
				_movecontext = true;
				predform->subterm(0,predform->subterms()[0]->accept(this));
			} else if(moveonlyright) {
				predform->subterm(0,predform->subterms()[0]->accept(this));
				_movecontext = true;
				predform->subterm(1,predform->subterms()[1]->accept(this));
			} else {
				traverse(predform);
			}
			_movecontext = savemovecontext;

			// Change the atom
			Formula* rewrittenformula = rewrite(predform);
			if(rewrittenformula == predform) { return predform; } 
			else { return rewrittenformula->accept(this); }
		}

		virtual Term* traverse(Term* term) {
			Context savecontext = _context;
			bool savemovecontext = _movecontext;
			for(size_t n = 0; n < term->subterms().size(); ++n) {
				term->subterm(n,term->subterms()[n]->accept(this));
			}
			for(size_t n = 0; n < term->subsets().size(); ++n) {
				term->subset(n,term->subsets()[n]->accept(this));
			}
			_context = savecontext;
			_movecontext = savemovecontext;
			return term;
		}

		VarTerm* visit(VarTerm* t) { 
			return t; 
		}

		virtual Term* visit(DomainTerm* t) {
			if(_movecontext && shouldMove(t)) {
				return move(t);
			} else {
				return t;
			}
		}

		virtual Term* visit(AggTerm* t) {
			if(_movecontext && shouldMove(t)) {
				return move(t);
			} else {
				return traverse(t);
			}
		}

		virtual Term* visit(FuncTerm* ft) {
			bool savemovecontext = _movecontext;
			_movecontext = true;
			Term* result = traverse(ft);
			_movecontext = savemovecontext;
			if(_movecontext && shouldMove(result)) {
				return move(result);
			} else { 
				return result;
			}
		}

		virtual SetExpr* visit(EnumSetExpr* s) {
			vector<Formula*> saveequalities = _equalities; _equalities.clear();
			set<Variable*> savevars = _variables; _variables.clear();
			bool savemovecontext = _movecontext; _movecontext = true;
			Context savecontext = _context;

			for(size_t n = 0; n < s->subterms().size(); ++n) {
				s->subterm(n,s->subterms()[n]->accept(this));
				if(not _equalities.empty()) {
					_equalities.push_back(s->subformulas()[n]);
					s->subformula(n,new BoolForm(SIGN::POS,true,_equalities,FormulaParseInfo()));
					savevars.insert(_variables.begin(),_variables.end());
					_equalities.clear();
					_variables.clear();
				}
			}

			_context = Context::POSITIVE;
			_movecontext = false;
			for(size_t n = 0; n < s->subformulas().size(); ++n) {
				s->subformula(n,s->subformulas()[n]->accept(this));
			}
			_context = savecontext;
			_movecontext = savemovecontext;
			_variables = savevars;
			_equalities = saveequalities;
			return s;
		}

		virtual SetExpr* visit(QuantSetExpr* s) {
			vector<Formula*> saveequalities = _equalities; _equalities.clear();
			set<Variable*> savevars = _variables; _variables.clear();
			bool savemovecontext = _movecontext; _movecontext = true;
			Context savecontext = _context; _context = Context::POSITIVE;

			s->subterm(0,s->subterms()[0]->accept(this));
			if(not _equalities.empty()) {
				_equalities.push_back(s->subformulas()[0]);
				BoolForm* bf = new BoolForm(SIGN::POS,true,_equalities,FormulaParseInfo());
				s->subformula(0,bf);
				for(auto it = _variables.begin(); it != _variables.end(); ++it) {
					s->addQuantVar(*it);
				}
				_equalities.clear();
				_variables.clear();
			}

			_movecontext = false;
			_context = Context::POSITIVE;
			s->subformula(0,s->subformulas()[0]->accept(this));

			_variables = savevars;
			_equalities = saveequalities;
			_context = savecontext;
			_movecontext = savemovecontext;
			return s;
		}
};

/******************************
	Three-valued term mover
******************************/

class ThreeValuedTermMover : public TermMover {
	private:
		AbstractStructure*			_structure;
		bool						_cpsupport;
		const set<const PFSymbol*> 	_cpsymbols;

	public:
		ThreeValuedTermMover(AbstractStructure* str, Context context = Context::POSITIVE,
				bool cps=false, const set<const PFSymbol*>& cpsymbols=set<const PFSymbol*>())
			: TermMover(context,str->vocabulary()), _structure(str), _cpsupport(cps), _cpsymbols(cpsymbols) { }

	private:
		bool isCPSymbol(const PFSymbol* symbol) const {
			return (VocabularyUtils::isComparisonPredicate(symbol)) || (_cpsymbols.find(symbol) != _cpsymbols.end());
		}

		bool shouldMove(Term* t) {
			if(_movecontext) {
				switch(t->type()) {
					case TT_FUNC: {
						FuncTerm* ft = dynamic_cast<FuncTerm*>(t);
						Function* func = ft->function();
						FuncInter* finter = _structure->inter(func);
						return (not finter->approxTwoValued());
					}
					case TT_AGG: {
						//TODO include test for CPSymbols...
						AggTerm* at = dynamic_cast<AggTerm*>(t);
						return (not SetUtils::approxTwoValued(at->set(),_structure));
					}
					default: { 
						break;
					}
				}
			}
			return false;
		}

		Formula* traverse(PredForm* f) {
			Context savecontext = _context;
			bool savemovecontext = _movecontext;
			_context = isPos(f->sign()) ? _context : not _context;
			for(size_t n = 0; n < f->subterms().size(); ++n) {
				if(_cpsupport) { _movecontext = not isCPSymbol(f->symbol()); }
				f->subterm(n,f->subterms()[n]->accept(this));
			}
			_context = savecontext;
			_movecontext = savemovecontext;
			return f;
		}
};


/*****************************
	Move partial functions 
*****************************/

class PartialTermMover : public TermMover {
	public:
		PartialTermMover(Context context, Vocabulary* voc) : TermMover(context,voc) { }

		bool shouldMove(Term* t) {
			return (_movecontext && TermUtils::isPartial(t));
		}
};

/***********************************
	Replace F(x) = y by P_F(x,y)
***********************************/

class FuncGrapher : public TheoryMutatingVisitor {
	private:
		bool _recursive;
	public:
		FuncGrapher(bool recursive = false) : _recursive(recursive) { }
		Formula*	visit(PredForm* pf);
		Formula*	visit(EqChainForm* ef);
};

Formula* FuncGrapher::visit(PredForm* pf) {
	if(_recursive) { pf = dynamic_cast<PredForm*>(traverse(pf)); }
	if(pf->symbol()->name() == "=/2") {
		PredForm* newpf = 0;
		if(typeid(*(pf->subterms()[0])) == typeid(FuncTerm)) {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterms()[0]);
			vector<Term*> vt = ft->subterms();
			vt.push_back(pf->subterms()[1]);
			newpf = new PredForm(pf->sign(),ft->function(),vt,pf->pi().clone());
			delete(ft); delete(pf);
		}
		else if(typeid(*(pf->subterms()[1])) == typeid(FuncTerm)) {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterms()[1]);
			vector<Term*> vt = ft->subterms();
			vt.push_back(pf->subterms()[0]);
			newpf = new PredForm(pf->sign(),ft->function(),vt,pf->pi().clone());
			delete(ft); delete(pf);
		}
		else {
			newpf = pf;
		}
		return newpf;
	}
	else { return pf; }
}

Formula* FuncGrapher::visit(EqChainForm* ef) {
	const FormulaParseInfo& finalpi = ef->pi();
	bool finalconj = ef->conj();
	if(_recursive) { ef = dynamic_cast<EqChainForm*>(traverse(ef)); }
	set<unsigned int> removecomps;
	set<unsigned int> removeterms;
	vector<Formula*> graphs;
	for(unsigned int comppos = 0; comppos < ef->comps().size(); ++comppos) {
		CompType comparison = ef->comps()[comppos];
		if((comparison == CompType::EQ && ef->conj()) || (comparison == CompType::NEQ && not ef->conj())) {
			if(typeid(*(ef->subterms()[comppos])) == typeid(FuncTerm)) {
				FuncTerm* functerm = dynamic_cast<FuncTerm*>(ef->subterms()[comppos]);
				vector<Term*> vt = functerm->subterms(); vt.push_back(ef->subterms()[comppos+1]);
				graphs.push_back(new PredForm(ef->isConjWithSign()?SIGN::POS:SIGN::NEG,functerm->function(),vt,FormulaParseInfo()));
				removecomps.insert(comppos);
				removeterms.insert(comppos);
			}
			else if(typeid(*(ef->subterms()[comppos+1])) == typeid(FuncTerm)) {
				FuncTerm* functerm = dynamic_cast<FuncTerm*>(ef->subterms()[comppos+1]);
				vector<Term*> vt = functerm->subterms(); vt.push_back(ef->subterms()[comppos]);
				graphs.push_back(new PredForm(ef->isConjWithSign()?SIGN::POS:SIGN::NEG,functerm->function(),vt,FormulaParseInfo()));
				removecomps.insert(comppos);
				removeterms.insert(comppos+1);
			}
		}
	}
	if(not graphs.empty()) {
		vector<Term*> newterms;
		vector<CompType> newcomps;
		for(size_t n = 0; n < ef->comps().size(); ++n) {
			if(removecomps.find(n) == removecomps.end()) { newcomps.push_back(ef->comps()[n]); }
		}
		for(size_t n = 0; n < ef->subterms().size(); ++n) {
			if(removeterms.find(n) == removeterms.end()) { newterms.push_back(ef->subterms()[n]); }
			else { delete(ef->subterms()[n]); }
		}
		EqChainForm* newef = new EqChainForm(ef->sign(),ef->conj(),newterms,newcomps,FormulaParseInfo());
		delete(ef);
		ef = newef;
	}
	
	bool remainingfuncterms = false;
	for(auto it = ef->subterms().begin(); it != ef->subterms().end(); ++it) {
		if(typeid(*(*it)) == typeid(FuncTerm)) {
			remainingfuncterms = true;
			break;
		}
	}
	Formula* nf = 0;
	if(remainingfuncterms) {
		EqChainRemover ecr;
		Formula* f = ecr.visit(ef);
		nf = f->accept(this);
	}
	else { nf = ef; }

	if(graphs.empty()) { return nf; }
	else {
		graphs.push_back(nf);
		return new BoolForm(SIGN::POS,isConj(nf->sign(), finalconj),graphs,finalpi.clone());
	}
}

class AggGrapher : public TheoryMutatingVisitor {
	private:
		bool _recursive;
	public:
		AggGrapher(bool recursive = false) : _recursive(recursive) { }
		Formula*	visit(PredForm* pf);
		Formula*	visit(EqChainForm* ef);
};

Formula* AggGrapher::visit(PredForm* pf) {
	if(_recursive) { pf = dynamic_cast<PredForm*>(traverse(pf)); }
	if(VocabularyUtils::isComparisonPredicate(pf->symbol())) {
		CompType comparison;
		if(pf->symbol()->name() == "=/2") {
			if(isPos(pf->sign())) { comparison = CompType::EQ; }
			else { comparison = CompType::NEQ; }
		}
		else if(pf->symbol()->name() == "</2") {
			if(isPos(pf->sign())) { comparison = CompType::LT; }
			else { comparison = CompType::GEQ; }
		}
		else {
			assert(pf->symbol()->name() == ">/2");
			if(isPos(pf->sign())) { comparison = CompType::GT; }
			else { comparison = CompType::LEQ; }
		}
		Formula* newpf = 0;
		if(typeid(*(pf->subterms()[0])) == typeid(AggTerm)) {
			AggTerm* at = dynamic_cast<AggTerm*>(pf->subterms()[0]);
			newpf = new AggForm(SIGN::POS,pf->subterms()[1],comparison,at,pf->pi().clone());
			delete(pf);
		}
		else if(typeid(*(pf->subterms()[1])) == typeid(AggTerm)) {
			AggTerm* at = dynamic_cast<AggTerm*>(pf->subterms()[1]);
			newpf = new AggForm(SIGN::POS,pf->subterms()[0],comparison,at,pf->pi().clone());
			delete(pf);
		}
		else {
			newpf = pf;
		}
		return newpf;
	}
	else { return pf; }
}

Formula* AggGrapher::visit(EqChainForm* ef) {
	if(_recursive) { ef = dynamic_cast<EqChainForm*>(traverse(ef)); }
	bool containsaggregates = false;
	for(unsigned int n = 0; n < ef->subterms().size(); ++n) {
		if(typeid(*(ef->subterms()[n])) == typeid(AggTerm)) {
			containsaggregates = true;
			break;
		}
	}
	if(containsaggregates) {
		EqChainRemover ecr;
		Formula* f = ecr.visit(ef);
		return f->accept(this);
	}
	else { return ef; }
}

/**
 * Count the number of subformulas
 */
class FormulaCounter : public TheoryVisitor {
	private:
		int	_result;
		void addAndTraverse(const Formula* f) { ++_result; traverse(f);	}
	public:
		FormulaCounter() : _result(0) { }
		int result() const { return _result;	}
		void visit(const PredForm* f)		{ addAndTraverse(f);	}
		void visit(const BoolForm* f)		{ addAndTraverse(f);	}
		void visit(const EqChainForm* f)	{ addAndTraverse(f);	}
		void visit(const QuantForm* f)		{ addAndTraverse(f);	}
		void visit(const EquivForm* f)		{ addAndTraverse(f);	}
		void visit(const AggForm* f)		{ addAndTraverse(f);	}
};

class FormulaFuncTermChecker : public TheoryVisitor {
	private:
		bool _result;
		void visit(const FuncTerm*) {
			_result = true;
		}
	public:
		bool run(Formula* f) {
			_result = false;
			f->accept(this);
			return _result;
		}
};

class Substituter : public TheoryMutatingVisitor {
	private:
		Term* _term;
		Variable* _variable;

		Term* traverse(Term* t) {
			if(t == _term) { return new VarTerm(_variable,TermParseInfo()); }
			else { return t; }
		}
	public:
		Substituter(Term* t, Variable* v) : _term(t), _variable(v) { }
};

namespace FormulaUtils {

	double estimatedCostAll(PredForm* query, const std::set<Variable*> freevars, bool inverse, AbstractStructure* structure) {
		FOBDDManager manager;
		FOBDDFactory factory(&manager);
		const FOBDD* bdd = factory.run(query);
		if(inverse) { bdd = manager.negation(bdd); }
		set<const FOBDDDeBruijnIndex*> indices;
//cerr << "Estimating the cost of bdd\n";
//manager.put(cerr,bdd);
//cerr << "With variables ";
//for(auto it = freevars.begin(); it != freevars.end(); ++it) cerr << *(*it) << ' ';
//cerr << endl;
		double res = manager.estimatedCostAll(bdd,manager.getVariables(freevars),indices,structure);
//cerr << "Estimated " << res << endl;
		return res;
	}


	Formula* removeNesting(Formula* f, Context context)	{
		TermMover atm(context); 
		return f->accept(&atm);			
	}

	Formula* removeEquiv(Formula* f) { 
		EquivRemover er; 
		return f->accept(&er);
	}

	Formula* flatten(Formula* f)	{ 
		Flattener fl; 
		return f->accept(&fl);				
	}

	Formula* substitute(Formula* f, Term* t, Variable* v) {
		Substituter s(t,v);
		return f->accept(&s);
	}

	bool containsFuncTerms(Formula* f) {
		FormulaFuncTermChecker fftc;
		return fftc.run(f);
	}

	BoolForm* trueFormula() {
		return new BoolForm(SIGN::POS,true,vector<Formula*>(0),FormulaParseInfo());
	}

	BoolForm* falseFormula() {
		return new BoolForm(SIGN::POS,false,vector<Formula*>(0),FormulaParseInfo());
	}

	Formula* removeEqChains(Formula* f, Vocabulary* v) {
		EqChainRemover ecr(v);
		Formula* newf = f->accept(&ecr);
		return newf;
	}

	Formula* graphFunctions(Formula* f) {
		FuncGrapher fg(true);
		Formula* newf = f->accept(&fg);
		return newf;
	}

//	Formula* moveThreeValTerms(Formula* f, AbstractStructure* str, bool poscontext) {
//		ThreeValTermMover tvtm(str,(poscontext ? PC_POSITIVE : PC_NEGATIVE));
//		return f->accept(&tvtm);
//	}

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
	 *		\param Context	true iff we are in a positive context
	 *		\param usingcp
	 *
	 *		\return The rewritten formula. If no rewriting was needed, it is the same pointer as pf.
	 *		If rewriting was needed, pf can be deleted, but not recursively.
	 *		
	 */
	Formula* moveThreeValuedTerms(Formula* f, AbstractStructure* str, Context posc, bool cpsupport, const set<const PFSymbol*> cpsymbols) {
		ThreeValuedTermMover tvtm(str,posc,cpsupport,cpsymbols);
		Formula* rewriting = f->accept(&tvtm);
		return rewriting;
	}

	Formula* movePartialTerms(Formula* f, Vocabulary* voc, Context context) {
		PartialTermMover ptm(context,voc);
		Formula* rewriting = f->accept(&ptm);
		return rewriting;
	}

	bool isMonotone(const AggForm* af) {
		switch(af->comp()) {
			case CompType::EQ: case CompType::NEQ: return false;
			case CompType::LT: case CompType::LEQ: {
				switch(af->right()->function()) {
					case AggFunction::CARD : case AggFunction::MAX: return isPos(af->sign());
					case AggFunction::MIN : return isNeg(af->sign());
					case AggFunction::SUM : return isPos(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AggFunction::PROD : return isPos(af->sign());//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			case CompType::GT: case CompType::GEQ: {
				switch(af->right()->function()) {
					case AggFunction::CARD : case AggFunction::MAX: return isNeg(af->sign());
					case AggFunction::MIN : return isPos(af->sign());
					case AggFunction::SUM : return isNeg(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AggFunction::PROD : return isNeg(af->sign());//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			default : assert(false);
		}
		return false;
	}

	bool isAntimonotone(const AggForm* af) {
		switch(af->comp()) {
			case CompType::EQ: case CompType::NEQ: return false;
			case CompType::LT: case CompType::LEQ: {
				switch(af->right()->function()) {
					case AggFunction::CARD : case AggFunction::MAX: return isNeg(af->sign());
					case AggFunction::MIN : return isPos(af->sign());
					case AggFunction::SUM : return isNeg(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AggFunction::PROD : return isNeg(af->sign());//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			case CompType::GT: case CompType::GEQ: {
				switch(af->right()->function()) {
					case AggFunction::CARD : case AggFunction::MAX: return isPos(af->sign());
					case AggFunction::MIN : return isNeg(af->sign());
					case AggFunction::SUM : return isPos(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AggFunction::PROD : return isPos(af->sign());//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			default : assert(false);
		}
		return false;
	}
}

namespace TheoryUtils {

	void pushNegations(AbstractTheory* t)	{ NegationPush np; t->accept(&np);		}
	void removeEquiv(AbstractTheory* t)		{ EquivRemover er; t->accept(&er);		}
	void flatten(AbstractTheory* t)			{ Flattener f; t->accept(&f);			}
	void removeEqChains(AbstractTheory* t)	{ EqChainRemover er; t->accept(&er);	}
	void moveQuantifiers(AbstractTheory* t)	{ QuantMover qm; t->accept(&qm);		}
	void removeNesting(AbstractTheory* t)	{ TermMover atm; t->accept(&atm);		}
	void completion(AbstractTheory* t)		{ Completer c; t->accept(&c);			}
	int  nrSubformulas(AbstractTheory* t)	{ FormulaCounter c; t->accept(&c); return c.result();	}
	void graphFunctions(AbstractTheory* t)	{ FuncGrapher fg(true); t->accept(&fg);	}
	void graphAggregates(AbstractTheory* t)	{ AggGrapher ag; t->accept(&ag);		}

	AbstractTheory* merge(AbstractTheory* at1, AbstractTheory* at2) {
		if(typeid(*at1) != typeid(Theory) || typeid(*at2) != typeid(Theory)) {
			notyetimplemented("Only merging of normal theories has been implemented...");
		}
		//TODO merge vocabularies?
		if(at1->vocabulary() == at2->vocabulary()) {
			AbstractTheory* at = at1->clone();
			Theory* t2 = static_cast<Theory*>(at2);
			for(auto it = t2->sentences().begin(); it != t2->sentences().end(); ++it) {
				at->add((*it)->clone());
			}
			for(auto it = t2->definitions().begin(); it != t2->definitions().end(); ++it) {
				at->add((*it)->clone());
			}
			for(auto it = t2->fixpdefs().begin(); it != t2->fixpdefs().end(); ++it) {
				at->add((*it)->clone());
			}
			return at;
		}
		else { return NULL; }
	}
}

/***************
	Visitors
***************/

void TheoryVisitor::visit(const Theory* t) {
	for(auto it = t->sentences().begin(); it != t->sentences().end(); ++it) {
		(*it)->accept(this);
	}
	for(auto it = t->definitions().begin(); it != t->definitions().end(); ++it) {
		(*it)->accept(this);
	}
	for(auto it = t->fixpdefs().begin(); it != t->fixpdefs().end(); ++it) {
		(*it)->accept(this);
	}
}

void TheoryVisitor::visit(const GroundTheory<GroundPolicy>* ) {
	// TODO
}

void TheoryVisitor::visit(const GroundTheory<SolverPolicy>* ) {
	// TODO
}

void TheoryVisitor::visit(const GroundTheory<PrintGroundPolicy>* ) {
	// TODO
}

void TheoryVisitor::traverse(const Formula* f) {
	for(size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterms()[n]->accept(this);
	}
	for(size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformulas()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const PredForm* pf)		{ traverse(pf); } 
void TheoryVisitor::visit(const EqChainForm* ef)	{ traverse(ef); } 
void TheoryVisitor::visit(const EquivForm* ef)		{ traverse(ef); } 
void TheoryVisitor::visit(const BoolForm* bf)		{ traverse(bf); } 
void TheoryVisitor::visit(const QuantForm* qf)		{ traverse(qf); } 
void TheoryVisitor::visit(const AggForm* af)		{ traverse(af); } 

void TheoryVisitor::visit(const Rule* r) {
	r->body()->accept(this);
}

void TheoryVisitor::visit(const Definition* d) {
	for(size_t n = 0; n < d->rules().size(); ++n) {
		d->rules()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const FixpDef* fd) {
	for(size_t n = 0; n < fd->rules().size(); ++n) {
		fd->rules()[n]->accept(this);
	}
	for(size_t n = 0; n < fd->defs().size(); ++n) {
		fd->defs()[n]->accept(this);
	}
}

void TheoryVisitor::traverse(const Term* t) {
	for(size_t n = 0; n < t->subterms().size(); ++n) {
		t->subterms()[n]->accept(this);
	}
	for(size_t n = 0; n < t->subsets().size(); ++n) {
		t->subsets()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const VarTerm* vt)		{ traverse(vt); } 
void TheoryVisitor::visit(const FuncTerm* ft)		{ traverse(ft); } 
void TheoryVisitor::visit(const DomainTerm* dt)		{ traverse(dt); } 
void TheoryVisitor::visit(const AggTerm* at)		{ traverse(at); } 

void TheoryVisitor::traverse(const SetExpr* s) {
	for(size_t n = 0; n < s->subterms().size(); ++n) {
		s->subterms()[n]->accept(this);
	}
	for(size_t n = 0; n < s->subformulas().size(); ++n) {
		s->subformulas()[n]->accept(this);
	}
}

void TheoryVisitor::visit(const EnumSetExpr* es)	{ traverse(es); }
void TheoryVisitor::visit(const QuantSetExpr* qs)	{ traverse(qs); }

Theory* TheoryMutatingVisitor::visit(Theory* t) {
	for(auto it = t->sentences().begin(); it != t->sentences().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for(auto it = t->definitions().begin(); it != t->definitions().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for(auto it = t->fixpdefs().begin(); it != t->fixpdefs().end(); ++it) {
		*it = (*it)->accept(this);
	}
	return t;
}

GroundTheory<GroundPolicy>* TheoryMutatingVisitor::visit(GroundTheory<GroundPolicy>* t) {
	// TODO
	return t;
}

GroundTheory<SolverPolicy>* TheoryMutatingVisitor::visit(GroundTheory<SolverPolicy>* t) {
	// TODO
	return t;
}

GroundTheory<PrintGroundPolicy>* TheoryMutatingVisitor::visit(GroundTheory<PrintGroundPolicy>* t) {
	// TODO
	return t;
}

Formula* TheoryMutatingVisitor::traverse(Formula* f) {
	for(size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterm(n,f->subterms()[n]->accept(this));
	}
	for(size_t n = 0; n < f->subformulas().size(); ++n) {
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
	for(size_t n = 0; n < d->rules().size(); ++n) {
		d->rule(n,d->rules()[n]->accept(this));
	}
	return d;
}

FixpDef* TheoryMutatingVisitor::visit(FixpDef* fd) {
	for(size_t n = 0; n < fd->rules().size(); ++n) {
		fd->rule(n,fd->rules()[n]->accept(this));
	}
	for(size_t n = 0; n < fd->defs().size(); ++n) {
		fd->def(n,fd->defs()[n]->accept(this));
	}
	return fd;
}

Term* TheoryMutatingVisitor::traverse(Term* t) {
	for(size_t n = 0; n < t->subterms().size(); ++n) {
		t->subterm(n,t->subterms()[n]->accept(this));
	}
	for(size_t n = 0; n < t->subsets().size(); ++n) {
		t->subset(n,t->subsets()[n]->accept(this));
	}
	return t;
}

Term* TheoryMutatingVisitor::visit(VarTerm* vt)		{ return traverse(vt); } 
Term* TheoryMutatingVisitor::visit(FuncTerm* ft)	{ return traverse(ft); } 
Term* TheoryMutatingVisitor::visit(DomainTerm* dt)	{ return traverse(dt); } 
Term* TheoryMutatingVisitor::visit(AggTerm* at)		{ return traverse(at); } 

SetExpr* TheoryMutatingVisitor::traverse(SetExpr* s) {
	for(size_t n = 0; n < s->subterms().size(); ++n) {
		s->subterm(n,s->subterms()[n]->accept(this));
	}
	for(size_t n = 0; n < s->subformulas().size(); ++n) {
		s->subformula(n,s->subformulas()[n]->accept(this));
	}
	return s;
}

SetExpr* TheoryMutatingVisitor::visit(EnumSetExpr* es)	{ return traverse(es); }
SetExpr* TheoryMutatingVisitor::visit(QuantSetExpr* qs) { return traverse(qs); }


/***********************
	Definition utils
***********************/

class OpenCollector : public TheoryVisitor {
	private:
		Definition*		_definition;
		set<PFSymbol*>	_result;
		void visit(const PredForm* pf) {
			if(_definition->defsymbols().find(pf->symbol()) == _definition->defsymbols().end()) {
				_result.insert(pf->symbol());
			}
			traverse(pf);
		}
		void visit(const FuncTerm* ft) {
			if(_definition->defsymbols().find(ft->function()) == _definition->defsymbols().end()) {
				_result.insert(ft->function());
			}
			traverse(ft);
		}
	public:
		const set<PFSymbol*>& run(Definition* d) {
			_definition = d;
			_result.clear();
			d->accept(this);
			return _result;
		}
};

namespace DefinitionUtils {
	set<PFSymbol*> opens(Definition* d) { OpenCollector oc; return oc.run(d);	}
}
