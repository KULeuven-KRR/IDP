/************************************
	print.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "print.hpp"
#include <iostream>
#include <utility>
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "options.hpp"
#include "namespace.hpp"
#include "ecnf.hpp"
#include "ground.hpp"
#include "common.hpp"

using namespace std;
using namespace rel_ops;

/**************
    Printer
**************/

Printer::Printer() {
	// Set indentation level to zero
	_indent = 0;
}

Printer* Printer::create(Options* opts) {
	switch(opts->language()) {
		case LAN_IDP:
			return new IDPPrinter(opts->printtypes(),opts->longnames());
		case LAN_ECNF:
			return new EcnfPrinter(opts->writeTranslation());
		default:
			assert(false);
			return NULL;
	}
}

string Printer::print(const AbstractTheory* t)		{ t->accept(this); return _out.str(); }
string Printer::print(const Formula* f)				{ f->accept(this); return _out.str(); }

void Printer::indent() 		{ _indent++; }
void Printer::unindent()	{ _indent--; }
void Printer::printtab() {
	for(unsigned int n = 0; n < _indent; ++n)
		output() << "  ";
}

/***************
    Theories
***************/

void IDPPrinter::visit(const Theory* t) {
	for(vector<Formula*>::const_iterator it = t->sentences().begin(); it != t->sentences().end(); ++it) {
		(*it)->accept(this); output() << ".\n";
	}
	for(vector<Definition*>::const_iterator it = t->definitions().begin(); it != t->definitions().end(); ++it) {
		(*it)->accept(this);
	}
	for(vector<FixpDef*>::const_iterator it = t->fixpdefs().begin(); it != t->fixpdefs().end(); ++it) {
		(*it)->accept(this);
	}
}

/** Formulas **/

void IDPPrinter::visit(const PredForm* f) {
	if(! f->sign())	output() << "~";
	f->symbol()->put(output(),_longnames);
	if(!f->subterms().empty()) {
		output() << "(";
		f->subterms()[0]->accept(this);
		for(unsigned int n = 1; n < f->subterms().size(); ++n) {
			output() << ",";
			f->subterms()[n]->accept(this);
		}
		output() << ")";
	}
}

void IDPPrinter::visit(const EqChainForm* f) {
	if(! f->sign())	output() << "~";
	output() << "(";
	f->subterms()[0]->accept(this);
	for(unsigned int n = 0; n < f->comps().size(); ++n) {
		switch(f->comps()[n]) {
			case CT_EQ:
				output() << " = ";
				break;
			case CT_NEQ:
				output() << " ~= ";
				break;
			case CT_LEQ:
				output() << " =< ";
				break;
			case CT_GEQ:
				output() << " >= ";
				break;
			case CT_LT:
				output() << " < ";
				break;
			case CT_GT:
				output() << " > ";
				break;
		}
		f->subterms()[n+1]->accept(this);
		if(! f->conj() && n+1 < f->comps().size()) {
			output() << " | ";
			f->subterms()[n+1]->accept(this);
		}
	}
	output() << ")";
}

void IDPPrinter::visit(const EquivForm* f) {
	if(! f->sign())	output() << "~";
	output() << "(";
	f->left()->accept(this);
	output() << " <=> ";
	f->right()->accept(this);
	output() << ")";
}

void IDPPrinter::visit(const BoolForm* f) {
	if(f->subformulas().empty()) {
		if(f->sign() == f->conj())
			output() << "true";
		else
			output() << "false";
	}
	else {
		if(! f->sign())	output() << "~";
		output() << "(";
		f->subformulas()[0]->accept(this);
		for(unsigned int n = 1; n < f->subformulas().size(); ++n) {
			if(f->conj())
				output() << " & ";
			else
				output() << " | ";
			f->subformulas()[n]->accept(this);
		}
		output() << ")";
	}
}

void IDPPrinter::visit(const QuantForm* f) {
	if(! f->sign())	output() << "~";
	output() << "(";
	if(f->univ())
		output() << "!";
	else
		output() << "?";
	for(set<Variable*>::const_iterator it = f->quantvars().begin(); it != f->quantvars().end(); ++it) {
		output() << " ";
		output() << (*it)->name();
		if((*it)->sort())
			output() << "[" << *((*it)->sort()) << "]";
	}
	output() << " : ";
	f->subformulas()[0]->accept(this);
	output() << ")";
}

