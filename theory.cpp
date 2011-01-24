/************************************
	theory.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "theory.hpp"
#include "visitor.hpp"
#include "builtin.hpp"
#include "ecnf.hpp"
#include "ground.hpp"
#include <iostream>
#include <set>

#include <typeinfo>

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

string BracketForm::to_string() const {
	string s;
	if(!_sign) s = s+ '~';
	s = s + "(" + _subf->to_string() + ")";
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

string AggForm::to_string() const {
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

		void visit(PredForm*);
		void visit(BoolForm*);
		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(QuantForm*);
		
};

void FormulaEvaluator::visit(PredForm* pf) {
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

void FormulaEvaluator::visit(EquivForm* ef) {
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

void FormulaEvaluator::visit(EqChainForm* ef) {
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

void FormulaEvaluator::visit(BoolForm* bf) {
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

void FormulaEvaluator::visit(QuantForm* qf) {
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

namespace FormulaUtils {
	TruthValue evaluate(Formula* f, AbstractStructure* s, const map<Variable*,TypedElement>& m) {
		FormulaEvaluator fe(f,s,m);
		return fe.returnvalue();
	}
}

/*************************
	Rewriting theories
*************************/

void Theory::add(AbstractTheory* t) {
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_definitions.push_back(t->definition(n));
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_fixpdefs.push_back(t->fixpdef(n));
	}
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_sentences.push_back(t->sentence(n));
	}

}

/** Push negations inside **/

class NegationPush : public Visitor {

	public:
		NegationPush(AbstractTheory* t)	: Visitor() { t->accept(this);	}

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

/** Rewrite A <=> B to (A => B) & (B => A) **/

class EquivRemover : public MutatingVisitor {

	public:
		EquivRemover(AbstractTheory* t)	: MutatingVisitor() { t->accept(this);	}

		BoolForm* visit(EquivForm*);

};

BoolForm* EquivRemover::visit(EquivForm* ef) {
	Formula* nl = ef->left()->accept(this);
	Formula* nr = ef->right()->accept(this);
	if(nl != ef->left()) delete(ef->left());
	if(nr != ef->right()) delete(ef->right());
	vector<Formula*> vf1(2);
	vector<Formula*> vf2(2);
	vf1[0] = nl; vf1[1] = nr;
	vf2[0] = nl->clone(); vf2[1] = nr->clone();
	vf1[0]->swapsign(); vf2[1]->swapsign();
	BoolForm* bf1 = new BoolForm(true,false,vf1,ef->pi());
	BoolForm* bf2 = new BoolForm(true,false,vf2,ef->pi());
	vector<Formula*> vf(2); vf[0] = bf1; vf[1] = bf2;
	BoolForm* bf = new BoolForm(ef->sign(),true,vf,ef->pi());
	return bf;
}

/** Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc. **/

class Flattener : public Visitor {

	public:
		Flattener(AbstractTheory* t) : Visitor() { t->accept(this);	}

		void visit(BoolForm*);
		void visit(QuantForm*);

};

void Flattener::visit(BoolForm* bf) {
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
}

void Flattener::visit(QuantForm* qf) {
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
}

/**  Rewrite chains of equalities to a conjunction or disjunction of atoms **/

class EqChainRemover : public MutatingVisitor {

	private:
		Vocabulary* _vocab;

	public:
		EqChainRemover(AbstractTheory* t)	: MutatingVisitor(), _vocab(t->vocabulary()) { t->accept(this);	}
		EqChainRemover()	: MutatingVisitor(), _vocab(0) {	}

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
		return vf[0];
	}
	else {
		BoolForm* bf = new BoolForm(ef->sign(),ef->conj(),vf,ef->pi());
		return bf;
	}
}

/** Convert to ecnf **/

enum ECNFCONVRTT { ECTT_ATOM, ECTT_CLAUSE, ECTT_CONJ, ECTT_AGG, ECTT_NOTHING };

class TheoryConvertor : public Visitor {
	
	private:
		EcnfTheory*			_returnvalue;		// The resulting ecnf theory

