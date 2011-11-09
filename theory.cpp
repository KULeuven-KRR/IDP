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
	for(auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(),(*it)->freeVars().cend());
	}
	for(auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(),(*it)->freeVars().cend());
	}
	for(auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		_freevars.erase(*it);
	}
}

void Formula::recursiveDelete() {
	for(auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for(auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for(auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		delete(*it);
	}
	delete(this);
}

bool Formula::contains(const Variable* v) const {
	for(auto it = _freevars.cbegin(); it != _freevars.cend(); ++it) {
		if(*it == v) return true;
	}
	for(auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		if(*it == v) return true;
	}
	for(auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		if((*it)->contains(v)) return true;
	}
	for(auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		if((*it)->contains(v)) return true;
	}
	return false;
}

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
	for(auto it = subterms().cbegin(); it != subterms().cend(); ++it) 
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
	for(auto it = subterms().cbegin(); it != subterms().cend(); ++it) { 
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
	for(auto it = subformulas().cbegin(); it != subformulas().cend(); ++it) 
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
	for(auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
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
	for(auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
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
	for(auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		newqv.insert(v);
	}
	return new Rule(newqv,_head->clone(mvv),_body->clone(mvv),_pi);
}

void Rule::recursiveDelete() {
	_head->recursiveDelete();
	_body->recursiveDelete();
	for(auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) delete(*it);
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
		for(auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
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
	for(auto it = _rules.cbegin(); it != _rules.cend(); ++it)
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
	for(auto it = _rules.cbegin(); it != _rules.cend(); ++it) 
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
	for(auto it = _defs.cbegin(); it != _defs.cend(); ++it) 
		newfd->add((*it)->clone());
	for(auto it = _rules.cbegin(); it != _rules.cend(); ++it) 
		newfd->add((*it)->clone());
	return newfd;
}

void FixpDef::recursiveDelete() {
	for(auto it = _defs.cbegin(); it != _defs.cend(); ++it) 
		delete(*it);
	for(auto it = _rules.cbegin(); it != _rules.cend(); ++it) 
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
	for(auto it = _rules.cbegin(); it != _rules.cend(); ++it) 
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
	for(auto it = _defs.cbegin(); it != _defs.cend(); ++it) {
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
	for(auto it = _sentences.cbegin(); it != _sentences.cend(); ++it)
		newtheory->add((*it)->clone());
	for(auto it = _definitions.cbegin(); it != _definitions.cend(); ++it)
		newtheory->add((*it)->clone());
	for(auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it)
		newtheory->add((*it)->clone());
	return newtheory;
}

void Theory::addTheory(AbstractTheory* ) {
	// TODO
}

void Theory::recursiveDelete() {
	for(auto it = _sentences.cbegin(); it != _sentences.cend(); ++it)
		(*it)->recursiveDelete();
	for(auto it = _definitions.cbegin(); it != _definitions.cend(); ++it)
		(*it)->recursiveDelete();
	for(auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it)
		(*it)->recursiveDelete();
	delete(this);
}

set<TheoryComponent*> Theory::components() const {
	set<TheoryComponent*> stc;
	for(auto it = _sentences.cbegin(); it != _sentences.cend(); ++it) stc.insert(*it);
	for(auto it = _definitions.cbegin(); it != _definitions.cend(); ++it) stc.insert(*it);
	for(auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it) stc.insert(*it);
	return stc;
}

void Theory::remove(Definition* d) {
	auto it = _definitions.begin();
	for(; it != _definitions.end(); ++it) {
		if(*it == d) break;
	}
	if(it != _definitions.end()){
		_definitions.erase(it);
	}
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
	for(auto it = _sentences.cbegin(); it != _sentences.cend(); ++it)
		(*it)->put(output,longnames,spaces+2);
	for(auto it = _definitions.cbegin(); it != _definitions.cend(); ++it)
		(*it)->put(output,longnames,spaces+2);
	for(auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it)
		(*it)->put(output,longnames,spaces+2);
	output << "}\n";
	return output;
}


/********************
	Formula utils
********************/

namespace FormulaUtils {

	BoolForm* trueFormula() {
		return new BoolForm(SIGN::POS,true,vector<Formula*>(0),FormulaParseInfo());
	}

	BoolForm* falseFormula() {
		return new BoolForm(SIGN::POS,false,vector<Formula*>(0),FormulaParseInfo());
	}

	double estimatedCostAll(PredForm* query, const std::set<Variable*> freevars, bool inverse, AbstractStructure* structure) {
		FOBDDManager manager;
		FOBDDFactory factory(&manager);
		const FOBDD* bdd = factory.run(query);
		if(inverse) { bdd = manager.negation(bdd); }
		set<const FOBDDDeBruijnIndex*> indices;
//cerr << "Estimating the cost of bdd\n";
//manager.put(cerr,bdd);
//cerr << "With variables ";
//for(auto it = freevars.cbegin(); it != freevars.cend(); ++it) cerr << *(*it) << ' ';
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


/*******************
	Theory utils
*******************/

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
			for(auto it = t2->sentences().cbegin(); it != t2->sentences().cend(); ++it) {
				at->add((*it)->clone());
			}
			for(auto it = t2->definitions().cbegin(); it != t2->definitions().cend(); ++it) {
				at->add((*it)->clone());
			}
			for(auto it = t2->fixpdefs().cbegin(); it != t2->fixpdefs().cend(); ++it) {
				at->add((*it)->clone());
			}
			return at;
		}
		else { return NULL; }
	}
}

/***********************
	Definition utils
***********************/

namespace DefinitionUtils {
	set<PFSymbol*> opens(Definition* d) { OpenCollector oc; return oc.run(d);	}
}

