/************************************
	theory.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theory.hpp"

#include <iostream>
#include <typeinfo>
#include <set>

#include "structure.hpp"
#include "data.hpp"
#include "visitor.hpp"
#include "builtin.hpp"
#include "ecnf.hpp"
#include "ground.hpp"

using namespace std;

extern string tabstring(unsigned int);
extern bool nexttuple(vector<unsigned int>&,const vector<unsigned int>&);

/*******************
	Constructors
*******************/

/** Cloning while keeping free variables **/

PredForm* PredForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

BracketForm* BracketForm::clone() const {
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

AggForm* AggForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

/** Cloning while substituting free variables **/

PredForm* PredForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> na(_args.size());
	for(unsigned int n = 0; n < _args.size(); ++n) na[n] = _args[n]->clone(mvv);
	PredForm* pf = new PredForm(_sign,_symb,na,_pi);
	return pf;
}

BracketForm* BracketForm::clone(const map<Variable*,Variable*>& mvv) const {
	Formula* nf = _subf->clone(mvv);
	return new BracketForm(_sign,nf);
}

EqChainForm* EqChainForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> nt(_terms.size());
	for(unsigned int n = 0; n < _terms.size(); ++n) nt[n] = _terms[n]->clone(mvv);
	EqChainForm* ef = new EqChainForm(_sign,_conj,nt,_comps,_signs,_pi);
	return ef;
}

EquivForm* EquivForm::clone(const map<Variable*,Variable*>& mvv) const {
	Formula* nl = _left->clone(mvv);
	Formula* nr = _right->clone(mvv);
	EquivForm* ef =  new EquivForm(_sign,nl,nr,_pi);
	return ef;
}

BoolForm* BoolForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Formula*> ns(_subf.size());
	for(unsigned int n = 0; n < _subf.size(); ++n) ns[n] = _subf[n]->clone(mvv);
	BoolForm* bf = new BoolForm(_sign,_conj,ns,_pi);
	return bf;
}

QuantForm* QuantForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Variable*> nv(_vars.size());
	map<Variable*,Variable*> nmvv = mvv;
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		nv[n] = new Variable(_vars[n]->name(),_vars[n]->sort(),_pi);
		nmvv[_vars[n]] = nv[n];
	}
	Formula* nf = _subf->clone(nmvv);
	QuantForm* qf = new QuantForm(_sign,_univ,nv,nf,_pi);
	return qf;
}

AggForm* AggForm::clone(const map<Variable*,Variable*>& mvv) const {
	Term* nl = _left->clone(mvv);
	AggTerm* nr = _right->clone(mvv);
	return new AggForm(_sign,_comp,nl,nr,_pi);
}


/*****************
	Destructors
*****************/

void PredForm::recursiveDelete() {
	for(unsigned int n = 0; n < _args.size(); ++n) _args[n]->recursiveDelete();
	delete(this);
}

void BracketForm::recursiveDelete() {
	_subf->recursiveDelete();
	delete(this);
}

void EqChainForm::recursiveDelete() {
	for(unsigned int n = 0; n < _terms.size(); ++n) _terms[n]->recursiveDelete();
	delete(this);
}

void EquivForm::recursiveDelete() {
	_left->recursiveDelete();
	_right->recursiveDelete();
	delete(this);
}

void BoolForm::recursiveDelete() {
	for(unsigned int n = 0; n < _subf.size(); ++n) _subf[n]->recursiveDelete();
	delete(this);
}

void QuantForm::recursiveDelete() {
	_subf->recursiveDelete();
	for(unsigned int n = 0; n < _vars.size(); ++n) delete(_vars[n]);
	delete(this);
}

void AggForm::recursiveDelete() {
	_left->recursiveDelete();
	_right->recursiveDelete();
	delete(this);
}

void Rule::recursiveDelete() {
	_head->recursiveDelete();
	_body->recursiveDelete();
	for(unsigned int n = 0; n < nrQvars(); ++n) delete(qvar(n));
	delete(this);
}

void Definition::recursiveDelete() {
	for(unsigned int n = 0; n < _rules.size(); ++n) _rules[n]->recursiveDelete();
	delete(this);
}

void FixpDef::recursiveDelete() {
	for(unsigned int n = 0; n < _rules.size(); ++n) _rules[n]->recursiveDelete();
	for(unsigned int n = 0; n < _defs.size(); ++n) _defs[n]->recursiveDelete();
	delete(this);
}

