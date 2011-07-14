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
			return new EcnfPrinter();
		default:
			assert(false);
	}
}

string Printer::print(const AbstractTheory* t)		{ t->accept(this); return _out.str(); }

void Printer::indent() 		{ _indent++; }
void Printer::unindent()	{ _indent--; }
void Printer::printtab() {
	for(unsigned int n = 0; n < _indent; ++n)
		_out << "  ";
}

/***************
    Theories
***************/

void IDPPrinter::visit(const Theory* t) {
	for(vector<Formula*>::const_iterator it = t->sentences().begin(); it != t->sentences().end(); ++it) {
		(*it)->accept(this); _out << ".\n";
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
	if(! f->sign())	_out << "~";
	f->symbol()->put(_out,_longnames);
	if(!f->subterms().empty()) {
		_out << "(";
		f->subterms()[0]->accept(this);
		for(unsigned int n = 1; n < f->subterms().size(); ++n) {
			_out << ",";
			f->subterms()[n]->accept(this);
		}
		_out << ")";
	}
}

void IDPPrinter::visit(const EqChainForm* f) {
	if(! f->sign())	_out << "~";
	_out << "(";
	f->subterms()[0]->accept(this);
	for(unsigned int n = 0; n < f->comps().size(); ++n) {
		switch(f->comps()[n]) {
			case CT_EQ:
				_out << " = ";
				break;
			case CT_NEQ:
				_out << " ~= ";
				break;
			case CT_LEQ:
				_out << " =< ";
				break;
			case CT_GEQ:
				_out << " >= ";
				break;
			case CT_LT:
				_out << " < ";
				break;
			case CT_GT:
				_out << " > ";
				break;
		}
		f->subterms()[n+1]->accept(this);
		if(! f->conj() && n+1 < f->comps().size()) {
			_out << " | ";
			f->subterms()[n+1]->accept(this);
		}
	}
	_out << ")";
}

void IDPPrinter::visit(const EquivForm* f) {
	if(! f->sign())	_out << "~";
	_out << "(";
	f->left()->accept(this);
	_out << " <=> ";
	f->right()->accept(this);
	_out << ")";
}

void IDPPrinter::visit(const BoolForm* f) {
	if(f->subformulas().empty()) {
		if(f->sign() == f->conj())
			_out << "true";
		else
			_out << "false";
	}
	else {
		if(! f->sign())	_out << "~";
		_out << "(";
		f->subformulas()[0]->accept(this);
		for(unsigned int n = 1; n < f->subformulas().size(); ++n) {
			if(f->conj())
				_out << " & ";
			else
				_out << " | ";
			f->subformulas()[n]->accept(this);
		}
		_out << ")";
	}
}

void IDPPrinter::visit(const QuantForm* f) {
	if(! f->sign())	_out << "~";
	_out << "(";
	if(f->univ())
		_out << "!";
	else
		_out << "?";
	for(set<Variable*>::const_iterator it = f->quantvars().begin(); it != f->quantvars().end(); ++it) {
		_out << " ";
		_out << (*it)->name();
		if((*it)->sort())
			_out << "[" << *((*it)->sort()) << "]";
	}
	_out << " : ";
	f->subformulas()[0]->accept(this);
	_out << ")";
}

/** Definitions **/

void IDPPrinter::visit(const Rule* r) {
	printtab();
	if(!r->quantvars().empty()) {
		_out << "!";
		for(set<Variable*>::const_iterator it = r->quantvars().begin(); it != r->quantvars().end(); ++it) {
			_out << " " << *(*it);
		}
		_out << " : ";
	}
	r->head()->accept(this);
	_out << " <- ";
	r->body()->accept(this);
	_out << ".";
}

void IDPPrinter::visit(const Definition* d) {
	printtab();
	_out << "{\n";
	indent();
	for(vector<Rule*>::const_iterator it = d->rules().begin(); it != d->rules().end(); ++it) {
		(*it)->accept(this);
		_out << "\n";
	}
	unindent();
	printtab();
	_out << "}\n";
}

void IDPPrinter::visit(const FixpDef* d) {
	printtab();
	_out << (d->lfp() ? "LFD" : "GFD") << " [\n";
	indent();
	for(vector<Rule*>::const_iterator it = d->rules().begin(); it != d->rules().end(); ++it) {
		(*it)->accept(this);
		_out << "\n";
	}
	for(vector<FixpDef*>::const_iterator it = d->defs().begin(); it != d->defs().end(); ++it) {
		(*it)->accept(this);
	}
	unindent();
	printtab();
	_out << "]\n";
}

/** Terms **/

void IDPPrinter::visit(const VarTerm* t) {
	_out << t->var()->name();
}

void IDPPrinter::visit(const FuncTerm* t) {
	t->function()->put(_out,_longnames);
	if(!t->subterms().empty()) {
		_out << "(";
		t->subterms()[0]->accept(this);
		for(unsigned int n = 1; n < t->subterms().size(); ++n) {
			_out << ",";
			t->subterms()[n]->accept(this);
		}
		_out << ")";
	}
}

void IDPPrinter::visit(const DomainTerm* t) {
	string str = t->value()->to_string();
	if(t->sort()) {
		if(SortUtils::isSubsort(t->sort(),VocabularyUtils::charsort())) {
			_out << '\'' << str << '\'';
		}
		else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::stringsort())) {
			_out << '\"' << str << '\"';
		}
		else {
			_out << str;
		}
	}
	else _out << '@' << str; 
}