		// Intermediate return values
		int					_curratom;			// Returned atom
		EcnfClause			_currclause;		// Returned set of literals
		EcnfDefinition		_currdefinition;	// Returned definition
		EcnfFixpDef			_currfixpdef;		// Returned fixpoint definitionn

		// Intermediate return value is an aggregate expression
		double				_currbound;			// The bound in the returned aggregate expression
		bool				_lowerbound;		// True iff the returned bound is a lower bound
		unsigned int		_currset;			// The set in the returned aggregate expression
		AggType				_curragg;			// The aggregate in the returned aggregate expression

		ECNFCONVRTT			_rettype;			// Type of the intermediate return value
		bool				_infixpdef;			// True iff the visitor is inside a fixpoint definition
		bool				_indef;				// True iff the visitor is inside a (non-fixpoint) definition

	public:
		
		TheoryConvertor(AbstractTheory* t) : 
			Visitor(), _returnvalue(new EcnfTheory()) { t->accept(this); }

		void visit(PredForm*);
		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(BoolForm*);
		void visit(QuantForm*);
		void visit(Rule*);
		void visit(Theory*);
		void visit(QuantSetExpr*);
		void visit(EnumSetExpr*);

		EcnfTheory*	returnvalue()	const { return _returnvalue;	}

};

void TheoryConvertor::visit(Theory* t) {
	// sentences
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_indef = false; _infixpdef = false;
		t->sentence(n)->accept(this);
		switch(_rettype) {
			case ECTT_ATOM:
				_returnvalue->addClause(vector<int>(1,_curratom));
				break;
			case ECTT_CLAUSE:
				_returnvalue->addClause(_currclause);
				break;
			case ECTT_CONJ:
				for(unsigned int m = 0; m < _currclause.size(); ++m) {
					_returnvalue->addClause(vector<int>(1,_currclause[m]));
				}
			case ECTT_NOTHING:
				break;
			default:
				assert(false);
		}
	}
	// definitions
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_indef = true; _infixpdef = false;
		_currdefinition = EcnfDefinition();
		t->definition(n)->accept(this);
		_returnvalue->addDefinition(_currdefinition);
	}
	// fixpoint definitions
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_indef = false; _infixpdef = true;
		_currfixpdef = EcnfFixpDef();
		t->fixpdef(n)->accept(this);
		_returnvalue->addFixpDef(_currfixpdef);
	}
}

void TheoryConvertor::visit(PredForm* pf) {
	if(pf->symb()->builtin()) {	// An aggregate literal
		assert(pf->symb()->name() == "</2" || pf->symb()->name() == ">/2");
		assert(!pf->sign());
		assert(typeid(*(pf->subterm(0))) == typeid(AggTerm));
		assert(typeid(*(pf->subterm(1))) == typeid(DomainTerm));
		AggTerm* agt = dynamic_cast<AggTerm*>(pf->subterm(0));
		DomainTerm* dt = dynamic_cast<DomainTerm*>(pf->subterm(1));
		agt->set()->accept(this);
		_curragg = agt->type();
		_lowerbound = (pf->symb()->name() == ">/2");
		_currbound = (dt->type() == ELDOUBLE ? dt->value()._double: double(dt->value()._int));	
		_rettype = ECTT_AGG;
	}
	else {	// A normal literal
		vector<TypedElement> args(pf->symb()->nrsorts());
		for(unsigned int n = 0; n < pf->symb()->nrsorts(); ++n) {
			assert(typeid(*(pf->subterm(n))) == typeid(DomainTerm));
			DomainTerm* dt = dynamic_cast<DomainTerm*>(pf->subterm(n));
			args[n]._element = dt->value();
			args[n]._type = dt->type();
		}
		_curratom = _returnvalue->translator()->translate(pf->symb(),args);
		if(!pf->sign()) _curratom = -_curratom;
		_rettype = ECTT_ATOM;
	}
}

void TheoryConvertor::visit(EqChainForm*) {
	assert(false);
}