void Theory::recursiveDelete() {
	for(unsigned int n = 0; n < _sentences.size(); ++n) _sentences[n]->recursiveDelete();
	for(unsigned int n = 0; n < _definitions.size(); ++n) _definitions[n]->recursiveDelete();
	for(unsigned int n = 0; n < _fixpdefs.size(); ++n) _fixpdefs[n]->recursiveDelete();
	delete(this);
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

void Definition::defsyms() {
	_defsyms.clear();
	for(unsigned int n = 0; n < _rules.size(); ++n) 
		add(_rules[n]->head()->symb());
}

void FixpDef::defsyms() {
	_defsyms.clear();
	for(unsigned int n = 0; n < _rules.size(); ++n) 
		add(_rules[n]->head()->symb());
}

/***************************
	Containment checking
***************************/

bool Formula::contains(const Variable* v) const {
	for(unsigned int n = 0; n < nrQvars(); ++n)
		if(qvar(n) == v) return true;
	for(unsigned int n = 0; n < nrSubterms(); ++n)
		if(subterm(n)->contains(v)) return true;
	for(unsigned int n = 0; n < nrSubforms(); ++n)
		if(subform(n)->contains(v)) return true;
	return false;
}

class ContainmentChecker : public Visitor {
	private:
		const PFSymbol*	_symbol;
		bool			_result;
	public:
		ContainmentChecker(const Formula* f, const PFSymbol* s) : Visitor(), _symbol(s), _result(false) { f->accept(this); }
		void visit(const PredForm* pf) { _result = (pf->symb() == _symbol); traverse(pf);	}
		void visit(const FuncTerm* ft) { _result = (ft->func() == _symbol); traverse(ft);	}
		bool result() { return _result; }
};

bool Formula::contains(const PFSymbol* s) const {
	ContainmentChecker cc(this,s);
	return cc.result();
}

/****************
	Debugging
****************/

string PredForm::to_string(unsigned int) const {
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

string BracketForm::to_string(unsigned int) const {
	string s;
	if(!_sign) s = s+ '~';
	s = s + "(" + _subf->to_string() + ")";
	return s;
}

string EqChainForm::to_string(unsigned int) const {
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

string EquivForm::to_string(unsigned int) const {
	string s = '(' + _left->to_string() + " <=> " + _right->to_string() + ')';
	return s;
}

string BoolForm::to_string(unsigned int) const {
	string s;
	if(_subf.empty()) {
		if(_sign == _conj) return "true";
		else return "false";
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

string QuantForm::to_string(unsigned int) const {
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

string AggForm::to_string(unsigned int) const {
	string s;
	if(!_sign) s = s + '~';
	s = s + '(' + _left->to_string() + _comp + _right->to_string() + ')';
	return s;
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


/********************
	Formula utils
********************/

/** Evaluate formulas **/

namespace TVUtils {

	TruthValue swaptruthvalue(TruthValue v) {
		switch(v) {
			case TV_TRUE: return TV_FALSE;
			case TV_FALSE: return TV_TRUE;
			default: return v;
		}
	}

	TruthValue glbt(TruthValue v1, TruthValue v2) {
		switch(v1) {
			case TV_TRUE:
				return v2;
			case TV_FALSE:
				return TV_FALSE;
			case TV_UNKN:
				switch(v2) {
					case TV_TRUE: 
					case TV_UNKN: 
						return TV_UNKN;
					case TV_FALSE: 
						return TV_FALSE;
				}
			default: assert(false); return TV_UNKN;
		}
	}

	TruthValue lubt(TruthValue v1, TruthValue v2) {
		switch(v1) {
			case TV_TRUE:
				return TV_TRUE;
			case TV_FALSE:
				return v2;
			case TV_UNKN:
				switch(v2) {
					case TV_TRUE: 
						return TV_TRUE;
					case TV_FALSE: 
					case TV_UNKN: 
						return TV_UNKN;
				}
			default: assert(false); return TV_UNKN;
		}
	}

	string to_string(TruthValue v) {
		switch(v) {
			case TV_TRUE: return "true";
			case TV_FALSE: return "false";
			case TV_UNKN: return "unknown";
			default: assert(false); return "";
		}
	}
}

class FormulaEvaluator : public Visitor {

	private:
		AbstractStructure*			_structure;
		map<Variable*,TypedElement>	_varmapping;
		TruthValue					_returnvalue;
		bool						_context;

	public:
		
		FormulaEvaluator(Formula* f, AbstractStructure* s, const map<Variable*,TypedElement>& m) :
			Visitor(), _structure(s), _varmapping(m), _context(true) { f->accept(this);	}
		
		TruthValue returnvalue()	const { return _returnvalue;	}

		void visit(const PredForm*);
		void visit(const BoolForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const QuantForm*);
		
};

void FormulaEvaluator::visit(const PredForm* pf) {
	// Evaluate the terms
	vector<SortTable*> argvalues(pf->nrSubterms());
	vector<TypedElement> currvalue(pf->nrSubterms());
	TermEvaluator* te = new TermEvaluator(_structure,_varmapping);
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		pf->subterm(n)->accept(te);
		argvalues[n] = te->returnvalue();
		currvalue[n]._type = argvalues[n]->type();
	}
	// Set the context
	if(!pf->sign()) _context = !_context;
	// Evaluate the formula
	SortTableTupleIterator stti(argvalues);
	_returnvalue = (_context ? TV_TRUE : TV_FALSE);
	PredInter* pt = _structure->inter(pf->symb());
	if(!stti.empty()) {
		do {
			for(unsigned int n = 0; n < pf->nrSubterms(); ++n) currvalue[n]._element = stti.value(n);
			if(pt->istrue(currvalue)) {
				if(!_context) {
					if(stti.singleton()) _returnvalue = TV_TRUE;
					else _returnvalue = TV_UNKN;
					break;
				}
			}
			else if(pt->isfalse(currvalue)) {
				if(_context) {
					if(stti.singleton()) _returnvalue = TV_FALSE;
					else _returnvalue = TV_UNKN;
					break;
				}
			}
			else {
				_returnvalue = TV_UNKN;
			}
		} while(stti.nextvalue());
	}
	if(!pf->sign()) {
		_returnvalue = TVUtils::swaptruthvalue(_returnvalue);
		_context = !_context;
	}
	// Delete tables
	for(unsigned int n = 0; n < argvalues.size(); ++n) delete(argvalues[n]);
}

void FormulaEvaluator::visit(const EquivForm* ef) {
	// NOTE: evaluating an equivalence in a three-valued structure/context with partial functions
	//		 leads to ambiguities! TODO: give a warning!
	ef->left()->accept(this);
	if(_returnvalue != TV_UNKN) {
		TruthValue lv = _returnvalue;
		ef->right()->accept(this);
		if(_returnvalue != TV_UNKN) {
			_returnvalue = (_returnvalue == lv ? TV_TRUE : TV_FALSE);
		}
	}
	if(!ef->sign()) _returnvalue = TVUtils::swaptruthvalue(_returnvalue);
}

void FormulaEvaluator::visit(const EqChainForm* ef) {
	vector<Formula*> vf(ef->nrSubterms()-1);
	for(unsigned int n = 0; n < vf.size(); ++n) {
		vector<Term*> vt(2); vt[0] = ef->subterm(n); vt[1] = ef->subterm(n+1);
		string pn = string(1,ef->comp(n)) + "/2";
		vf[n] = new PredForm(ef->compsign(n),StdBuiltin::instance()->pred(pn),vt,FormParseInfo());
	}
	BoolForm* bf = new BoolForm(ef->sign(),ef->conj(),vf,FormParseInfo());
	bf->accept(this);
	for(unsigned int n = 0; n < vf.size(); ++n) { delete(vf[n]); }
	delete(bf);
}

void FormulaEvaluator::visit(const BoolForm* bf) {
	if(!bf->sign()) _context = !_context;
	TruthValue result;
	if(bf->conj()) result = TV_TRUE;
	else result = TV_FALSE;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		bf->subform(n)->accept(this);
		result = (bf->conj()) ? TVUtils::glbt(result,_returnvalue) : TVUtils::lubt(result,_returnvalue) ;
		if((bf->conj() && result == TV_FALSE) || (!(bf->conj()) && result == TV_TRUE)) break;
	}
	_returnvalue = result;
	if(!bf->sign()) {
		_returnvalue = TVUtils::swaptruthvalue(_returnvalue);
		_context = !_context;
	}
	return;
}

void FormulaEvaluator::visit(const QuantForm* qf) {
	if(!qf->sign()) _context = !_context;
	TruthValue result = (qf->univ()) ? TV_TRUE : TV_FALSE;
	SortTableTupleIterator vti(qf->qvars(),_structure);
	if(!vti.empty()) {
		for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
			TypedElement e; 
			e._type = vti.type(n); 
			_varmapping[qf->qvar(n)] = e;
		}
		do {
			for(unsigned int n = 0; n < qf->nrQvars(); ++n) 
				_varmapping[qf->qvar(n)]._element = vti.value(n);
			qf->subf()->accept(this);
			result = (qf->univ()) ? TVUtils::glbt(result,_returnvalue) : TVUtils::lubt(result,_returnvalue) ;
			if((qf->univ() && result == TV_FALSE) || ((!qf->univ()) && result == TV_TRUE)) break;
		} while(vti.nextvalue());
	}
	_returnvalue = result;
	if(!qf->sign()) {
		_returnvalue = TVUtils::swaptruthvalue(_returnvalue);
		_context = !_context;
	}
	return;
}

/*************************
	Rewriting theories
*************************/

void Theory::add(Theory* t) {
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n)
		_definitions.push_back(t->definition(n));
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n)
		_fixpdefs.push_back(t->fixpdef(n));
	for(unsigned int n = 0; n < t->nrSentences(); ++n)
		_sentences.push_back(t->sentence(n));
}

Theory* Theory::clone() const {
	Theory* newtheory = new Theory(_name,_vocabulary,ParseInfo());
	for(unsigned int n = 0; n < _sentences.size(); ++n) 
		newtheory->add(_sentences[n]->clone());
	for(unsigned int n = 0; n < _definitions.size(); ++n) 
		newtheory->add(_definitions[n]->clone());
	for(unsigned int n = 0; n < _fixpdefs.size(); ++n) 
		newtheory->add(_fixpdefs[n]->clone());
	return newtheory;
}

Definition* Definition::clone() const {
	Definition* newdef = new Definition();
	for(unsigned int n = 0; n < _rules.size(); ++n) newdef->add(_rules[n]->clone());
	newdef->defsyms();
	return newdef;
}

FixpDef* FixpDef::clone() const {
	FixpDef* newfd = new FixpDef(_lfp);
	for(unsigned int n = 0; n < _defs.size(); ++n) newfd->add(_defs[n]->clone());
	for(unsigned int n = 0; n < _rules.size(); ++n) newfd->add(_rules[n]->clone());
	newfd->defsyms();
	return newfd;
}

Rule* Rule::clone() const {
	map<Variable*,Variable*> mvv;
	vector<Variable*> newqv;
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		Variable* v = new Variable(_vars[n]->name(),_vars[n]->sort(),ParseInfo());
		mvv[_vars[n]] = v;
		newqv.push_back(v);
	}
	return new Rule(newqv,_head->clone(mvv),_body->clone(mvv),ParseInfo());
}

/** Push negations inside **/

class NegationPush : public MutatingVisitor {

	public:
		NegationPush(AbstractTheory* t)	: MutatingVisitor() { t->accept(this);	}

		Formula*	visit(EqChainForm*);
		Formula* 	visit(EquivForm*);
		Formula* 	visit(BoolForm*);
		Formula* 	visit(QuantForm*);

		Formula*	traverse(Formula*);
		Term*		traverse(Term*);

};

Formula* NegationPush::traverse(Formula* f) {
	for(unsigned int n = 0; n < f->nrSubforms(); ++n)
		f->subform(n)->accept(this);
	for(unsigned int n = 0; n < f->nrSubterms(); ++n)
		f->subterm(n)->accept(this);
	return f;
}

Term* NegationPush::traverse(Term* t) {
	for(unsigned int n = 0; n < t->nrSubforms(); ++n)
		t->subform(n)->accept(this);
	for(unsigned int n = 0; n < t->nrSubterms(); ++n)
		t->subterm(n)->accept(this);
	return t;
}

Formula* NegationPush::visit(EqChainForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->conj(!f->conj());
		for(unsigned int n = 0; n < f->nrSubterms()-1; ++n)  f->compsign(n,!f->compsign(n));
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
		for(unsigned int n = 0; n < f->nrSubforms(); ++n) 
			f->subform(n)->swapsign();
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

/** Rewrite A <=> B to (A => B) & (B => A) **/

class EquivRemover : public MutatingVisitor {

	public:
		EquivRemover(AbstractTheory* t)	: MutatingVisitor() { t->accept(this);	}

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

/** Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc. **/

class Flattener : public MutatingVisitor {

	public:
		Flattener(AbstractTheory* t) : MutatingVisitor() { t->accept(this);	}

		Formula*	visit(BoolForm*);
		Formula* 	visit(QuantForm*);

		Formula*	traverse(Formula*);
		Term*		traverse(Term*);

};

Formula* Flattener::traverse(Formula* f) {
	for(unsigned int n = 0; n < f->nrSubforms(); ++n)
		f->subform(n)->accept(this);
	for(unsigned int n = 0; n < f->nrSubterms(); ++n)
		f->subterm(n)->accept(this);
	return f;
}

Term* Flattener::traverse(Term* t) {
	for(unsigned int n = 0; n < t->nrSubforms(); ++n)
		t->subform(n)->accept(this);
	for(unsigned int n = 0; n < t->nrSubterms(); ++n)
		t->subterm(n)->accept(this);
	return t;
}

Formula* Flattener::visit(BoolForm* bf) {
	vector<Formula*> newsubf;
	traverse(bf);
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		if(typeid(*(bf->subform(n))) == typeid(BoolForm)) {
			BoolForm* sbf = dynamic_cast<BoolForm*>(bf->subform(n));
			if((bf->conj() == sbf->conj()) && sbf->sign()) {
				for(unsigned int m = 0; m < sbf->nrSubforms(); ++m) newsubf.push_back(sbf->subform(m));
				delete(sbf);
			}
			else newsubf.push_back(bf->subform(n));
		}
		else newsubf.push_back(bf->subform(n));
	}
	bf->subf(newsubf);
	return bf;
}

Formula* Flattener::visit(QuantForm* qf) {
	traverse(qf);	
	if(typeid(*(qf->subf())) == typeid(QuantForm)) {
		QuantForm* sqf = dynamic_cast<QuantForm*>(qf->subf());
		if((qf->univ() == sqf->univ()) && sqf->sign()) {
			qf->subf(sqf->subf());
			for(unsigned int n = 0; n < sqf->nrQvars(); ++n) 
				qf->add(sqf->qvar(n));
			delete(sqf);
		}
	}
	return qf;
}

/**  Rewrite chains of equalities to a conjunction or disjunction of atoms **/

class EqChainRemover : public MutatingVisitor {

	private:
		Vocabulary* _vocab;

	public:
		EqChainRemover(AbstractTheory* t)	: MutatingVisitor(), _vocab(t->vocabulary()) { t->accept(this);	}
		EqChainRemover()	: MutatingVisitor(), _vocab(0) {	}
		EqChainRemover(Vocabulary* v)	: MutatingVisitor(), _vocab(v) {	}

		Formula* visit(EqChainForm*);

};

Formula* EqChainRemover::visit(EqChainForm* ef) {
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) ef->subterm(n)->accept(this);
	vector<Formula*> vf;
	for(unsigned int n = 0; n < ef->nrSubterms()-1; ++n) {
		Predicate* p = 0;
		switch(ef->comp(n)) {
			case '=' : p = StdBuiltin::instance()->pred("=/2"); break;
			case '<' : p = StdBuiltin::instance()->pred("</2"); break;
			case '>' : p = StdBuiltin::instance()->pred(">/2"); break;
			default: assert(false);
		}
		vector<Sort*> vs(2); vs[0] = ef->subterm(n)->sort(); vs[1] = ef->subterm(n+1)->sort();
		p = p->disambiguate(vs,_vocab);
		assert(p);
		vector<Term*> vt(2); 
		if(n) vt[0] = ef->subterm(n)->clone();
		else vt[0] = ef->subterm(n);
		vt[1] = ef->subterm(n+1);
		PredForm* pf = new PredForm(ef->compsign(n),p,vt,ef->pi());
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

/** Move quantifiers **/

class QuantMover : public MutatingVisitor {

	public:
		QuantMover(AbstractTheory* t)	: MutatingVisitor() { t->accept(this);	}

		Formula* visit(QuantForm*);

};

Formula* QuantMover::visit(QuantForm* qf) {
	if(typeid(*(qf->subf())) == typeid(BoolForm)) {
		BoolForm* bf = dynamic_cast<BoolForm*>(qf->subf());
		bool u = (qf->univ() == qf->sign());
		bool c = (bf->conj() == bf->sign());
		if(u == c) {
			bool s = (qf->sign() == bf->sign());
			vector<Formula*> vf(bf->nrSubforms());
			for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
				QuantForm* nqf = new QuantForm(s,u,qf->qvars(),bf->subform(n),FormParseInfo());
				vf[n] = nqf->clone();
				delete(nqf);
			}
			qf->subf()->recursiveDelete();
			BoolForm* nbf = new BoolForm(true,c,vf,FormParseInfo());
			delete(qf);
			return nbf->accept(this);
		}
	}
	return MutatingVisitor::visit(qf);
}

/** Tseitin transformation **/

class Tseitinizer : public MutatingVisitor {
	private:
		bool			_insiderule;
		bool			_maketseitin;
		AbstractTheory*	_theory;
	public:

		Tseitinizer(AbstractTheory* t);

		Theory*		visit(Theory*);
		Formula*	visit(BoolForm*);
		Formula*	visit(EqChainForm*);
		Formula*	visit(QuantForm*);

};

Tseitinizer::Tseitinizer(AbstractTheory* t) {
	// Clone t's vocabulary!
	// TODO Vocabulary* v = new Vocabulary(t->vocabulary());
	// TODO t->vocabulary(v);
	
	// Prepare the theory
	TheoryUtils::remove_eqchains(t);
	TheoryUtils::push_negations(t);
	TheoryUtils::flatten(t);

	// Apply the transformation
	_theory = t;
	t->accept(this);
}

Theory* Tseitinizer::visit(Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_maketseitin = false;
		_insiderule = false;
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
		_maketseitin = false;
		_insiderule = true;
		Definition* d = t->definition(n)->accept(this);
		t->definition(n,d);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_maketseitin = false;
		_insiderule = true;
		FixpDef* fd = t->fixpdef(n)->accept(this);
		t->fixpdef(n,fd);
	}
	return t;
}

Formula* Tseitinizer::visit(EqChainForm* e) {
	assert(false);
	return e;
}

Formula* Tseitinizer::visit(QuantForm* q) {
	assert(false);
	return q;
}

Formula* Tseitinizer::visit(BoolForm* bf) {
	if(_insiderule) {
		// TODO
	}
	else {
		if(_maketseitin) {
			vector<Sort*> vs(bf->nrFvars());
			for(unsigned int n = 0; n < bf->nrFvars(); ++n) vs[n] = bf->fvar(n)->sort();
			Predicate* tspred = new Predicate(vs);
			_theory->vocabulary()->addPred(tspred);
			if(bf->conj()) {	// A conjunction deeper than sentence level
				for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
					map<Variable*,Variable*> mvv;
					vector<Variable*> vv(bf->nrFvars());
					vector<Term*> vt(bf->nrFvars());
					for(unsigned int m = 0; m < bf->nrFvars(); ++m) {
						vv[m] = new Variable(bf->fvar(m)->name(),bf->fvar(m)->sort(),ParseInfo());
						vt[m] = new VarTerm(vv[m],ParseInfo());
						mvv[bf->fvar(m)] = vv[m];
					}
					vector<Formula*> vf(2);
					vf[0] = bf->subform(n)->clone(mvv);
					vf[1] = new PredForm(false,tspred,vt,FormParseInfo());
					BoolForm* f = new BoolForm(true,false,vf,FormParseInfo());
					if(vv.empty()) _theory->add(f);
					else _theory->add(new QuantForm(true,true,vv,f,FormParseInfo()));
					bf->subform(n)->recursiveDelete();
				}
			}
			else {	// A disjunction deeper than sentence level
				map<Variable*,Variable*> mvv;
				vector<Variable*> vv(bf->nrFvars());
				vector<Term*> vt(bf->nrFvars());
				for(unsigned int n = 0; n < bf->nrFvars(); ++n) {
					vv[n] = new Variable(bf->fvar(n)->name(),bf->fvar(n)->sort(),ParseInfo());
					vt[n] = new VarTerm(vv[n],ParseInfo());
					mvv[bf->fvar(n)] = vv[n];
				}
				vector<Formula*> vf;
				vf.push_back(new PredForm(false,tspred,vt,FormParseInfo()));
				for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
					vf.push_back(bf->subform(n)->clone(mvv));
					bf->subform(n)->recursiveDelete();
				}
				BoolForm* f = new BoolForm(true,false,vf,FormParseInfo());
				if(vv.empty()) _theory->add(f);
				else _theory->add(new QuantForm(true,true,vv,f,FormParseInfo()));
			}
			vector<Term*> vt(bf->nrFvars());
			for(unsigned int n = 0; n < bf->nrFvars(); ++n) vt[n] = new VarTerm(bf->fvar(n),ParseInfo());
			delete(bf);
			return new PredForm(true,tspred,vt,FormParseInfo());
		}
		else {
			if(bf->conj()) {	// A conjunction at sentence level: split the conjunction
				if(bf->nrSubforms()) {
					for(unsigned int n = 1; n < bf->nrSubforms(); ++n) {
						_theory->add(bf->subform(n));
					}
					Formula* ret = bf->subform(0)->accept(this);
					delete(bf);
					return ret;
				}
				else return bf;
			}
			else {	// A disjunction at sentence level: go one level deeper
				for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
					_maketseitin = true;
					Formula* nf = bf->subform(n)->accept(this);
					bf->subf(n,nf);
				}
				return bf;
			}
		}
	}
	return bf;
}