void IDPPrinter::visit(const AggTerm* t) {
	switch(t->function()) {
		case AGG_CARD: _out << '#'; break;
		case AGG_SUM: _out << "sum"; break;
		case AGG_PROD: _out << "prod"; break;
		case AGG_MIN: _out << "min"; break;
		case AGG_MAX: _out << "max"; break;
		default: assert(false);
	}
	t->set()->accept(this);
}

/** Sets **/

void IDPPrinter::visit(const EnumSetExpr* s) {
	_out << "[ ";
	if(!s->subformulas().empty()) {
		s->subformulas()[0]->accept(this);
		for(unsigned int n = 1; n < s->subformulas().size(); ++n) {
			_out << ";";
			s->subformulas()[n]->accept(this);
		}
	}
	_out << " ]";
}

void IDPPrinter::visit(const QuantSetExpr* s) {
	_out << "{";
	for(set<Variable*>::const_iterator it = s->quantvars().begin(); it != s->quantvars().end(); ++it) {
		_out << " ";
		_out << (*it)->name();
		if((*it)->sort())
			_out << "[" << (*it)->sort()->name() << "]";
	}
	_out << ": ";
	s->subformulas()[0]->accept(this);
	_out << ": ";
	s->subterms()[0]->accept(this);
	_out << " }";
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
		_out << pfs->name().substr(0,pfs->name().find('/'));
		// Print the symbol's sorts.
		if(pfs->nrSorts()) {
			_out << '[';
			for(unsigned int n = 0; n < pfs->nrSorts(); ++n) {
				if(pfs->sort(n)) {
					_out << pfs->sort(n)->name();
					if(n != pfs->nrSorts()-1) _out << ',';
				}
			}
			_out << ']';
		}
		// Get the atom's arguments for the translator.
		const vector<const DomainElement*>& args = _translator->args(atomnr);
		// Print the atom's arguments.
		if(typeid(*pfs) == typeid(Predicate)) {
			if(not args.empty()) {
				_out << "(";
				for(unsigned int n = 0; n < args.size(); ++n) {
					_out << args[n]->to_string();
					if(n != args.size()-1) _out << ",";
				}
				_out << ")";
			}
		}
		else {
			assert(typeid(*pfs) == typeid(Function));
			if(args.size() > 1) {
				_out << "(";
				for(unsigned int n = 0; n < args.size()-1; ++n) {
					_out << args[n]->to_string();
					if(n != args.size()-2) _out << ",";
				}
				_out << ")";
			}
			_out << " = " << args.back()->to_string();
		}
	}
	else {
		// If there was no symbol, then the atom is a tseitin.
		assert(! pfs);
		_out << "tseitin_" << atomnr;
	}
}

