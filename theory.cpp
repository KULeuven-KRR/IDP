#include "common.hpp"
#include <sstream>
#include <iostream>
#include <typeinfo>
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "error.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"

#include "theoryinformation/CollectOpensOfDefinitions.hpp"
#include "theoryinformation/CheckContainment.hpp"

#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

#include "utils/TheoryUtils.hpp"

using namespace std;

/**************
	Formula
**************/

IMPLACCEPTBOTH(AggForm, Formula)
IMPLACCEPTBOTH(BoolForm, Formula)
IMPLACCEPTBOTH(EqChainForm, Formula)
IMPLACCEPTBOTH(EquivForm, Formula)
IMPLACCEPTBOTH(QuantForm, Formula)
IMPLACCEPTBOTH(PredForm, Formula)
IMPLACCEPTBOTH(Definition, Definition)
IMPLACCEPTBOTH(FixpDef, FixpDef)
IMPLACCEPTBOTH(Rule, Rule)
IMPLACCEPTBOTH(Theory, AbstractTheory)

void Formula::setFreeVars() {
	_freevars.clear();
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(), (*it)->freeVars().cend());
	}
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(), (*it)->freeVars().cend());
	}
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		_freevars.erase(*it);
	}
}

void Formula::recursiveDelete() {
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		delete (*it);
	}
	delete (this);
}

bool Formula::contains(const Variable* v) const {
	for (auto it = _freevars.cbegin(); it != _freevars.cend(); ++it) {
		if (*it == v) { return true; }
	}
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		if (*it == v) { return true; }
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		if ((*it)->contains(v)) { return true; }
	}
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		if ((*it)->contains(v)) { return true; }
	}
	return false;
}

bool Formula::contains(const PFSymbol* s) const {
	return FormulaUtils::containsSymbol(s, this);
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
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

PredForm*	PredForm::cloneKeepVars() const{
	vector<Term*> na;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		na.push_back((*it)->cloneKeepVars());
	}
	PredForm* pf = new PredForm(sign(), _symbol, na, pi().clone());
	return pf;
}

PredForm* PredForm::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Term*> na;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		na.push_back((*it)->clone(mvv));
	}
	PredForm* pf = new PredForm(sign(), _symbol, na, pi().clone(mvv));
	return pf;
}

ostream& PredForm::put(ostream& output) const {
	if (isNeg(sign())) {
		output << '~';
	}
	_symbol->put(output);
	if (typeid(*_symbol) == typeid(Predicate)) {
		if (not subterms().empty()) {
			output << '(';
			for (size_t n = 0; n < subterms().size(); ++n) {
				subterms()[n]->put(output);
				if (n < subterms().size() - 1) {
					output << ',';
				}
			}
			output << ')';
		}
	} else {
		Assert(typeid(*_symbol) == typeid(Function));
		if (subterms().size() > 1) {
			output << '(';
			for (size_t n = 0; n < subterms().size() - 1; ++n) {
				subterms()[n]->put(output);
				if (n + 1 < subterms().size() - 1) {
					output << ',';
				}
			}
			output << ')';
		}
		output << " = ";
		subterms().back()->put(output);
	}
	return output;
}

/******************
	EqChainForm
******************/

EqChainForm* EqChainForm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

EqChainForm*	EqChainForm::cloneKeepVars() const{
	vector<Term*> nt;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		nt.push_back((*it)->cloneKeepVars());
	}
	EqChainForm* ef = new EqChainForm(sign(), _conj, nt, _comps, pi().clone());
	return ef;
}

EqChainForm* EqChainForm::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Term*> nt;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		nt.push_back((*it)->clone(mvv));
	}
	EqChainForm* ef = new EqChainForm(sign(), _conj, nt, _comps, pi().clone(mvv));
	return ef;
}

ostream& EqChainForm::put(ostream& output) const {
	if (isNeg(sign())) { output << '~'; }
	output << '(';
	subterms()[0]->put(output);
	for (size_t n = 0; n < _comps.size(); ++n) {
		output << ' ' << comps()[n] << ' ';
		subterms()[n + 1]->put(output);
		if (not _conj && n + 1 < _comps.size()) {
			output << " | ";
			subterms()[n + 1]->put(output);
		}
	}
	output << ')';
	return output;
}

/****************
	EquivForm
****************/

EquivForm* EquivForm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

EquivForm*	EquivForm::cloneKeepVars() const{
	Formula* nl = left()->cloneKeepVars();
	Formula* nr = right()->cloneKeepVars();
	EquivForm* ef = new EquivForm(sign(), nl, nr, pi().clone());
	return ef;
}

