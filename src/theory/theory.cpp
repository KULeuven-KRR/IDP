/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"

#include "information/CollectOpensOfDefinitions.hpp"
#include "information/CheckContainment.hpp"

#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

#include "printers/print.hpp"

#include "TheoryUtils.hpp"

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
IMPLACCEPTBOTH(Theory, Theory)

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
	if (not _allwaysDeleteRecursively) {
		deleteChildren(true);
	}
	delete (this);
}

void Formula::recursiveDeleteKeepVars() {
	if (not _allwaysDeleteRecursively) {
		deleteChildren(false);
	}
	delete (this);
}
Formula::~Formula() {
	if (_allwaysDeleteRecursively) {
		deleteChildren(true);
	}
}

void Formula::deleteChildren(bool deleteVars) {
	if (deleteVars) {
		for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
			(*it)->recursiveDelete();
		}
		for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
			(*it)->recursiveDelete();
		}
		for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
			//FIXME delete (*it);
		}
	} else {
		for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
			(*it)->recursiveDeleteKeepVars();
		}
		for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
			(*it)->recursiveDeleteKeepVars();
		}
	}
}
bool Formula::contains(const Variable* v) const {
	for (auto it = _freevars.cbegin(); it != _freevars.cend(); ++it) {
		if (*it == v) {
			return true;
		}
	}
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		if (*it == v) {
			return true;
		}
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	return false;
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

PredForm* PredForm::cloneKeepVars() const {
	vector<Term*> na;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		na.push_back((*it)->cloneKeepVars());
	}
	PredForm* pf = new PredForm(sign(), _symbol, na, pi());
	return pf;
}

PredForm* PredForm::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Term*> na;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		na.push_back((*it)->clone(mvv));
	}
	PredForm* pf = new PredForm(sign(), _symbol, na, pi());
	return pf;
}

bool PredForm::isGraphedFunction() const {
	return symbol()->isFunction();
}

ostream& PredForm::put(ostream& output) const {
	if (isNeg(sign())) {
		output << '~';
	}
	_symbol->put(output);
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
	return output;
}

/******************
 EqChainForm
 ******************/

EqChainForm* EqChainForm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

EqChainForm* EqChainForm::cloneKeepVars() const {
	vector<Term*> nt;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		nt.push_back((*it)->cloneKeepVars());
	}
	EqChainForm* ef = new EqChainForm(sign(), _conj, nt, _comps, pi());
	return ef;
}

EqChainForm* EqChainForm::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Term*> nt;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		nt.push_back((*it)->clone(mvv));
	}
	EqChainForm* ef = new EqChainForm(sign(), _conj, nt, _comps, pi());
	return ef;
}