void IDPPrinter::printTerm(unsigned int termnr) { 
	// Make sure there is a translator.
	assert(_termtranslator);
	// Get information from the term translator.
	const Function* func = _termtranslator->function(termnr);
	if(func) {
		// Print the symbol's name.
		_out << func->name().substr(0,func->name().find('/'));
		// Print the symbol's sorts.
		if(func->nrSorts()) {
			_out << '[';
			for(unsigned int n = 0; n < func->nrSorts(); ++n) {
				if(func->sort(n)) {
					_out << func->sort(n)->name();
					if(n != func->nrSorts()-1) _out << ',';
				}
			}
			_out << ']';
		}
		// Get the arguments from the translator.
		const vector<GroundTerm>& args = _termtranslator->args(termnr);
		// Print the arguments.
		if(not args.empty()) {
			_out << "(";
			for(vector<GroundTerm>::const_iterator gtit = args.begin(); gtit != args.end(); ++gtit) {
				if((*gtit)._isvarid) {
					printTerm((*gtit)._varid);
				} else {
					_out << (*gtit)._domelement->to_string();
				}
				if(*gtit != args.back()) _out << ",";
			}
			_out << ")";
		}
	} else {
		_out << "var_" << termnr;
//		CPTsBody* cprelation = _termtranslator->cprelation(varid);
//		CPReification(1,cprelation).accept(this);
	}
}

void IDPPrinter::printAggregate(double bound, bool lower, AggFunction aggtype, unsigned int setnr) {
	_out << bound << (lower ? " =< " : " >= ");
	switch(aggtype) {
		case AGG_CARD: 	_out << "card("; break;
		case AGG_SUM: 	_out << "sum("; break;
		case AGG_PROD: 	_out << "prod("; break;
		case AGG_MIN: 	_out << "min("; break;
		case AGG_MAX: 	_out << "max("; break;
		default: assert(false);
	}
	_out << "set_" << setnr << ")." << endl;
}

void EcnfPrinter::printAggregate(AggFunction aggtype, TsType arrow, unsigned int defnr, bool lower, int head, unsigned int setnr, double bound) {
	switch(aggtype) {
		case AGG_CARD: 	_out << "Card "; break;
		case AGG_SUM: 	_out << "Sum "; break;
		case AGG_PROD: 	_out << "Prod "; break;
		case AGG_MIN: 	_out << "Min "; break;
		case AGG_MAX: 	_out << "Max "; break;
		default: assert(false);
	}
	#warning "Replacing implication by equivalence...";
	switch(arrow) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL:  // (Reverse) implication is not supported by solver (yet)
			_out << "C ";
			break;
		case TS_RULE: 
			_out << "<- " << defnr << ' ';
			break; 
		default: assert(false);
	}
	_out << (lower ? 'G' : 'L') << ' ' << head << ' ' << setnr << ' ' << bound << " 0" << endl;
}