EquivForm* EquivForm::clone(const map<Variable*, Variable*>& mvv) const {
	Formula* nl = left()->clone(mvv);
	Formula* nr = right()->clone(mvv);
	EquivForm* ef = new EquivForm(sign(), nl, nr, pi().clone(mvv));
	return ef;
}

ostream& EquivForm::put(ostream& output) const {
	output << '(';
	left()->put(output);
	output << " <=> ";
	right()->put(output);
	output << ')';
	return output;
}

/***************
	BoolForm
***************/

BoolForm* BoolForm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

BoolForm*	BoolForm::cloneKeepVars() const{
	vector<Formula*> ns;
	for (auto it = subformulas().cbegin(); it != subformulas().cend(); ++it) {
		ns.push_back((*it)->cloneKeepVars());
	}
	BoolForm* bf = new BoolForm(sign(), _conj, ns, pi().clone());
	return bf;
}

BoolForm* BoolForm::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Formula*> ns;
	for (auto it = subformulas().cbegin(); it != subformulas().cend(); ++it) {
		ns.push_back((*it)->clone(mvv));
	}
	BoolForm* bf = new BoolForm(sign(), _conj, ns, pi().clone(mvv));
	return bf;
}

ostream& BoolForm::put(ostream& output) const {
	if (subformulas().empty()) {
		if (isConjWithSign()) {
			output << "true";
		} else {
			output << "false";
		}
	} else {
		if (isNeg(sign())) { output << '~'; }
		output << '(';
		for (size_t n = 0; n < subformulas().size(); ++n) {
			subformulas()[n]->put(output);
			if (n < subformulas().size() - 1) {
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
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

QuantForm*	QuantForm::cloneKeepVars() const{
	auto nf = subformula()->cloneKeepVars();
	return new QuantForm(sign(), quant(), quantVars(), nf, pi().clone());
}

QuantForm* QuantForm::clone(const map<Variable*, Variable*>& mvv) const {
	set<Variable*> nv;
	map<Variable*, Variable*> nmvv = mvv;
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		Variable* v = new Variable((*it)->name(), (*it)->sort(), pi());
		nv.insert(v);
		nmvv[*it] = v;
	}
	Formula* nf = subformula()->clone(nmvv);
	QuantForm* qf = new QuantForm(sign(), quant(), nv, nf, pi().clone(mvv));
	return qf;
}

ostream& QuantForm::put(ostream& output) const {
	if (isNeg(sign())) { output << '~'; }
	output << '(';
	output << (isUniv() ? '!' : '?');
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		output << ' ';
		(*it)->put(output);
	}
	output << " : ";
	subformula()->put(output);
	output << ')';
	return output;
}

/**************
	AggForm
**************/

AggForm::AggForm(SIGN sign, Term* l, CompType c, AggTerm* r, const FormulaParseInfo& pi) :
		Formula(sign, pi), _comp(c), _aggterm(r) {
	addSubterm(l);
	addSubterm(r);
}

AggForm* AggForm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

AggForm*	AggForm::cloneKeepVars() const{
	auto nl = left()->cloneKeepVars();
	auto nr = right()->cloneKeepVars();
	return new AggForm(sign(), nl, _comp, nr, pi().clone());
}

AggForm* AggForm::clone(const map<Variable*, Variable*>& mvv) const {
	Term* nl = left()->clone(mvv);
	AggTerm* nr = right()->clone(mvv);
	return new AggForm(sign(), nl, _comp, nr, pi().clone(mvv));
}

ostream& AggForm::put(ostream& output) const {
	if (isNeg(sign())) { output << '~'; }
	output << '(';
	left()->put(output);
	output << ' ' << _comp << ' ';
	right()->put(output);
	output << ')';
	return output;
}

/***********
	Rule
***********/

Rule* Rule::clone() const {
	map<Variable*, Variable*> mvv;
	set<Variable*> newqv;
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		Variable* v = new Variable((*it)->name(), (*it)->sort(), ParseInfo());
		mvv[*it] = v;
		newqv.insert(v);
	}
	return new Rule(newqv, _head->clone(mvv), _body->clone(mvv), _pi);
}

void Rule::recursiveDelete() {
	_head->recursiveDelete();
	_body->recursiveDelete();
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		delete (*it);
	}
	delete (this);
}

ostream& Rule::put(ostream& output) const {
	if (not _quantvars.empty()) {
		output << "!";
		for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
			output << ' ';
			(*it)->put(output);
		}
		output << " : ";
	}
	_head->put(output);
	output << " <- ";
	_body->put(output);
	output << '.';
	return output;
}

ostream& operator<<(ostream& output, const Rule& r) {
	return r.put(output);
}

/******************
	Definitions
******************/

Definition* Definition::clone() const {
	Definition* newdef = new Definition();
	for (auto it = _rules.cbegin(); it != _rules.cend(); ++it) {
		newdef->add((*it)->clone());
	}
	return newdef;
}