void TheoryConvertor::visit(EquivForm* ef) {
	assert(ef->sign());

	ef->left()->accept(this);
	assert(_rettype == ECTT_ATOM);
	int lhs = _curratom;

	ef->right()->accept(this);
	assert(_rettype == ECTT_AGG);

	_returnvalue->addAgg(EcnfAgg(_curragg,_lowerbound,EHA_EQUIV,lhs,_currset,_currbound));
	_rettype = ECTT_NOTHING;
}

void TheoryConvertor::visit(BoolForm* bf) {
	assert(bf->sign());
	vector<int> literals;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		bf->subform(n)->accept(this);
		if(n == 1 && _rettype == ECTT_AGG) {
			assert(bf->nrSubforms() == 2);
			_returnvalue->addAgg(EcnfAgg(_curragg,_lowerbound,EHA_IMPLIES,-literals[0],_currset,_currbound));
			_rettype = ECTT_NOTHING;
			return;
		}
		else literals.push_back(_curratom);
		
	}
	_currclause = literals;
	_rettype = (bf->conj() ? ECTT_CONJ : ECTT_CLAUSE);
}

void TheoryConvertor::visit(QuantForm*) {
	assert(false);
}

void TheoryConvertor::visit(Rule* r) {
	r->head()->accept(this);
	int headatom = _curratom;
	r->body()->accept(this);
	if(_rettype == ECTT_AGG) {
		EcnfAgg efa(_curragg,_lowerbound,EHA_DEFINED,headatom,_currset,_currbound);
		if(_infixpdef) _currfixpdef.addAgg(efa,_returnvalue->translator());
		else _currdefinition.addAgg(efa,_returnvalue->translator());
	}
	else {
		if(_rettype == ECTT_ATOM) {
			_currclause = vector<int>(1,_curratom);
			_rettype = ECTT_CONJ;
		}
		if(_infixpdef) _currfixpdef.addRule(headatom,_currclause,_rettype==ECTT_CONJ,_returnvalue->translator());
		else _currdefinition.addRule(headatom,_currclause,_rettype==ECTT_CONJ,_returnvalue->translator());
	}
}

void TheoryConvertor::visit(QuantSetExpr*) {
	assert(false);
}

void TheoryConvertor::visit(EnumSetExpr* e) {
	vector<int> literals;
	vector<double> weights;
	for(unsigned int n = 0; n < e->nrSubforms(); ++n) {
		e->subform(n)->accept(this);
		assert(_rettype == ECTT_ATOM);
		literals.push_back(_curratom);
		assert(typeid(*(e->subterm(n))) == typeid(DomainTerm));
		DomainTerm* dt = dynamic_cast<DomainTerm*>(e->subterm(n));
		if(dt->type() == ELINT) weights.push_back(double((dt->value())._int));
		else weights.push_back((dt->value())._double);
	}
	_currset = _returnvalue->addSet(literals,weights);
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
		if(f != t->sentence(n)) {
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
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_maketseitin = false;
		_insiderule = true;
		Definition* d = t->definition(n)->accept(this);
		if(d != t->definition(n)) {
			delete(t->definition(n));
			t->definition(n,d);
		}
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_maketseitin = false;
		_insiderule = true;
		FixpDef* fd = t->fixpdef(n)->accept(this);
		if(fd != t->fixpdef(n)) {
			delete(t->fixpdef(n));
			t->fixpdef(n,fd);
		}
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
			return new PredForm(true,tspred,vt,FormParseInfo());
		}
		else {
			if(bf->conj()) {	// A conjunction at sentence level: split the conjunction
				if(bf->nrSubforms()) {
					for(unsigned int n = 1; n < bf->nrSubforms(); ++n) {
						_theory->add(bf->subform(n));
					}
					return bf->subform(0)->accept(this);
				}
				else return bf;
			}
			else {	// A disjunction at sentence level: go one level deeper
				for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
					_maketseitin = true;
					Formula* nf = bf->subform(n)->accept(this);
					if(nf != bf->subform(n)) {
						delete(bf->subform(n));
						bf->subf(n,nf);
					}
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
		if(f != t->sentence(n)) {
			if(f->trueformula()) {	// skip true formulas
				++move;
				f->recursiveDelete();
				delete(t->sentence(n));
			}
			else if(f->falseformula()) {	// reduce theory to 'false'
				for(unsigned int m = 0; m < n-move; ++m) t->sentence(m)->recursiveDelete();
				for(unsigned int m = n+1; m < t->nrSentences(); ++m) t->sentence(m)->recursiveDelete();
				for(unsigned int m = 0; m < t->nrDefinitions(); ++m) t->definition(m)->recursiveDelete();
				for(unsigned int m = 0; m < t->nrFixpDefs(); ++m) t->fixpdef(m)->recursiveDelete();
				delete(t->sentence(n));
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
				delete(t->sentence(n));
				t->sentence(n-move,f);
			}
		}
		else t->sentence(n-move,t->sentence(n));
	}
	for(unsigned int n = 0; n < move; ++n) t->pop_sentence();

	// Definitions
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		Definition* d = t->definition(n)->accept(this);
		if(d != t->definition(n)) {
			delete(t->definition(n));
			t->definition(n,d);
		}
	}

	// Fixpoint definitions
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		FixpDef* fd = t->fixpdef(n)->accept(this);
		if(fd != t->fixpdef(n)) {
			delete(t->fixpdef(n));
			t->fixpdef(n,fd);
		}
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
		return nf;
	}
	else if(vf.size() == 1) {
		if(!bf->sign()) vf[0]->swapsign();
		return vf[0];
	}
	else return new BoolForm(bf->sign(),bf->conj(),vf,FormParseInfo());
}