/** Definitions **/

void IDPPrinter::visit(const Rule* r) {
	printtab();
	if(!r->quantvars().empty()) {
		output() << "!";
		for(set<Variable*>::const_iterator it = r->quantvars().begin(); it != r->quantvars().end(); ++it) {
			output() << " " << *(*it);
		}
		output() << " : ";
	}
	r->head()->accept(this);
	output() << " <- ";
	r->body()->accept(this);
	output() << ".";
}

void IDPPrinter::visit(const Definition* d) {
	printtab();
	output() << "{\n";
	indent();
	for(vector<Rule*>::const_iterator it = d->rules().begin(); it != d->rules().end(); ++it) {
		(*it)->accept(this);
		output() << "\n";
	}
	unindent();
	printtab();
	output() << "}\n";
}

void IDPPrinter::visit(const FixpDef* d) {
	printtab();
	output() << (d->lfp() ? "LFD" : "GFD") << " [\n";
	indent();
	for(vector<Rule*>::const_iterator it = d->rules().begin(); it != d->rules().end(); ++it) {
		(*it)->accept(this);
		output() << "\n";
	}
	for(vector<FixpDef*>::const_iterator it = d->defs().begin(); it != d->defs().end(); ++it) {
		(*it)->accept(this);
	}
	unindent();
	printtab();
	output() << "]\n";
}

/** Terms **/

void IDPPrinter::visit(const VarTerm* t) {
	output() << t->var()->name();
}

void IDPPrinter::visit(const FuncTerm* t) {
	t->function()->put(output(),_longnames);
	if(!t->subterms().empty()) {
		output() << "(";
		t->subterms()[0]->accept(this);
		for(unsigned int n = 1; n < t->subterms().size(); ++n) {
			output() << ",";
			t->subterms()[n]->accept(this);
		}
		output() << ")";
	}
}

void IDPPrinter::visit(const DomainTerm* t) {
	string str = t->value()->to_string();
	if(t->sort()) {
		if(SortUtils::isSubsort(t->sort(),VocabularyUtils::charsort())) {
			output() << '\'' << str << '\'';
		}
		else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::stringsort())) {
			output() << '\"' << str << '\"';
		}
		else {
			output() << str;
		}
	}
	else output() << '@' << str;
}

void IDPPrinter::visit(const AggTerm* t) {
	switch(t->function()) {
		case AGG_CARD: output() << '#'; break;
		case AGG_SUM: output() << "sum"; break;
		case AGG_PROD: output() << "prod"; break;
		case AGG_MIN: output() << "min"; break;
		case AGG_MAX: output() << "max"; break;
		default: assert(false);
	}
	t->set()->accept(this);
}

/** Sets **/

void IDPPrinter::visit(const EnumSetExpr* s) {
	output() << "[ ";
	if(!s->subformulas().empty()) {
		s->subformulas()[0]->accept(this);
		for(unsigned int n = 1; n < s->subformulas().size(); ++n) {
			output() << ";";
			s->subformulas()[n]->accept(this);
		}
	}
	output() << " ]";
}

void IDPPrinter::visit(const QuantSetExpr* s) {
	output() << "{";
	for(set<Variable*>::const_iterator it = s->quantvars().begin(); it != s->quantvars().end(); ++it) {
		output() << " ";
		output() << (*it)->name();
		if((*it)->sort())
			output() << "[" << (*it)->sort()->name() << "]";
	}
	output() << ": ";
	s->subformulas()[0]->accept(this);
	output() << ": ";
	s->subterms()[0]->accept(this);
	output() << " }";
}

/****************
	Grounding
****************/