/** Partial evaluation over structure **/

class Reducer : public MutatingVisitor {
	private:
		AbstractStructure*			_structure;
	public:
		Reducer(AbstractTheory* t, AbstractStructure* s) : MutatingVisitor(), _structure(s) { t->accept(this);	}
		Theory*		visit(Theory* t);
		Formula*	visit(PredForm* pf);
		Formula*	visit(BoolForm* bf);
		Formula*	visit(QuantForm* qf);
		Formula*	visit(EquivForm* ef);
		Formula*	visit(EqChainForm* ef);
};

Theory* Reducer::visit(Theory* t) {

	// Sentences
	unsigned int move = 0;
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		Formula* f = t->sentence(n)->accept(this);
		if(f->trueformula()) {	// skip true formulas
			++move;
			f->recursiveDelete();
		}
		else if(f->falseformula()) {	// reduce theory to 'false'
			for(unsigned int m = 0; m < n-move; ++m) t->sentence(m)->recursiveDelete();
			for(unsigned int m = n+1; m < t->nrSentences(); ++m) t->sentence(m)->recursiveDelete();
			for(unsigned int m = 0; m < t->nrDefinitions(); ++m) t->definition(m)->recursiveDelete();
			for(unsigned int m = 0; m < t->nrFixpDefs(); ++m) t->fixpdef(m)->recursiveDelete();
			t->clear_sentences();
			t->clear_definitions();
			t->clear_fixpdefs();
			move = 0;
			t->add(f);
			break;
		}
		else {
			vector<Variable*> vv;
			for(unsigned int m = 0; m < f->nrFvars(); ++m) {
				vv.push_back(f->fvar(m));
			}
			if(!vv.empty()) {
				f = new QuantForm(true,true,vv,f,f->pi());
			}
			t->sentence(n-move,f);
		}
	}
	for(unsigned int n = 0; n < move; ++n) t->pop_sentence();

	// Definitions
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		Definition* d = t->definition(n)->accept(this);
		t->definition(n,d);
	}

	// Fixpoint definitions
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		FixpDef* fd = t->fixpdef(n)->accept(this);
		t->fixpdef(n,fd);
	}

	// Return the reduced theory
	return t;
}