Formula* Reducer::visit(QuantForm* qf) {
	Formula* ns = qf->subf()->accept(this);
	if(ns != qf->subf()) {
		delete(qf->subf());
		qf->subf(ns);
	}
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
	if(nl != ef->left()) delete(ef->left());
	if(nr != ef->right()) delete(ef->right());

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
		if(nt != ef->subterm(n)) {
			delete(ef->subterm(n));
			ef->term(n,nt);
		}
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
		return r;
	}
	else return ef;
}

/** Move functions **/

class FuncGrapher : public MutatingVisitor {
	public:
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
	if(nf != f) delete(f);
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
	if(nf != f) delete(f);
	return nf;
}

Formula* FuncMover::visit(PredForm* pf) {
	bool save = _poscontext;
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		Term* nt = pf->subterm(n)->accept(this);
		if(nt != pf->subterm(n)) {
			delete(pf->subterm(n));
			pf->arg(n,nt);
		}
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
		if(f != qf) delete(qf);
		_poscontext = save;
		return f;
	}
}


Theory* FuncMover::visit(Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_poscontext = true;
		Formula* f = t->sentence(n)->accept(this);
		if(f != t->sentence(n)) {
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
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_poscontext = false;
		Definition* d = t->definition(n)->accept(this);
		if(d != t->definition(n)) {
			delete(t->definition(n));
			t->definition(n,d);
		}
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_poscontext = false;	// TODO: maybe not correct?
		FixpDef* fd = t->fixpdef(n)->accept(this);
		if(fd != t->fixpdef(n)) {
			delete(t->fixpdef(n));
			t->fixpdef(n,fd);
		}
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
			return af;
		}
		else if(typeid(*(pf->subterm(1))) == typeid(AggTerm)) {
			AggTerm* at = dynamic_cast<AggTerm*>(pf->subterm(1));
			char c = '=';
			if(pfs == ">/2") c = '>';
			else if(pfs == "</2") c = '<'; 
			AggForm* af = new AggForm(pf->sign(),c,pf->subterm(0),at,FormParseInfo());
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
	if(nf != f) delete(f);
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
		if(nt != pf->subterm(n)) {
			delete(pf->subterm(n));
			pf->arg(n,nt);
		}
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
		if(f != qf) delete(qf);
		return f;
	}
}

Formula* AggMover::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	if(nf != f) delete(f);
	return nf;
}

// TODO: HIER BEZIG: Move Aggregates in terms and rules
// TODO: functions moeten ook verplaatst worden uit de head van regels

/** Theory utils **/

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
	
	/** ECNF **/
	EcnfTheory*	convert_to_ecnf(AbstractTheory* t)	{ TheoryConvertor tc(t); return tc.returnvalue();	}

}