void IDPPrinter::visit(const GroundTheory* g) {
	_translator = g->translator();
	_termtranslator = g->termtranslator();
	for(unsigned int n = 0; n < g->nrClauses(); ++n) {
//TODO visitor for GroundClause?
		if(g->clause(n).empty()) {
			_out << "false";
		}
		else {
			for(unsigned int m = 0; m < g->clause(n).size(); ++m) {
				if(g->clause(n)[m] < 0) _out << '~';
				printAtom(g->clause(n)[m]);
				if(m < g->clause(n).size()-1) _out << " | ";
			}
		}
		_out << "." << endl;
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

void EcnfPrinter::visit(const GroundTheory* g) {
	_structure = g->structure();
	_termtranslator = g->termtranslator();
	_out << "p ecnf def aggr\n";
	for(unsigned int n = 0; n < g->nrClauses(); ++n) {
		for(unsigned int m = 0; m < g->clause(n).size(); ++m)
			_out << g->clause(n)[m] << ' ';
		_out << '0' << endl;
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
	for(unsigned int n = 0; n < g->nrCPReifications(); ++n)
		g->cpreification(n)->accept(this);
}

void IDPPrinter::visit(const GroundDefinition* d) {
	printtab();
	_out << '{' << endl;
	indent();
	for(GroundDefinition::const_ruleiterator it = d->begin(); it != d->end(); ++it) {
		printtab();
		printAtom(it->first);
		_out << " <- ";
		(it->second)->accept(this);
	}
	unindent();
	_out << '}' << endl;
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
			if(b->literal(n) < 0) _out << '~';
			printAtom(b->literal(n));
			if(n != b->size()-1) _out << ' ' << c << ' ';
		}
	}
	else {
		assert(b->empty());
		if(b->type() == RT_CONJ)
			_out << "true";
		else
			_out << "false";
	}
	_out << '.' << endl;
}

void EcnfPrinter::visit(const PCGroundRuleBody* b) {
	_out << (b->type() == RT_CONJ ? "C " : "D ");
	_out << "<- " << _currentdefnr << ' ' << _currenthead << ' ';
	for(unsigned int n = 0; n < b->size(); ++n)
		_out << b->literal(n) << ' ';
	_out << '0' << endl;
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
		case TS_IMPL: 	_out << " => "; break;
		case TS_RIMPL: 	_out << " <= "; break;
		case TS_EQ: 	_out << " <=> "; break;
		case TS_RULE: default: assert(false);
	}
	printAggregate(a->bound(),a->lower(),a->type(),a->setnr());
}

void EcnfPrinter::visit(const GroundAggregate* a) {
	assert(a->arrow() != TS_RULE);
	printAggregate(a->type(),a->arrow(),ID_FOR_UNDEFINED,a->lower(),a->head(),a->setnr(),a->bound());
}

void IDPPrinter::visit(const GroundSet* s) {
	_out << "set_" << s->setnr() << " = [ ";
	for(unsigned int n = 0; n < s->size(); ++n) {
		if(s->weighted()) _out << '('; 
		printAtom(s->literal(n));
		if(s->weighted()) _out << ',' << s->weight(n) << ')';
		if(n < s->size()-1) _out << "; ";
	}
	_out << " ]" << endl;
}

void EcnfPrinter::visit(const GroundSet* s) {
	_out << (s->weighted() ? "WSet" : "Set") << ' ' << s->setnr();
	for(unsigned int n = 0; n < s->size(); ++n) {
		_out << ' ' << s->literal(n);
		if(s->weighted()) _out << '=' << s->weight(n);
	}
	_out << " 0" << endl;
}

void IDPPrinter::visit(const CPReification* cpr) {
	printAtom(cpr->_head);
	switch(cpr->_body->type()) {
		case TS_RULE: 	_out << " <- "; break;
		case TS_IMPL: 	_out << " => "; break;
		case TS_RIMPL: 	_out << " <= "; break;
		case TS_EQ: 	_out << " <=> "; break;
		default: assert(false);
	}
	cpr->_body->left()->accept(this);
	switch(cpr->_body->comp()) {
		case CT_EQ:		_out << " = "; break;
		case CT_NEQ:	_out << " ~= "; break;
		case CT_LEQ:	_out << " =< "; break;
		case CT_GEQ:	_out << " >= "; break;
		case CT_LT:		_out << " < "; break;
		case CT_GT:		_out << " > "; break;
		default: assert(false);
	}
	CPBound right = cpr->_body->right();
	if(right._isvarid) printTerm(right._varid);
	else _out << right._bound;
	_out << '.' << endl;
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
		assert(domain);
		assert(domain->approxfinite());
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
		_out << '0' << endl;
	}
}

void EcnfPrinter::printCPReification(string type, int head, unsigned int left, CompType comp, long right) {
	_out << type << ' ' << head << ' ' << left << ' ' << comp << ' ' << right << " 0" << endl;
}

void EcnfPrinter::printCPReification(string type, int head, vector<unsigned int> left, CompType comp, long right) {
	_out << type << ' ' << head << ' ';
	for(vector<unsigned int>::const_iterator it = left.begin(); it != left.end(); ++it)
		_out << *it << ' ';
	_out << comp << ' ' << right << " 0" << endl;
}