const int ID_FOR_UNDEFINED = -1;
void IDPPrinter::printAtom(int atomnr) {
	// Make sure there is a translator.
	assert(_translator);
	// The sign of the literal is handled on higher level.
	atomnr = abs(atomnr);
	// Get the atom's symbol from the translator.
	PFSymbol* pfs = _translator->symbol(atomnr);
	if(pfs) {
		// Print the symbol's name.
		output() << pfs->name().substr(0,pfs->name().find('/'));
		// Print the symbol's sorts.
		if(pfs->nrSorts()) {
			output() << '[';
			for(unsigned int n = 0; n < pfs->nrSorts(); ++n) {
				if(pfs->sort(n)) {
					output() << pfs->sort(n)->name();
					if(n != pfs->nrSorts()-1) output() << ',';
				}
			}
			output() << ']';
		}
		// Get the atom's arguments for the translator.
		const vector<const DomainElement*>& args = _translator->args(atomnr);
		// Print the atom's arguments.
		if(typeid(*pfs) == typeid(Predicate)) {
			if(not args.empty()) {
				output() << "(";
				for(unsigned int n = 0; n < args.size(); ++n) {
					output() << args[n]->to_string();
					if(n != args.size()-1) output() << ",";
				}
				output() << ")";
			}
		}
		else {
			assert(typeid(*pfs) == typeid(Function));
			if(args.size() > 1) {
				output() << "(";
				for(unsigned int n = 0; n < args.size()-1; ++n) {
					output() << args[n]->to_string();
					if(n != args.size()-2) output() << ",";
				}
				output() << ")";
			}
			output() << " = " << args.back()->to_string();
		}
	}
	else {
		// If there was no symbol, then the atom is a tseitin.
		assert(! pfs);
		output() << "tseitin_" << atomnr;
	}
}

void IDPPrinter::printTerm(unsigned int termnr) { 
	// Make sure there is a translator.
	assert(_termtranslator);
	// Get information from the term translator.
	const Function* func = _termtranslator->function(termnr);
	if(func) {
		// Print the symbol's name.
		output() << func->name().substr(0,func->name().find('/'));
		// Print the symbol's sorts.
		if(func->nrSorts()) {
			output() << '[';
			for(unsigned int n = 0; n < func->nrSorts(); ++n) {
				if(func->sort(n)) {
					output() << func->sort(n)->name();
					if(n != func->nrSorts()-1) output() << ',';
				}
			}
			output() << ']';
		}
		// Get the arguments from the translator.
		const vector<GroundTerm>& args = _termtranslator->args(termnr);
		// Print the arguments.
		if(not args.empty()) {
			output() << "(";
			for(vector<GroundTerm>::const_iterator gtit = args.begin(); gtit != args.end(); ++gtit) {
				if((*gtit)._isvarid) {
					printTerm((*gtit)._varid);
				} else {
					output() << (*gtit)._domelement->to_string();
				}
				if(*gtit != args.back()){
					output() << ",";
				}
			}
			output() << ")";
		}
	} else {
		_out << "var_" << termnr;
//		CPTsBody* cprelation = _termtranslator->cprelation(varid);
//		CPReification(1,cprelation).accept(this);
	}
}

void IDPPrinter::printAggregate(double bound, bool lower, AggFunction aggtype, unsigned int setnr) {
	output() << bound << (lower ? " =< " : " >= ");
	switch(aggtype) {
		case AGG_CARD: 	output() << "card("; break;
		case AGG_SUM: 	output() << "sum("; break;
		case AGG_PROD: 	output() << "prod("; break;
		case AGG_MIN: 	output() << "min("; break;
		case AGG_MAX: 	output() << "max("; break;
		default: assert(false);
	}
	output() << "set_" << setnr << ")." <<"\n";
}

void EcnfPrinter::printAggregate(AggFunction aggtype, TsType arrow, unsigned int defnr, bool lower, int head, unsigned int setnr, double bound) {
	switch(aggtype) {
		case AGG_CARD: 	output() << "Card "; break;
		case AGG_SUM: 	output() << "Sum "; break;
		case AGG_PROD: 	output() << "Prod "; break;
		case AGG_MIN: 	output() << "Min "; break;
		case AGG_MAX: 	output() << "Max "; break;
		default: assert(false);
	}
	#warning "Replacing implication by equivalence...";
	switch(arrow) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL:  // (Reverse) implication is not supported by solver (yet)
			output() << "C ";
			break;
		case TS_RULE: 
			output() << "<- " << defnr << ' ';
			break; 
		default: assert(false);
	}
	output() << (lower ? 'G' : 'L') << ' ' << head << ' ' << setnr << ' ' << bound << " 0" <<"\n";
}