Formula* Reducer::visit(PredForm* pf) {
	if(pf->nrFvars() == 0) {
		map<Variable*,TypedElement> mvt;
		TruthValue tv = FormulaUtils::evaluate(pf,_structure,mvt);
		switch(tv) {
			case TV_TRUE:
			case TV_FALSE:
			{
				vector<Formula*> vf(0);
				BoolForm* bf = new BoolForm(true,tv==TV_TRUE,vf,FormParseInfo());
				for(unsigned int n = 0; n < pf->nrSubterms(); ++n) pf->subterm(n)->recursiveDelete();
				delete(pf);
				return bf;
			}
			default:
				return pf;
		}
	}
	else return pf;
}

Formula* Reducer::visit(BoolForm* bf) {
	vector<Formula*> vf;
	unsigned int n = 0;
	Formula* nf;
	for(; n < bf->nrSubforms(); ++n) {
		nf = bf->subform(n)->accept(this);
		if(nf == bf->subform(n)) {
			nf = nf->clone();
			bf->subform(n)->recursiveDelete();
		}
		else delete(bf->subform(n));
		if(nf->trueformula()) {
			if(!bf->conj()) break; 
			else nf->recursiveDelete();
		}
		else if(nf->falseformula()) {
			if(bf->conj()) break;
			else nf->recursiveDelete();
		}
		else vf.push_back(nf);
	}
	if(n != bf->nrSubforms()) {
		for(unsigned int m = 0; m < vf.size(); ++m) vf[m]->recursiveDelete();
		for(unsigned int m = n+1; m < bf->nrSubforms(); ++m) bf->subform(m)->recursiveDelete();
		if(!bf->sign()) nf->swapsign();
		delete(bf);
		return nf;
	}
	else if(vf.size() == 1) {
		if(!bf->sign()) vf[0]->swapsign();
		delete(bf);
		return vf[0];
	}
	else {
		BoolForm* nbf = new BoolForm(bf->sign(),bf->conj(),vf,FormParseInfo());
		delete(bf);
		return nbf;
	}
}