ostream& EqChainForm::put(ostream& output) const {
	if (isNeg(sign())) {
		output << '~';
	}
	output << '(';
	subterms()[0]->put(output);
	for (size_t n = 0; n < _comps.size(); ++n) {
		output << ' ' << print(comps()[n]) << ' ';
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

EquivForm* EquivForm::cloneKeepVars() const {
	Formula* nl = left()->cloneKeepVars();
	Formula* nr = right()->cloneKeepVars();
	EquivForm* ef = new EquivForm(sign(), nl, nr, pi());
	return ef;
}

EquivForm* EquivForm::clone(const map<Variable*, Variable*>& mvv) const {
	Formula* nl = left()->clone(mvv);
	Formula* nr = right()->clone(mvv);
	EquivForm* ef = new EquivForm(sign(), nl, nr, pi());
	return ef;
}

ostream& EquivForm::put(ostream& output) const {
	if(isNeg(sign())){
		output << '~';
	}
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

BoolForm* BoolForm::cloneKeepVars() const {
	vector<Formula*> ns;
	for (auto it = subformulas().cbegin(); it != subformulas().cend(); ++it) {
		ns.push_back((*it)->cloneKeepVars());
	}
	BoolForm* bf = new BoolForm(sign(), _conj, ns, pi());
	return bf;
}

BoolForm* BoolForm::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Formula*> ns;
	for (auto it = subformulas().cbegin(); it != subformulas().cend(); ++it) {
		ns.push_back((*it)->clone(mvv));
	}
	BoolForm* bf = new BoolForm(sign(), _conj, ns, pi());
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
		if (isNeg(sign())) {
			output << '~';
		}
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

QuantForm* QuantForm::cloneKeepVars() const {
	auto nf = subformula()->cloneKeepVars();
	return new QuantForm(sign(), quant(), quantVars(), nf, pi());
}

QuantForm* QuantForm::clone(const map<Variable*, Variable*>& mvv) const {
	varset nv;
	map<Variable*, Variable*> nmvv = mvv;
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		Variable* v = new Variable((*it)->name(), (*it)->sort(), pi());
		nv.insert(v);
		nmvv[*it] = v;
	}
	Formula* nf = subformula()->clone(nmvv);
	QuantForm* qf = new QuantForm(sign(), quant(), nv, nf, pi());
	return qf;
}

ostream& QuantForm::put(ostream& output) const {
	if (isNeg(sign())) {
		output << '~';
	}
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

AggForm::AggForm(SIGN sign, Term* l, CompType c, AggTerm* r, const FormulaParseInfo& pi)
		: Formula(sign, pi), _comp(c), _aggterm(r) {
	Assert(l!=NULL);
	Assert(r!=NULL);
	addSubterm(l);
	addSubterm(r);
}

AggForm* AggForm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

AggForm* AggForm::cloneKeepVars() const {
	auto nl = getBound()->cloneKeepVars();
	auto nr = getAggTerm()->cloneKeepVars();
	return new AggForm(sign(), nl, _comp, nr, pi());
}

AggForm* AggForm::clone(const map<Variable*, Variable*>& mvv) const {
	auto nl = getBound()->clone(mvv);
	auto nr = getAggTerm()->clone(mvv);
	return new AggForm(sign(), nl, _comp, nr, pi());
}

ostream& AggForm::put(ostream& output) const {
	if (isNeg(sign())) {
		output << '~';
	}
	output << '(';
	getBound()->put(output);
	output << ' ' << print(_comp) << ' ';
	getAggTerm()->put(output);
	output << ')';
	return output;
}

/***********
 Rule
 ***********/

Rule* Rule::clone() const {
	map<Variable*, Variable*> mvv;
	varset newqv;
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
		//FIXME delete (*it);
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

Definition::Definition()
		: id(getGlobal()->getNewID()) {
}

Definition* Definition::clone() const {
	Definition* newdef = new Definition();
	for (auto it = _rules.cbegin(); it != _rules.cend(); ++it) {
		newdef->add((*it)->clone());
	}
	return newdef;
}

PFSymbol* getDefSymbol(const Rule* r){
	if(VocabularyUtils::isPredicate(r->head()->symbol(), STDPRED::EQ)){
		return dynamic_cast<FuncTerm*>(r->head()->subterms()[0])->function();
	}else{
		return r->head()->symbol();
	}
}

void Definition::updateDefinedSymbols() {
	_defsyms.clear();
	for (auto rule : _rules) {
		_defsyms.insert(getDefSymbol(rule));
	}
}

void Definition::recursiveDelete() {
	for (auto rule : _rules) {
		rule->recursiveDelete();
	}
	delete (this);
}

void Definition::add(Rule* r) {
	_rules.insert(r);
	_defsyms.insert(getDefSymbol(r));
}

void Definition::remove(Rule* r) {
	_rules.erase(r);
	_defsyms.insert(getDefSymbol(r));
	updateDefinedSymbols();
}

ostream& Definition::put(ostream& output) const {
	output << "{";
	if (not _rules.empty()) {
		pushtab();
		for (auto rule : _rules) {
			output << nt();
			rule->put(output);
		}
		poptab();
	}
	output << nt() << '}';
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
			output << nt();
			_rules[n]->put(output);
		}
		poptab();
	}
	pushtab();
	for (auto it = _defs.cbegin(); it != _defs.cend(); ++it) {
		output << nt();
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
	auto newtheory = new Theory(_name, _vocabulary, ParseInfo());
	for (auto it : getComponents()) {
		newtheory->add(it->clone());
	}
	return newtheory;
}

void Theory::addTheory(AbstractTheory*) {
	// TODO
}

void Theory::recursiveDelete() {//FIXME handle all cases with an enum or a visitor
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

void Theory::add(Formula* f) {
	_sentences.push_back(f);
}
void Theory::add(Definition* d) {
	_definitions.push_back(d);
}
void Theory::add(FixpDef* fd) {
	_fixpdefs.push_back(fd);
}

void Theory::add(TheoryComponent* comp) { // FIXME handle all cases with an enum or a visitor
	auto form = dynamic_cast<Formula*>(comp);
	if (form != NULL) {
		add(form);
		return;
	}
	auto def = dynamic_cast<Definition*>(comp);
	if (def != NULL) {
		add(def);
		return;
	}
	auto fixpdef = dynamic_cast<FixpDef*>(comp);
	if (fixpdef != NULL) {
		add(fixpdef);
		return;
	}

	throw notyetimplemented("Adding theorycomponents to a theory that are no sentences or (complex) (fixpoint) definitions");
}


vector<TheoryComponent*> Theory::getComponents() const {
	vector<TheoryComponent*> stc;
	// NOTE: re-ordered for lazy grounding
	for (auto it = _definitions.cbegin(); it != _definitions.cend(); ++it) {
		stc.push_back(*it);
	}
	for (auto it = _sentences.cbegin(); it != _sentences.cend(); ++it) {
		auto quantform = dynamic_cast<QuantForm*>(*it);
		if (quantform != NULL && quantform->isUniv()) {
			stc.push_back(*it);
		}
	}
	for (auto it = _sentences.cbegin(); it != _sentences.cend(); ++it) {
		auto quantform = dynamic_cast<QuantForm*>(*it);
		if (quantform == NULL || not quantform->isUniv()) {
			stc.push_back(*it);
		}
	}
	for (auto it = _fixpdefs.cbegin(); it != _fixpdefs.cend(); ++it) {
		stc.push_back(*it);
	}
	return stc;
}

void Theory::remove(Definition* d) {
	auto it = _definitions.begin();
	for (; it != _definitions.end(); ++it) {
		if (*it == d) {
			break;
		}
	}
	if (it != _definitions.end()) {
		_definitions.erase(it);
	}
}

std::ostream& Theory::put(std::ostream& output) const {
	auto printer = Printer::create(output);
	printer->startTheory();
	printer->print(this);
	printer->endTheory();
	return output;
}