void IDPPrinter::visit(const GroundClause& g){
	if(g.empty()) {
		output() << "false";
	}
	else {
		for(unsigned int m = 0; m < g.size(); ++m) {
			if(g[m] < 0) output() << '~';
			printAtom(g[m]);
			if(m < g.size()-1) output() << " | ";
		}
	}
	output() << "." <<"\n";
}

void IDPPrinter::visit(const GroundTheory* g) {
	_translator = g->translator();
	_termtranslator = g->termtranslator();
	for(unsigned int n = 0; n < g->nrClauses(); ++n) {
		visit(g->clause(n));
	}
	for(unsigned int n = 0; n < g->nrDefinitions(); ++n)
		g->definition(n)->accept(this);
	for(unsigned int n = 0; n < g->nrSets(); ++n)
		g->set(n)->accept(this);
	for(unsigned int n = 0; n < g->nrAggregates(); ++n)
		g->aggregate(n)->accept(this);
	//TODO: repeat above for fixpoint definitions
	for(unsigned int n = 0; n < g->nrCPReifications(); ++n)
		g->cpreification(n)->accept(this);
}

void EcnfPrinter::visit(const GroundClause& g){
	for(unsigned int m = 0; m < g.size(); ++m){
		output() << g[m] << ' ';
	}
	output() << '0' << "\n";
}

void EcnfPrinter::visit(const GroundTheory* g) {
	_structure = g->structure();
	_termtranslator = g->termtranslator();
	output() << "p ecnf def aggr\n";
	for(unsigned int n = 0; n < g->nrClauses(); ++n) {
		visit(g->clause(n));
	}
	for(unsigned int n = 0; n < g->nrSets(); ++n) //NOTE: Print sets before aggregates!!
		g->set(n)->accept(this);
	for(unsigned int n = 0; n < g->nrDefinitions(); ++n) {
		_currentdefnr = n+1; //NOTE: Ecnf parsers do not like identifiers to be zero.
		g->definition(n)->accept(this);
	}
	for(unsigned int n = 0; n < g->nrAggregates(); ++n)
		g->aggregate(n)->accept(this);
	//TODO: repeat above for fixpoint definitions

	for(unsigned int n = 0; n < g->nrCPReifications(); ++n) {
		g->cpreification(n)->accept(this);
	}

	if(writeTranlation()){
		output() <<"=== Atomtranslation ===" << "\n";
		GroundTranslator* translator = g->translator();
		int atom = 1;
		while(translator->isSymbol(atom)){
			if(!translator->isTseitin(atom)){
				output() << atom <<"|" <<translator->printAtom(atom) <<"\n";
			}
			atom++;
		}
		output() <<"==== ====" <<"\n";
	}
}

void IDPPrinter::visit(const GroundDefinition* d) {
	printtab();
	output() << "{\n";
	indent();
	for(GroundDefinition::const_ruleiterator it = d->begin(); it != d->end(); ++it) {
		printtab();
		printAtom(it->first);
		output() << " <- ";
		(it->second)->accept(this);
	}
	unindent();
	output() << "}\n";
}

void EcnfPrinter::visit(const GroundDefinition* d) {
	for(GroundDefinition::const_ruleiterator it = d->begin(); it != d->end(); ++it) {
		_currenthead = it->first;
		(it->second)->accept(this);
	}
}

void IDPPrinter::visit(const PCGroundRuleBody* b) {
	char c = (b->type() == RT_CONJ ? '&' : '|');
	if(! b->empty()) {
		for(unsigned int n = 0; n < b->size(); ++n) {
			if(b->literal(n) < 0) output() << '~';
			printAtom(b->literal(n));
			if(n != b->size()-1) output() << ' ' << c << ' ';
		}
	}
	else {
		assert(b->empty());
		if(b->type() == RT_CONJ)
			output() << "true";
		else
			output() << "false";
	}
	output() << ".\n";
}

void EcnfPrinter::visit(const PCGroundRuleBody* b) {
	output() << (b->type() == RT_CONJ ? "C " : "D ");
	output() << "<- " << _currentdefnr << ' ' << _currenthead << ' ';
	for(unsigned int n = 0; n < b->size(); ++n){
		output() << b->literal(n) << ' ';
	}
	output() << "0\n";
}

void IDPPrinter::visit(const AggGroundRuleBody* b) {
	printAggregate(b->bound(),b->lower(),b->aggtype(),b->setnr());
}