Formula* Reducer::visit(QuantForm* qf) {
	Formula* ns = qf->subf()->accept(this);
	qf->subf(ns);
	qf->setfvars();
	if(qf->nrFvars() == 0) {
		map<Variable*,TypedElement> mvt;
		TruthValue tv = FormulaUtils::evaluate(qf,_structure,mvt);
		switch(tv) {
			case TV_TRUE:
			case TV_FALSE:
			{
				vector<Formula*> vf(0);
				BoolForm* bf = new BoolForm(true,tv==TV_TRUE,vf,FormParseInfo());
				qf->subf()->recursiveDelete();	
				for(unsigned int n = 0; n < qf->nrQvars(); ++n) delete(qf->qvar(n));
				delete(qf);
				return bf;
			}
			default:
				return qf;
		}
	}
	else return qf;
}

Formula* Reducer::visit(EquivForm* ef) {
	Formula* nl = ef->left()->accept(this);
	Formula* nr = ef->right()->accept(this);

	Formula* r = 0;
	if(nl->trueformula()) {
		nl->recursiveDelete();
		r = nr;
	}
	else if(nl->falseformula()) {
		nl->recursiveDelete();
		r = nr;
		r->swapsign();
	}
	else if(nr->trueformula()) {
		nr->recursiveDelete();
		r = nl;
	}
	else if(nr->falseformula()) {
		nr->recursiveDelete();
		r = nl;
		r->swapsign();
	}

	if(r) {
		if(!ef->sign()) r->swapsign();
		delete(ef);
		return r;
	}
	else {
		ef->left(nl);
		ef->right(nr);
		ef->setfvars();
		return ef;
	}
}

Formula* Reducer::visit(EqChainForm* ef) {
	// Reduce subterms
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		Term* nt = ef->subterm(n)->accept(this);
		ef->term(n,nt);
	}
	ef->setfvars();

	unsigned int n = 1;
	vector<unsigned int> splits;
	for(; n < ef->nrSubterms(); ++n) {
		if(ef->subterm(n-1)->nrFvars() == 0 && ef->subterm(n)->nrFvars() == 0) {
			vector<Term*> vt(2); vt[0] = ef->subterm(n-1); vt[1] = ef->subterm(n);
			string pn = string(1,ef->comp(n)) + "/2";
			PredForm* pf = new PredForm(ef->compsign(n-1),StdBuiltin::instance()->pred(pn),vt,FormParseInfo());
			map<Variable*,TypedElement> mvt;
			TruthValue tv = FormulaUtils::evaluate(pf,_structure,mvt);
			delete(pf);
			bool br = false;
			switch(tv) {
				case TV_TRUE:
					if(ef->conj()) splits.push_back(n-1);
					else br = true;
					break;
				case TV_FALSE:
					if(ef->conj()) br = true;
					else splits.push_back(n-1);
					break;
				case TV_UNKN:
					break;
				default:
					assert(false);
			}	
			if(br) break;
		}
	}

	if(n != ef->nrSubterms()) {
		for(unsigned int m = 0; m < ef->nrSubterms(); ++m) ef->subterm(m)->recursiveDelete();
		vector<Formula*> vf(0);
		BoolForm* bf = new BoolForm(true,ef->conj() != ef->sign(),vf,FormParseInfo());
		delete(ef);
		return bf;
	}
	else if(!splits.empty()) {
		vector<Formula*> conjuncts;
		unsigned int prevsplit = 0;
		for(unsigned int m = 0; m < splits.size(); ++m) {
			vector<Term*> vt;
			vector<char> vc;
			vector<bool> vb;
			for(unsigned int k = prevsplit; k < splits[m]; ++k) {
				vt.push_back(ef->subterm(k)->clone());
				vc.push_back(ef->comp(k));
				vb.push_back(ef->compsign(k));
			}
			if(!vt.empty()) {
				vt.push_back(ef->subterm(splits[m])->clone());
				conjuncts.push_back(new EqChainForm(true,ef->conj(),vt,vc,vb,FormParseInfo()));
			}
			prevsplit = splits[m] + 1;
		}
		Formula* r = 0;
		if(!conjuncts.empty()) {
			if(conjuncts.size() == 1) {
				r = conjuncts[0];
				if(!ef->sign()) r->swapsign();
			}
			else {
				r = new BoolForm(ef->sign(),ef->conj(),conjuncts,FormParseInfo());
			}
		}
		for(unsigned int m = 0; m < ef->nrSubterms(); ++m) ef->subterm(m)->recursiveDelete();
		delete(ef);
		return r;
	}
	else return ef;
}