void Definition::recursiveDelete() {
	for (size_t n = 0; n < _rules.size(); ++n) {
		_rules[n]->recursiveDelete();
	}
	delete (this);
}

void Definition::add(Rule* r) {
	_rules.push_back(r);
	_defsyms.insert(r->head()->symbol());
}

void Definition::rule(unsigned int n, Rule* r) {
	_rules[n] = r;
	_defsyms.clear();
	for (auto it = _rules.cbegin(); it != _rules.cend(); ++it) {
		_defsyms.insert((*it)->head()->symbol());
	}
}

ostream& Definition::put(ostream& output) const {
	output << "{ ";
	if (not _rules.empty()) {
		_rules[0]->put(output);
		pushtab();
		for (size_t n = 1; n < _rules.size(); ++n) {
			output << '\n';
			output << tabs();
			_rules[n]->put(output);
		}
		poptab();
	}
	output << '}';
	return output;
}

/***************************
	Fixpoint definitions
***************************/

FixpDef* FixpDef::clone() const {
	FixpDef* newfd = new FixpDef(_lfp);
	for (auto it = _defs.cbegin(); it != _defs.cend(); ++it) {
		newfd->add((*it)->clone());
	}
	for (auto it = _rules.cbegin(); it != _rules.cend(); ++it) {
		newfd->add((*it)->clone());
	}
	return newfd;
}

void FixpDef::recursiveDelete() {
	for (auto it = _defs.cbegin(); it != _defs.cend(); ++it) {
		delete (*it);
	}
	for (auto it = _rules.cbegin(); it != _rules.cend(); ++it) {
		delete (*it);
	}
	delete (this);
}

void FixpDef::add(Rule* r) {
	_rules.push_back(r);
	_defsyms.insert(r->head()->symbol());
}

void FixpDef::rule(unsigned int n, Rule* r) {
	_rules[n] = r;
	_defsyms.clear();
	for (auto it = _rules.cbegin(); it != _rules.cend(); ++it) {
		_defsyms.insert((*it)->head()->symbol());
	}
}

ostream& FixpDef::put(ostream& output) const {
	output << (_lfp ? "LFD [  " : "GFD [  ");
	if (not _rules.empty()) {
		_rules[0]->put(output);
		pushtab();
		for (size_t n = 1; n < _rules.size(); ++n) {
			output << '\n'<<tabs();
			_rules[n]->put(output);
		}
		poptab();
	}
	pushtab();
	for (auto it = _defs.cbegin(); it != _defs.cend(); ++it) {
		output << '\n' << tabs();
		(*it)->put(output);
	}
	poptab();
	output << " ]";
	return output;
}

/***************
	Theories
***************/

Theory* Theory::clone() const {
	Theory* newtheory = new Theory(_name, _vocabulary, ParseInfo());
	for (auto it = _sentences.cbegin(); it != _sentences.cend(); ++it) {
		newtheory->add((*it)->clone());
	}
	for (auto it = _definitions.cbegin(); it != _definitions.cend(); ++it) {
		newtheory->add((*it)->clone());
	}
	for (auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it) {
		newtheory->add((*it)->clone());
	}
	return newtheory;
}

void Theory::addTheory(AbstractTheory*) {
	// TODO
}

void Theory::recursiveDelete() {
	for (auto it = _sentences.cbegin(); it != _sentences.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for (auto it = _definitions.cbegin(); it != _definitions.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for (auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	delete (this);
}

set<TheoryComponent*> Theory::components() const {
	set<TheoryComponent*> stc;
	for (auto it = _sentences.cbegin(); it != _sentences.cend(); ++it) {
		stc.insert(*it);
	}
	for (auto it = _definitions.cbegin(); it != _definitions.cend(); ++it) {
		stc.insert(*it);
	}
	for (auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it) {
		stc.insert(*it);
	}
	return stc;
}

void Theory::remove(Definition* d) {
	auto it = _definitions.begin();
	for (; it != _definitions.end(); ++it) {
		if (*it == d) { break; }
	}
	if (it != _definitions.end()) {
		_definitions.erase(it);
	}
}

std::ostream& Theory::put(std::ostream& output) const {
	output << "theory " << name();
	if (_vocabulary) {
		output << " : " << vocabulary()->name();
	}
	output << " {\n";
	pushtab();
	output <<tabs();
	for (auto it = _sentences.cbegin(); it != _sentences.cend(); ++it) {
		output <<toString(*it);
	}
	for (auto it = _definitions.cbegin(); it != _definitions.cend(); ++it) {
		output <<toString(*it);
	}
	for (auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it) {
		output <<toString(*it);
	}
	poptab();
	output << "}";
	return output;
}