void EcnfPrinter::visit(const AggGroundRuleBody* b) {
	printAggregate(b->aggtype(),TS_RULE,_currentdefnr,b->lower(),_currenthead,b->setnr(),b->bound());
}

void IDPPrinter::visit(const GroundAggregate* a) {
	printAtom(a->head());
	switch(a->arrow()) {
		case TS_IMPL: 	output() << " => "; break;
		case TS_RIMPL: 	output() << " <= "; break;
		case TS_EQ: 	output() << " <=> "; break;
		case TS_RULE: default: assert(false);
	}
	printAggregate(a->bound(),a->lower(),a->type(),a->setnr());
}

void EcnfPrinter::visit(const GroundAggregate* a) {
	assert(a->arrow() != TS_RULE);
	printAggregate(a->type(),a->arrow(),ID_FOR_UNDEFINED,a->lower(),a->head(),a->setnr(),a->bound());
}

void IDPPrinter::visit(const GroundSet* s) {
	output() << "set_" << s->setnr() << " = [ ";
	for(unsigned int n = 0; n < s->size(); ++n) {
		if(s->weighted()) output() << '(';
		printAtom(s->literal(n));
		if(s->weighted()) output() << ',' << s->weight(n) << ')';
		if(n < s->size()-1) output() << "; ";
	}
	output() << " ]\n";
}

void EcnfPrinter::visit(const GroundSet* s) {
	output() << (s->weighted() ? "WSet" : "Set") << ' ' << s->setnr();
	for(unsigned int n = 0; n < s->size(); ++n) {
		output() << ' ' << s->literal(n);
		if(s->weighted()) output() << '=' << s->weight(n);
	}
	output() << " 0\n";
}

void IDPPrinter::visit(const CPReification* cpr) {
	printAtom(cpr->_head);
	switch(cpr->_body->type()) {
		case TS_RULE: 	output() << " <- "; break;
		case TS_IMPL: 	output() << " => "; break;
		case TS_RIMPL: 	output() << " <= "; break;
		case TS_EQ: 	output() << " <=> "; break;
		default: assert(false);
	}
	cpr->_body->left()->accept(this);
	switch(cpr->_body->comp()) {
		case CT_EQ:		output() << " = "; break;
		case CT_NEQ:	output() << " ~= "; break;
		case CT_LEQ:	output() << " =< "; break;
		case CT_GEQ:	output() << " >= "; break;
		case CT_LT:		output() << " < "; break;
		case CT_GT:		output() << " > "; break;
		default: assert(false);
	}
	CPBound right = cpr->_body->right();
	if(right._isvarid) printTerm(right._varid);
	else output() << right._bound;
	output() << ".\n";
}

void EcnfPrinter::printCPVariables(vector<unsigned int> varids) {
	for(vector<unsigned int>::const_iterator it = varids.begin(); it != varids.end(); ++it) {
		printCPVariable(*it);
	}
}

void EcnfPrinter::printCPVariable(unsigned int varid) {
	if(_printedvarids.find(varid) == _printedvarids.end()) {
		_printedvarids.insert(varid);
		SortTable* domain = _termtranslator->domain(varid);
		if(domain->isRange()) {
			int minvalue = domain->first()->value()._int;
			int maxvalue = domain->last()->value()._int;
			_out << "INTVAR " << varid << ' ' << minvalue << ' ' << maxvalue << ' ';
		} else {
			_out << "INTVARDOM " << varid << ' ';
			for(SortIterator it = domain->sortbegin(); it.hasNext(); ++it) {
				int value = (*it)->value()._int;
				_out << value << ' ';
			}
		}
		_out << '0' << "\n";
	}
}

void EcnfPrinter::printCPReification(string type, int head, unsigned int left, CompType comp, long right) {
	_out << type << ' ' << head << ' ' << left << ' ' << comp << ' ' << right << " 0" << "\n";
}

void EcnfPrinter::printCPReification(string type, int head, vector<unsigned int> left, CompType comp, long right) {
	_out << type << ' ' << head << ' ';
	for(vector<unsigned int>::const_iterator it = left.begin(); it != left.end(); ++it)
		_out << *it << ' ';
	_out << comp << ' ' << right << " 0" << "\n";
}