/** Move functions **/

class FuncGrapher : public MutatingVisitor {
	public:
		FuncGrapher() { }
		FuncGrapher(AbstractTheory* t) : MutatingVisitor() { t->accept(this);	}
		Formula*	visit(PredForm* pf);
		Formula*	visit(EqChainForm* ef);
};

Formula* FuncGrapher::visit(PredForm* pf) {
	if(pf->symb()->name() == "=/2") {
		if(typeid(*(pf->subterm(0))) == typeid(FuncTerm)) {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterm(0));
			vector<Term*> vt;
			for(unsigned int n = 0; n < ft->nrSubterms(); ++n) vt.push_back(ft->subterm(n));
			vt.push_back(pf->subterm(1));
			pf = new PredForm(pf->sign(),ft->func(),vt,FormParseInfo());
			delete(ft);
		}
		else if(typeid(*(pf->subterm(1))) == typeid(FuncTerm)) {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterm(1));
			vector<Term*> vt;
			for(unsigned int n = 0; n < ft->nrSubterms(); ++n) vt.push_back(ft->subterm(n));
			vt.push_back(pf->subterm(0));
			pf = new PredForm(pf->sign(),ft->func(),vt,FormParseInfo());
			delete(ft);
		}
	}
	return MutatingVisitor::visit(pf);
}

Formula* FuncGrapher::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	return nf;
}

class FuncMover : public MutatingVisitor {
	private:
		bool				_poscontext;	// true iff we are in a positive context
		vector<Formula*>	_funcgraphs;
		vector<Variable*>	_newvars;
		set<Function*>		_dontmove;		// don't move the functions in this set
	public:
		FuncMover(AbstractTheory* t) : MutatingVisitor() { t->accept(this);	}
		Theory*		visit(Theory* t);
		Formula*	visit(QuantForm* qf);
		Formula*	visit(BoolForm* bf);
		Formula*	visit(EquivForm* ef);
		Formula*	visit(EqChainForm* ef);
		Formula*	visit(PredForm* pf);
		SetExpr*	visit(EnumSetExpr*);
		SetExpr*	visit(QuantSetExpr*);
		Term*		visit(FuncTerm*);
};

Term* FuncMover::visit(FuncTerm* ft) {
	Term* t = MutatingVisitor::visit(ft);
	Function* f = ft->func();
	if((f->partial() || !f->builtin()) && _dontmove.find(f) == _dontmove.end()) {
		Variable* v = new Variable(f->outsort());
		VarTerm* vt = new VarTerm(v,ParseInfo());
		vector<Term*> args;
		for(unsigned int n = 0; n < f->arity(); ++n) args.push_back(t->subterm(n));
		args.push_back(vt);
		PredForm* pf = new PredForm(true,f,args,FormParseInfo());
		_funcgraphs.push_back(pf);
		_newvars.push_back(v);
		delete(ft);
		return vt->clone();
	}
	else return t;
}

SetExpr* FuncMover::visit(EnumSetExpr* ese) {
	bool save = _poscontext;
	_poscontext = true;
	SetExpr* e = MutatingVisitor::visit(ese);
	_poscontext = save;
	return e;
}

SetExpr* FuncMover::visit(QuantSetExpr* qse) {
	bool save = _poscontext;
	_poscontext = true;
	SetExpr* e = MutatingVisitor::visit(qse);
	_poscontext = save;
	return e;
}

Formula* FuncMover::visit(QuantForm* qf) {
	bool save = _poscontext;
	if(!qf->sign()) _poscontext = !_poscontext;
	Formula* f =  MutatingVisitor::visit(qf);
	_poscontext = save;
	return f;
}

Formula* FuncMover::visit(BoolForm* qf) {
	bool save = _poscontext;
	if(!qf->sign()) _poscontext = !_poscontext;
	Formula* f = MutatingVisitor::visit(qf);
	_poscontext = save;
	return f;
}

Formula* FuncMover::visit(EquivForm* qf) {
	// TODO: check if the formula contains partial functions. 
	//		 If yes: rewrite the equivalence first to get the right contexts
	bool save = _poscontext;
	if(!qf->sign()) _poscontext = !_poscontext;
	Formula* f = MutatingVisitor::visit(qf);
	_poscontext = save;
	return f;
}

Formula* FuncMover::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	return nf;
}

Formula* FuncMover::visit(PredForm* pf) {
	bool save = _poscontext;
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		Term* nt = pf->subterm(n)->accept(this);
		pf->arg(n,nt);
	}
	_poscontext = save;
	if(_funcgraphs.empty()) return pf;
	else {
		if(_poscontext) {
			for(unsigned int n = 0; n < _funcgraphs.size(); ++n)
				_funcgraphs[n]->swapsign();
		}
		_funcgraphs.push_back(pf->clone());
		for(unsigned int n = 0; n < pf->nrSubterms(); ++n) pf->subterm(n)->recursiveDelete();
		BoolForm* bf = new BoolForm(true,!_poscontext,_funcgraphs,FormParseInfo());
		QuantForm* qf = new QuantForm(true,_poscontext,_newvars,bf,FormParseInfo());
		_funcgraphs.clear();
		_newvars.clear();
		Formula* f = visit(qf);
		_poscontext = save;
		delete(pf);
		return f;
	}
}


Theory* FuncMover::visit(Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_poscontext = true;
		Formula* f = t->sentence(n)->accept(this);
		vector<Variable*> vv;
		for(unsigned int m = 0; m < f->nrFvars(); ++m) {
			vv.push_back(f->fvar(m));
		}
		if(!vv.empty()) {
			f = new QuantForm(true,true,vv,f,f->pi());
		}
		delete(t->sentence(n));
		t->sentence(n,f);
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_poscontext = false;
		Definition* d = t->definition(n)->accept(this);
		t->definition(n,d);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_poscontext = false;	// TODO: maybe not correct?
		FixpDef* fd = t->fixpdef(n)->accept(this);
		t->fixpdef(n,fd);
	}
	TheoryUtils::flatten(t);
	return t;
}

/** Move aggregates **/

class AggGrapher : public MutatingVisitor {
	public:
		AggGrapher(AbstractTheory* t) : MutatingVisitor() { t->accept(this);	}
		Formula* visit(PredForm* pf);
		Formula* visit(EqChainForm* ef);
};