void EcnfPrinter::printCPReification(string type, int head, vector<unsigned int> left, vector<int> weights, CompType comp, long right) {
	_out << type << ' ' << head << ' ';
	for(vector<unsigned int>::const_iterator it = left.begin(); it != left.end(); ++it)
		_out << *it << ' ';
	_out << " | ";
	for(vector<int>::const_iterator it = weights.begin(); it != weights.end(); ++it)
		_out << *it << ' ';
	_out << comp << ' ' << right << " 0" << endl;
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
	_out << "sum[ ";
	for(vector<unsigned int>::const_iterator vit = cpt->_varids.begin(); vit != cpt->_varids.end(); ++vit) {
		printTerm(*vit);
		if(*vit != cpt->_varids.back()) _out << "; ";
	}
	_out << " ]";
}

void IDPPrinter::visit(const CPWSumTerm* cpt) {
	vector<unsigned int>::const_iterator vit;
	vector<int>::const_iterator wit;
	_out << "wsum[ ";
	for(vit = cpt->_varids.begin(), wit = cpt->_weights.begin(); vit != cpt->_varids.end() && wit != cpt->_weights.end(); ++vit, ++wit) {
		_out << '('; printTerm(*vit); _out << ',' << *wit << ')';
		if(*vit != cpt->_varids.back()) _out << "; ";
	}
	_out << " ]";
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
	return _out.str();
}

void IDPPrinter::visit(const Sort* s) {
	printtab();
	_out << "type " << s->name();
	if(!(s->parents().empty())) {
		_out << " isa " << (*(s->parents().begin()))->name();
		set<Sort*>::const_iterator it = s->parents().begin(); ++it;
		for(; it != s->parents().end(); ++it)
			_out << "," << (*it)->name();
	}
	_out << "\n";
}

void IDPPrinter::visit(const Predicate* p) {
	printtab();
	if(p->overloaded()) {
		_out << "overloaded predicate " << p->name() << '\n';
	}
	else {
		_out << p->name().substr(0,p->name().find('/'));
		if(p->arity() > 0) {
			_out << "(" << p->sort(0)->name();
			for(unsigned int n = 1; n < p->arity(); ++n)
				_out << "," << p->sort(n)->name();
			_out << ")";
		}
		_out << "\n";
	}
}

void IDPPrinter::visit(const Function* f) {
	printtab();
	if(f->overloaded()) {
		_out << "overloaded function " << f->name() << '\n';
	}
	else {
		if(f->partial())
			_out << "partial ";
		_out << f->name().substr(0,f->name().find('/'));
		if(f->arity() > 0) {
			_out << "(" << f->insort(0)->name();
			for(unsigned int n = 1; n < f->arity(); ++n)
				_out << "," << f->insort(n)->name();
			_out << ")";
		}
		_out << " : " << f->outsort()->name() << "\n";
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
		_out << "#vocabulary " << s->vocabulary(n)->name() << " {\n";
		indent();
		s->vocabulary(n)->accept(this);
		unindent();
		printtab();
		_out << "}\n";
	}
	for(unsigned int n = 0; n < s->nrTheos(); ++n) {
		printtab();
		_out << "#theory " << s->theory(n)->name();
		_out << " : " << s->theory(n)->vocabulary()->name() << " {\n";
		indent();
		s->theory(n)->accept(this);
		unindent();
		printtab();
		_out << "}\n";
	}
	for(unsigned int n = 0; n < s->nrStructs(); ++n) {
		printtab();
		_out << "#structure " << s->structure(n)->name(); 
		_out << " : " << s->structure(n)->vocabulary()->name() << " {\n";
		indent();
		s->structure(n)->accept(this);
		unindent();
		printtab();
		_out << "}\n";
	} 
	for(unsigned int n = 0; n < s->nrSubs(); ++n) {
		printtab();
		_out << "#namespace " << s->subspace(n)->name() << " {\n"; 
		indent();
		s->subspace(n)->accept(this);
		unindent();
		printtab();
		_out << "}\n";
	}
	//TODO Print procedures and options?
}
*/