void EcnfPrinter::printCPReification(string type, int head, vector<unsigned int> left, vector<int> weights, CompType comp, long right) {
	_out << type << ' ' << head << ' ';
	for(vector<unsigned int>::const_iterator it = left.begin(); it != left.end(); ++it)
		_out << *it << ' ';
	_out << " | ";
	for(vector<int>::const_iterator it = weights.begin(); it != weights.end(); ++it)
		_out << *it << ' ';
	_out << comp << ' ' << right << " 0" << "\n";
}

void EcnfPrinter::visit(const CPReification* cpr) {
	CompType comp = cpr->_body->comp();
	CPTerm* left = cpr->_body->left();
	CPBound right = cpr->_body->right();
	if(typeid(*left) == typeid(CPVarTerm)) {
		CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
		printCPVariable(term->_varid);
		if(right._isvarid) { // CPBinaryRelVar
			printCPVariable(right._varid);
			printCPReification("BINTRT",cpr->_head,term->_varid,comp,right._varid);
		}
		else { // CPBinaryRel
			printCPReification("BINTRI",cpr->_head,term->_varid,comp,right._bound);
		}
	}
	else if(typeid(*left) == typeid(CPSumTerm)) {
		CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
		printCPVariables(term->_varids);
		if(right._isvarid) { // CPSumWithVar
			printCPVariable(right._varid);
			printCPReification("SUMSTRT",cpr->_head,term->_varids,comp,right._varid);
		}
		else { // CPSum
			printCPReification("SUMSTRI",cpr->_head,term->_varids,comp,right._bound);
		}
	}
	else {
		assert(typeid(*left) == typeid(CPWSumTerm));
		CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
		printCPVariables(term->_varids);
		if(right._isvarid) { // CPSumWeightedWithVar
			printCPVariable(right._varid);
			printCPReification("SUMSTSIRT",cpr->_head,term->_varids,term->_weights,comp,right._varid);
		}
		else { // CPSumWeighted
			printCPReification("SUMSTSIRI",cpr->_head,term->_varids,term->_weights,comp,right._bound);
		}
	}
}

void IDPPrinter::visit(const CPSumTerm* cpt) {
	output() << "sum[ ";
	for(vector<unsigned int>::const_iterator vit = cpt->_varids.begin(); vit != cpt->_varids.end(); ++vit) {
		printTerm(*vit);
		if(*vit != cpt->_varids.back()) output() << "; ";
	}
	output() << " ]";
}

void IDPPrinter::visit(const CPWSumTerm* cpt) {
	vector<unsigned int>::const_iterator vit;
	vector<int>::const_iterator wit;
	output() << "wsum[ ";
	for(vit = cpt->_varids.begin(), wit = cpt->_weights.begin(); vit != cpt->_varids.end() && wit != cpt->_weights.end(); ++vit, ++wit) {
		output() << '('; printTerm(*vit); output() << ',' << *wit << ')';
		if(*vit != cpt->_varids.back()) output() << "; ";
	}
	output() << " ]";
}

void IDPPrinter::visit(const CPVarTerm* cpt) {
	printTerm(cpt->_varid);
}


/*****************
    Structures
*****************/

ostream& IDPPrinter::print(std::ostream& output, SortTable* table) const {
	SortIterator it = table->sortbegin();
	output << "{ ";
	if(it.hasNext()) {
		output << (*it)->to_string();
		++it;
		for(; it.hasNext(); ++it) {
			output << "; " << (*it)->to_string();
		}
	}
	output << " }";
	return output;
}

ostream& IDPPrinter::print(std::ostream& output, const PredTable* table) const {
	if(table->approxfinite()) {
		TableIterator kt = table->begin();
		if(table->arity()) {
			output << "{ ";
			if(kt.hasNext()) {
				ElementTuple tuple = *kt;
				output << tuple[0]->to_string();
				for(ElementTuple::const_iterator lt = ++tuple.begin(); lt != tuple.end(); ++lt) {
					output << ',' << (*lt)->to_string();
				}
				++kt;
				for(; kt.hasNext(); ++kt) {
					output << "; ";
					tuple = *kt;
					output << tuple[0]->to_string();
					for(ElementTuple::const_iterator lt = ++tuple.begin(); lt != tuple.end(); ++lt) {
						output << ',' << (*lt)->to_string();
					}
				}
			}
			output << " }";
		}
		else if(kt.hasNext()) output << "true";
		else output << "false";
		return output;
	}
	else return output << "possibly infinite table";
}