Formula* AggGrapher::visit(PredForm* pf) {
	string pfs = pf->symb()->name();
	if(pfs == "=/2" || pfs == "</2" || pfs == ">/2") {
		if(typeid(*(pf->subterm(0))) == typeid(AggTerm)) {
			AggTerm* at = dynamic_cast<AggTerm*>(pf->subterm(0));
			char c = '=';
			if(pfs == "</2") c = '>';
			else if(pfs == ">/2") c = '<'; 
			AggForm* af = new AggForm(pf->sign(),c,pf->subterm(1),at,FormParseInfo());
			delete(pf);
			return af;
		}
		else if(typeid(*(pf->subterm(1))) == typeid(AggTerm)) {
			AggTerm* at = dynamic_cast<AggTerm*>(pf->subterm(1));
			char c = '=';
			if(pfs == ">/2") c = '>';
			else if(pfs == "</2") c = '<'; 
			AggForm* af = new AggForm(pf->sign(),c,pf->subterm(0),at,FormParseInfo());
			delete(pf);
			return af;
		}
		else return pf;
	}
	else return pf;
}

Formula* AggGrapher::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	return nf;
}

class AggMover : public MutatingVisitor {
	private:
		vector<AggTerm*>	_aggs;
		vector<Variable*>	_newvars;
	public:
		AggMover(AbstractTheory* t) : MutatingVisitor() { t->accept(this);	}
		Formula* visit(PredForm*);
		Formula* visit(EqChainForm*);
};

Formula* AggMover::visit(PredForm* pf) {
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		Term* nt = pf->subterm(n)->accept(this);
		pf->arg(n,nt);
	}
	if(_aggs.empty()) return pf;
	else {
		vector<Formula*> vf;
		for(unsigned int n = 0; n < _aggs.size(); ++n) {
			vector<Term*> vt(2); 
			Term* l = new VarTerm(_newvars[n],ParseInfo());
			AggTerm* r = _aggs[n];
			AggForm* af = new AggForm(false,'=',l,r,FormParseInfo());
			vf.push_back(af);
		}
		vf.push_back(pf->clone());
		for(unsigned int n = 0; n < pf->nrSubterms(); ++n) pf->subterm(n)->recursiveDelete();
		BoolForm* bf = new BoolForm(true,false,vf,FormParseInfo());
		QuantForm* qf = new QuantForm(true,true,_newvars,bf,FormParseInfo());
		_aggs.clear();
		_newvars.clear();
		Formula* f = MutatingVisitor::visit(qf);
		delete(pf);
		return f;
	}
}

Formula* AggMover::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	return nf;
}

class ThreeValTermMover : public MutatingVisitor {
	private:
		AbstractStructure*			_structure;
		bool						_poscontext;
		bool						_cpcontext;
		const set<const Function*>	_cpfunctions;
		vector<Formula*>			_termgraphs;
		vector<Variable*>			_variables;
		bool						_istoplevelterm;
		bool isCPFunction(const Function* func) const { return _cpfunctions.find(func) != _cpfunctions.end(); }
	public:
		ThreeValTermMover(AbstractStructure* str, bool posc, bool cpc=false, const set<const Function*>& cpfuncs=set<const Function*>()):
			_structure(str), _poscontext(posc), _cpcontext(cpc), _cpfunctions(cpfuncs) { }
		Formula*	visit(PredForm* pf);
		Formula*	visit(AggForm* af);
		Term*		visit(FuncTerm* ft);
		Term*		visit(AggTerm*	at);
};

Term* ThreeValTermMover::visit(FuncTerm* functerm) {
	// Get the function and its interpretation
	Function* func = functerm->func();
	FuncInter* funcinter = _structure->inter(func);

	if(funcinter->fasttwovalued() || (_cpcontext && _istoplevelterm && isCPFunction(func))) {
		// The function is two-valued or we want to pass it to the constraint solver. Leave as is, just visit its children.
		for(unsigned int n = 0; n < functerm->nrSubterms(); ++n) {
			_istoplevelterm = false;
			Term* nterm = functerm->subterm(n)->accept(this);
			functerm->arg(n,nterm);
		}
		functerm->setfvars();
		return functerm;
	}
//	else if(_cpcontext && _istoplevelterm && (func->name() == "+/2")) {
//		for(unsigned int n = 0; functerm->nrSubterms(); ++n) {
//			if(typeid(*(functerm->subterm(n))) == typeid(FuncTerm*)) {
//				Functerm* subterm = dynamic_cast<FuncTerm*>(functerm->subterm(n));
//				if(subterm->func()->name() != "+/2") _istoplevelterm = false;
//			}
//			else _istoplevelterm = false;
//			Term* nterm = functerm->subterm(n)->accept(this);
//			functerm->arg(n,nterm);
//		}
//		functerm->setfvars();
//		return functerm;
//	}
	else {
		// The function is three-valued. Move it: create a new variable and an equation.
		Variable* var = new Variable(func->outsort());
		VarTerm* varterm = new VarTerm(var,ParseInfo());
		vector<Term*> args;
		for(unsigned int n = 0; n < func->arity(); ++n)
			args.push_back(functerm->subterm(n));
		args.push_back(varterm);
		PredForm* predform = new PredForm(true,func,args,FormParseInfo());
		_termgraphs.push_back(predform);
		_variables.push_back(var);
		delete(functerm);
		return varterm->clone();
	}
}

Term* ThreeValTermMover::visit(AggTerm* at) {
	bool twovalued = SetUtils::isTwoValued(at->set(),_structure);
	if(twovalued || (_cpcontext && _istoplevelterm)) return at;
	else {
		Variable* v = new Variable(at->sort());
		VarTerm* vt = new VarTerm(v,ParseInfo());
		AggTerm* newat = new AggTerm(at->set(),at->type(),ParseInfo());
		AggForm* af = new AggForm(true,'=',vt,newat,FormParseInfo());
		_termgraphs.push_back(af);
		_variables.push_back(v);
		delete(at);
		return vt->clone();
	}
};

Formula* ThreeValTermMover::visit(PredForm* pf) {
	// Handle built-in predicates
	string symbname = pf->symb()->name();
	if(! _cpcontext) {
		if(symbname == "=/2") {
			Term* left = pf->subterm(0);
			Term* right = pf->subterm(1);
			if(typeid(*left) == typeid(FuncTerm)) {
				FuncTerm* ft = dynamic_cast<FuncTerm*>(left);
				if(!_structure->inter(ft->func())->fasttwovalued()) { 
					Formula* newpf = FormulaUtils::graph_functions(pf);
					return newpf->accept(this);
				}
			}
			else if(typeid(*right) == typeid(FuncTerm)) {
				FuncTerm* ft = dynamic_cast<FuncTerm*>(right);
				if(!_structure->inter(ft->func())->fasttwovalued()) { 
					Formula* newpf = FormulaUtils::graph_functions(pf);
					return newpf->accept(this);
				}
			}
			else if(typeid(*left) == typeid(AggTerm)) { //TODO: merge with cases for < and >
				AggTerm* agt = dynamic_cast<AggTerm*>(left);
				AggForm* af = new AggForm(pf->sign(),'=',right,agt,FormParseInfo());
				delete(pf);
				return af->accept(this);
			}
			else if(typeid(*right) == typeid(AggTerm)) { //TODO: merge with cases for < and >
				AggTerm* agt = dynamic_cast<AggTerm*>(right);
				AggForm* af = new AggForm(pf->sign(),'=',left,agt,FormParseInfo());
				delete(pf);
				return af->accept(this);
			}
		}
		else if(symbname == "</2" || symbname == ">/2") {
			//TODO: Check whether handled correctly when both sides are AggTerms!!
			Term* left = pf->subterm(0);
			Term* right = pf->subterm(1);
			char c;
			if(typeid(*left) == typeid(AggTerm)) {
				AggTerm* agt = dynamic_cast<AggTerm*>(left);
				c = (symbname == "</2") ? '>' : '<';
				AggForm* af = new AggForm(pf->sign(),c,right,agt,FormParseInfo());
				delete(pf);
				return af->accept(this);
			}
			else if(typeid(*right) == typeid(AggTerm)) {
				AggTerm* agt = dynamic_cast<AggTerm*>(right);
				c = (symbname == "</2") ? '<' : '>';
				AggForm* af = new AggForm(pf->sign(),c,left,agt,FormParseInfo());
				delete(pf);
				return af->accept(this);
			}
		}
	}
	// Visit the subterms
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		_istoplevelterm = (pf->symb()->ispred() || n == pf->nrSubterms()-1);
		Term* nt = pf->subterm(n)->accept(this);
		pf->arg(n,nt);
	}

	if(_termgraphs.empty()) {	// No rewriting was needed, simply return the given atom
		return pf;
	}
	else {	// Rewriting was needed
		
		// In a positive context, the equations are negated
		if(_poscontext) {
			for(unsigned int n = 0; n < _termgraphs.size(); ++n)
				_termgraphs[n]->swapsign();
		}

		// Memory management for the original atom
		PredForm* npf = new PredForm(pf->sign(),pf->symb(),pf->args(),FormParseInfo());
		_termgraphs.push_back(npf);

		// Create and return the rewriting
		BoolForm* bf = new BoolForm(true,!_poscontext,_termgraphs,FormParseInfo());
		QuantForm* qf = new QuantForm(true,_poscontext,_variables,bf,FormParseInfo());
		delete(pf);
		return qf;
	}
}

Formula* ThreeValTermMover::visit(AggForm* af) {
	_istoplevelterm = true;
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
		BoolForm* bf = new BoolForm(true,!_poscontext,_termgraphs,FormParseInfo());
		QuantForm* qf = new QuantForm(true,_poscontext,_variables,bf,FormParseInfo());
		return qf;
	}
}

/** Formula utils **/

namespace FormulaUtils {
	TruthValue evaluate(Formula* f, AbstractStructure* s, const map<Variable*,TypedElement>& m) {
		FormulaEvaluator fe(f,s,m);
		return fe.returnvalue();
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
	 * Formula* moveThreeValTerms(PredForm* pf, AbstractStructure* str, bool poscontext)
	 * DESCRIPTION
	 *		Non-recursively moves terms that are three-valued according to a given structure
	 *		outside a given atom. The applied rewriting depends on the given context:
	 *			positive context:
	 *				P(t) becomes	! x : t = x => P(x).
	 *			negative context:
	 *				P(t) becomes	? x : t = x & P(x).
	 *		The fact that the rewriting is non-recursive means that in the above example, term t 
	 *		can still contain terms that are three-valued according to the structure.
	 *
	 * PARAMETERS
	 *		pf			- the given atom
	 *		str			- the given structure
	 *		poscontext	- true iff we are in a positive context
	 * POSTCONDITIONS
	 *		In the rewriting, the atom does not contain any three-valued terms anymore
	 * RETURNS
	 *		The rewritten formula. If no rewriting was needed, it is the same pointer as pf.
	 *		If rewriting was needed, pf can be deleted, but not recursively.
	 *		
	 */
	Formula* moveThreeValTerms(Formula* f, AbstractStructure* str, bool poscontext, bool usingcp, const set<const Function*> cpfunctions) {
		ThreeValTermMover tvtm(str,poscontext,usingcp,cpfunctions);
		Formula* rewriting = f->accept(&tvtm);
		return rewriting;
	}

	bool monotone(const AggForm* af) {
		switch(af->comp()) {
			case '=' : return false;
			case '<' : {
				switch(af->right()->type()) {
					case AGGCARD : case AGGMAX: return af->sign();
					case AGGMIN : return !af->sign();
					case AGGSUM : return af->sign(); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AGGPROD : return af->sign();//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			case '>' : { 
				switch(af->right()->type()) {
					case AGGCARD : case AGGMAX: return !af->sign();
					case AGGMIN : return af->sign();
					case AGGSUM : return !af->sign(); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AGGPROD : return !af->sign();//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			default : assert(false);
		}
		return false;
	}

	bool antimonotone(const AggForm* af) {
		switch(af->comp()) {
			case '=' : return false;
			case '<' : { 
				switch(af->right()->type()) {
					case AGGCARD : case AGGMAX: return !af->sign();
					case AGGMIN : return af->sign();
					case AGGSUM : return !af->sign(); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AGGPROD : return !af->sign();//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			case '>' : {
				switch(af->right()->type()) {
					case AGGCARD : case AGGMAX: return af->sign();
					case AGGMIN : return !af->sign();
					case AGGSUM : return af->sign(); //FIXME: Asserts that weights are positive! Not correct otherwise.
					case AGGPROD : return af->sign();//FIXME: Asserts that weights are larger than one! Not correct otherwise.
				}
				break;
			}
			default : assert(false);
		}
		return false;
	}
}

/** Theory utils **/

TheoryComponent* AbstractTheory::component(unsigned int n) const {
	if(n < nrSentences()) return sentence(n);
	else if(n < nrSentences() + nrDefinitions()) {
		return definition(n - nrSentences());
	}
	else return fixpdef(n - nrSentences() - nrDefinitions());
}

namespace TheoryUtils {
	
	/** Rewriting theories **/
	void push_negations(AbstractTheory* t)		{ NegationPush np(t);	}
	void remove_equiv(AbstractTheory* t)		{ EquivRemover er(t);	}
	void flatten(AbstractTheory* t)				{ Flattener f(t);		}
	void remove_eqchains(AbstractTheory* t)		{ EqChainRemover er(t);	}
	void move_quantifiers(AbstractTheory* t)	{ QuantMover qm(t);		}
	void move_functions(AbstractTheory* t)		{ FuncGrapher fg(t); 
												  FuncMover fm(t);		}

	/** Tseitin transformation **/
	void tseitin(AbstractTheory* t)	{ Tseitinizer tz(t); } 

	/** Simplification **/
	void reduce(AbstractTheory* t, AbstractStructure* s)	{ Reducer sf(t,s);	}
	
}