ostream& IDPPrinter::printasfunc(std::ostream& output, const PredTable* table) const {
	if(table->approxfinite()) {
		TableIterator kt = table->begin();
		output << "{ ";
		if(kt.hasNext()) {
			ElementTuple tuple = *kt;
			if(tuple.size() > 1) output << tuple[0]->to_string();
			for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
				output << ',' << tuple[n]->to_string();
			}
			output << "->" << tuple.back()->to_string();
			++kt;
			for(; kt.hasNext(); ++kt) {
				output << "; ";
				tuple = *kt;
				if(tuple.size() > 1) output << tuple[0]->to_string();
				for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
					output << ',' << tuple[n]->to_string();
				}
				output << "->" << tuple.back()->to_string();
			}
		}
		output << " }";
		return output;
	}
	else return output << "possibly infinite table";
}

ostream& IDPPrinter::print(std::ostream& output, FuncTable* table) const {
	vector<SortTable*> vst = table->universe().tables();
	vst.pop_back();
	Universe univ(vst);
	if(univ.approxfinite()) {
		TableIterator kt = table->begin();
		if(table->arity() != 0) {
			output << "{ ";
			if(kt.hasNext()) {
				ElementTuple tuple = *kt;
				output << tuple[0]->to_string();
				for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
					output << ',' << tuple[n]->to_string();
				}
				output << "->" << tuple.back()->to_string();
				++kt;
				for(; kt.hasNext(); ++kt) {
					output << "; ";
					tuple = *kt;
					output << tuple[0]->to_string();
					for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
						output << ',' << tuple[n]->to_string();
					}
					output << "->" << tuple.back()->to_string();
				}
			}
			output << " }";
		}
		else if(kt.hasNext()) output << (*kt)[0]->to_string();
		else output << "{ }";
		return output;
	}
	else return output << "possibly infinite table";
}

string IDPPrinter::print(const AbstractStructure* structure) {
	stringstream sstr;
	Vocabulary* voc = structure->vocabulary();

	for(map<string,set<Sort*> >::const_iterator it = voc->firstsort(); it != voc->lastsort(); ++it) {
		for(set<Sort*>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			Sort* s = *jt;
			if(!s->builtin()) {
				sstr << *s << " = ";
				SortTable* st = structure->inter(s);
				print(sstr,st);
				sstr << '\n';
			}
		}
	}
	for(map<string,Predicate*>::const_iterator it = voc->firstpred(); it != voc->lastpred(); ++it) {
		set<Predicate*> sp = it->second->nonbuiltins();
		for(set<Predicate*>::iterator jt = sp.begin(); jt != sp.end(); ++jt) {
			Predicate* p = *jt;
			if(p->arity() != 1 || p->sorts()[0]->pred() != p) {
				PredInter* pi = structure->inter(p);
				if(pi->approxtwovalued()) {
					sstr << *p << " = ";
					const PredTable* pt = pi->ct();
					print(sstr,pt);
					sstr << '\n';
				}
				else {
					const PredTable* ct = pi->ct();
					sstr << *p << "<ct> = ";
					print(sstr,ct);
					sstr << '\n';
					const PredTable* cf = pi->cf();
					sstr << *p << "<cf> = ";
					print(sstr,cf);
					sstr << '\n';
				}
			}
		}
	}
	for(map<string,Function*>::const_iterator it = voc->firstfunc(); it != voc->lastfunc(); ++it) {
		set<Function*> sf = it->second->nonbuiltins();
		for(set<Function*>::iterator jt = sf.begin(); jt != sf.end(); ++jt) {
			Function* f = *jt;
			FuncInter* fi = structure->inter(f);
			if(fi->approxtwovalued()) {
				FuncTable* ft = fi->functable();
				sstr << *f << " = ";
				print(sstr,ft);
				sstr << '\n';
			}
			else {
				PredInter* pi = fi->graphinter();
				const PredTable* ct = pi->ct();
				sstr << *f << "<ct> = ";
				printasfunc(sstr,ct);
				sstr << '\n';
				const PredTable* cf = pi->cf();
				sstr << *f << "<cf> = ";
				printasfunc(sstr,cf);
				sstr << '\n';
			}
		}
	}
	return sstr.str();
}

/*******************
    Vocabularies
*******************/
string EcnfPrinter::print(const Vocabulary* ) {
	return "(vocabulary cannot be printed in ecnf)";
}

string EcnfPrinter::print(const Namespace* ) {
	return "(namespace cannot be printed in ecnf)";
}

string EcnfPrinter::print(const AbstractStructure* ) {
	return "(structure cannot be printed in ecnf)";
}

string IDPPrinter::print(const Vocabulary* v) {
	for(map<string,set<Sort*> >::const_iterator it = v->firstsort(); it != v->lastsort(); ++it) {
		for(set<Sort*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			if(!(*jt)->builtin() || v == Vocabulary::std()) visit(*jt);
		}
	}
	for(map<string,Predicate*>::const_iterator it = v->firstpred(); it != v->lastpred(); ++it) {
		if(!it->second->builtin() || v == Vocabulary::std()) visit(it->second);
	}
	for(map<string,Function*>::const_iterator it = v->firstfunc(); it != v->lastfunc(); ++it) {
		if(!it->second->builtin() || v == Vocabulary::std()) visit(it->second);
	}
	return output().str();
}

void IDPPrinter::visit(const Sort* s) {
	printtab();
	output() << "type " << s->name();
	if(!(s->parents().empty())) {
		output() << " isa " << (*(s->parents().begin()))->name();
		set<Sort*>::const_iterator it = s->parents().begin(); ++it;
		for(; it != s->parents().end(); ++it)
			output() << "," << (*it)->name();
	}
	output() << "\n";
}

void IDPPrinter::visit(const Predicate* p) {
	printtab();
	if(p->overloaded()) {
		output() << "overloaded predicate " << p->name() << '\n';
	}
	else {
		output() << p->name().substr(0,p->name().find('/'));
		if(p->arity() > 0) {
			output() << "(" << p->sort(0)->name();
			for(unsigned int n = 1; n < p->arity(); ++n)
				output() << "," << p->sort(n)->name();
			output() << ")";
		}
		output() << "\n";
	}
}

void IDPPrinter::visit(const Function* f) {
	printtab();
	if(f->overloaded()) {
		output() << "overloaded function " << f->name() << '\n';
	}
	else {
		if(f->partial())
			output() << "partial ";
		output() << f->name().substr(0,f->name().find('/'));
		if(f->arity() > 0) {
			output() << "(" << f->insort(0)->name();
			for(unsigned int n = 1; n < f->arity(); ++n)
				output() << "," << f->insort(n)->name();
			output() << ")";
		}
		output() << " : " << f->outsort()->name() << "\n";
	}
}

/*****************
	Namespaces
*****************/

string IDPPrinter::print(const Namespace* ) {
	return string("not yet implemented");
}

/*
void IDPPrinter::visit(const Namespace* s) {
	for(unsigned int n = 0; n < s->nrVocs(); ++n) {
		printtab();
		output() << "#vocabulary " << s->vocabulary(n)->name() << " {\n";
		indent();
		s->vocabulary(n)->accept(this);
		unindent();
		printtab();
		output() << "}\n";
	}
	for(unsigned int n = 0; n < s->nrTheos(); ++n) {
		printtab();
		output() << "#theory " << s->theory(n)->name();
		output() << " : " << s->theory(n)->vocabulary()->name() << " {\n";
		indent();
		s->theory(n)->accept(this);
		unindent();
		printtab();
		output() << "}\n";
	}
	for(unsigned int n = 0; n < s->nrStructs(); ++n) {
		printtab();
		output() << "#structure " << s->structure(n)->name();
		output() << " : " << s->structure(n)->vocabulary()->name() << " {\n";
		indent();
		s->structure(n)->accept(this);
		unindent();
		printtab();
		output() << "}\n";
	} 
	for(unsigned int n = 0; n < s->nrSubs(); ++n) {
		printtab();
		output() << "#namespace " << s->subspace(n)->name() << " {\n";
		indent();
		s->subspace(n)->accept(this);
		unindent();
		printtab();
		output() << "}\n";
	}
	//TODO Print procedures and options?
}
*/
