/************************************
	print.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "print.hpp"
#include "common.hpp"
#include "ecnf.hpp"
#include "element.hpp"
#include "namespace.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "options.hpp"
#include "ground.hpp"

using namespace std;

/**************
    Printer
**************/

Printer::Printer() {
	// Set indentation level to zero
	_indent = 0;
}

Printer* Printer::create(InfOptions* opts) {
	switch(opts->_format) {
		case OF_TXT:
			return new SimplePrinter();
		case OF_IDP:
			return new IDPPrinter(opts->_printtypes);
		case OF_ECNF:
			return new EcnfPrinter();
		default:
			assert(false);
	}
}

string Printer::print(const Namespace* n)			{ n->accept(this); return _out.str(); }
string Printer::print(const Vocabulary* v)			{ v->accept(this); return _out.str(); }
string Printer::print(const AbstractTheory* t)		{ t->accept(this); return _out.str(); }
string Printer::print(const AbstractStructure* s)	{ s->accept(this); return _out.str(); }	

void Printer::indent() 		{ _indent++; }
void Printer::unindent()	{ _indent--; }
void Printer::printtab() {
	for(unsigned int n = 0; n < _indent; ++n)
		_out << "  ";
}

/***************
    Theories
***************/

void SimplePrinter::visit(const Theory* t) {
	_out << t->to_string();
}

void IDPPrinter::visit(const Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		printtab();
		t->sentence(n)->accept(this);
		_out << ".\n";
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n)
		t->definition(n)->accept(this);
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n)
		t->fixpdef(n)->accept(this);
}

/** Formulas **/

void IDPPrinter::visit(const PredForm* f) {
	if(! f->sign())	_out << "~";
#ifndef NDEBUG
	_out << f->symb()->to_string();
#else
	string fullname = f->symb()->name();
	_out << fullname.substr(0,fullname.find('/'));
#endif
	if(f->nrSubterms()) {
		_out << "(";
		f->subterm(0)->accept(this);
		for(unsigned int n = 1; n < f->nrSubterms(); ++n) {
			_out << ",";
			f->subterm(n)->accept(this);
		}
		_out << ")";
	}
}

void IDPPrinter::visit(const EqChainForm* f) {
	if(! f->sign())	_out << "~";
	_out << "(";
	f->subterm(0)->accept(this);
	for(unsigned int n = 0; n < f->nrComps(); ++n) {
		switch(f->comp(n)) {
			case '=':
				if(f->compsign(n)) _out << " = ";
				else _out << " ~= ";
				break;
			case '<':
				if(f->compsign(n)) _out << " < ";
				else _out << " >= ";
				break;
			case '>':
				if(f->compsign(n)) _out << " > ";
				else _out << " =< ";
				break;
		}
		f->subterm(n+1)->accept(this);
		if(! f->conj() && n+1 < f->nrComps()) {
			_out << " | ";
			f->subterm(n+1)->accept(this);
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
	if(! f->nrSubforms()) {
		if(f->sign() == f->conj())
			_out << "true";
		else
			_out << "false";
	}
	else {
		if(! f->sign())	_out << "~";
		_out << "(";
		f->subform(0)->accept(this);
		for(unsigned int n = 1; n < f->nrSubforms(); ++n) {
			if(f->conj())
				_out << " & ";
			else
				_out << " | ";
			f->subform(n)->accept(this);
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
	for(unsigned int n = 0; n < f->nrQvars(); ++n) {
		_out << " ";
		_out << f->qvar(n)->name();
		if(f->qvar(n)->sort())
			_out << "[" << f->qvar(n)->sort()->name() << "]";
	}
	_out << " : ";
	f->subform(0)->accept(this);
	_out << ")";
}

/** Definitions **/

void IDPPrinter::visit(const Rule* r) {
	printtab();
	if(r->nrQvars()) {
		_out << "!";
		for(unsigned int n = 0; n < r->nrQvars(); ++n) {
			_out << " " << r->qvar(n)->name();
			if(r->qvar(n)->sort())
				_out << "[" << r->qvar(n)->sort()->name() << "]";
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
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		d->rule(n)->accept(this);
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
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		d->rule(n)->accept(this);
		_out << "\n";
	}
	for(unsigned int n = 0; n < d->nrDefs(); ++n) {
		d->def(n)->accept(this);
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
#ifndef NDEBUG
	_out << t->func()->to_string();
#else
	string fullname = t->func()->name();
	_out << fullname.substr(0,fullname.find('/'));
#endif
	if(t->nrSubterms()) {
		_out << "(";
		t->arg(0)->accept(this);
		for(unsigned int n = 1; n < t->nrSubterms(); ++n) {
			_out << ",";
			t->arg(n)->accept(this);
		}
		_out << ")";
	}
}

void IDPPrinter::visit(const DomainTerm* t) {
	_out << ElementUtil::ElementToString(t->value(),t->type());
}

void IDPPrinter::visit(const AggTerm* t) {
	_out << t->type();
	t->set()->accept(this);
}

/** Sets **/

void IDPPrinter::visit(const EnumSetExpr* s) {
	_out << "[ ";
	if(s->nrSubforms()) {
		s->subform(0)->accept(this);
		for(unsigned int n = 1; n < s->nrSubforms(); ++n) {
			_out << ";";
			s->subform(n)->accept(this);
		}
	}
	_out << " ]";
}

void IDPPrinter::visit(const QuantSetExpr* s) {
	_out << "{";
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		_out << " ";
		_out << s->qvar(n)->name();
		if(s->firstargsort())
			_out << "[" << s->qvar(n)->sort()->name() << "]";
	}
	_out << ": ";
	s->subform(0)->accept(this);
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
		const vector<domelement>& args = _translator->args(atomnr);
		// Print the atom's arguments.
		if(pfs->ispred()) {
			if(not args.empty()) {
				_out << "(";
				for(unsigned int n = 0; n < args.size(); ++n) {
					_out << ElementUtil::ElementToString(args[n]);
					if(n != args.size()-1) _out << ",";
				}
				_out << ")";
			}
		}
		else {
			assert(not pfs->ispred());
			if(args.size() > 1) {
				_out << "(";
				for(unsigned int n = 0; n < args.size()-1; ++n) {
					_out << ElementUtil::ElementToString(args[n]);
					if(n != args.size()-2) _out << ",";
				}
				_out << ")";
			}
			_out << " = " << ElementUtil::ElementToString(args.back());
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
	Function* func = _termtranslator->function(termnr);
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
		const vector<domelement>& args = _termtranslator->args(termnr);
		// Print the arguments.
		if(not args.empty()) {
			_out << "(";
			for(unsigned int c = 0; c < args.size(); ++c) {
				_out << ElementUtil::ElementToString(args[c]);
				if(c != args.size()-1) _out << ",";
			}
			_out << ")";
		}
	}
}

void IDPPrinter::printAggregate(double bound, bool lower, AggType aggtype, unsigned int setnr) {
	_out << bound << (lower ? " =< " : " >= ");
	switch(aggtype) {
		case AGGCARD: 	_out << "card("; break;
		case AGGSUM: 	_out << "sum("; break;
		case AGGPROD: 	_out << "prod("; break;
		case AGGMIN: 	_out << "min("; break;
		case AGGMAX: 	_out << "max("; break;
		default: assert(false);
	}
	_out << "set_" << setnr << ")." << endl;
}

void EcnfPrinter::printAggregate(AggType aggtype, TsType arrow, unsigned int defnr, bool lower, int head, unsigned int setnr, double bound) {
	switch(aggtype) {
		case AGGCARD: 	_out << "Card "; break;
		case AGGSUM: 	_out << "Sum "; break;
		case AGGPROD: 	_out << "Prod "; break;
		case AGGMIN: 	_out << "Min "; break;
		case AGGMAX: 	_out << "Max "; break;
		default: assert(false);
	}
	#warning "Replacing implication by equivalence...";
	switch(arrow) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL:  /* (Reverse) implication is not supported by solver (yet)*/
			_out << "C ";
			break;
		case TS_RULE: 
			_out << "<- " << defnr << ' ';
			break; 
		default: assert(false);
	}
	_out << (lower ? 'G' : 'L') << ' ' << head << ' ' << setnr << ' ' << bound << " 0" << endl;
}

void SimplePrinter::visit(const GroundTheory* g) {
	_out << g->to_string();
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
	if(right._isvarid) printTerm(right._value._varid);
	else _out << right._value._bound;
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
		Function* function = _termtranslator->function(varid);
		SortTable* domain = _structure->inter(function->outsort());
		int minvalue = domain->element(0)._int;
		int maxvalue = domain->element(domain->size()-1)._int;
		unsigned int difference = maxvalue - minvalue;
		if(difference + 1 == domain->size()) {
			_out << "INTVAR " << varid << ' ' << minvalue << ' ' << maxvalue << ' ';
		} else {
			_out << "INTVARDOM " << varid << ' ';
			for(unsigned int n = 0; n < domain->size(); ++n) {
				int value = domain->element(n)._int;
				_out << value << ' ';
			}
		}
		_out << '0' << endl;
	}
}

void EcnfPrinter::printCPReification(string type, int head, unsigned int left, CompType comp, int right) {
	#warning "Might be dangerous to cast varids to bounds (unsigned int to int)";
	_out << type << ' ' << head << ' ' << left << ' ' << comp << ' ' << right << " 0" << endl;
}

void EcnfPrinter::printCPReification(string type, int head, vector<unsigned int> left, CompType comp, int right) {
	#warning "Might be dangerous to cast varids to bounds (unsigned int to int)";
	_out << type << ' ' << head << ' ';
	for(vector<unsigned int>::const_iterator it = left.begin(); it != left.end(); ++it)
		_out << *it << ' ';
	_out << comp << ' ' << right << " 0" << endl;
}

void EcnfPrinter::printCPReification(string type, int head, vector<unsigned int> left, vector<int> weights, CompType comp, int right) {
	#warning "Might be dangerous to cast varids to bounds (unsigned int to int)";
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
			printCPVariable(right._value._varid);
			printCPReification("BINTRT",cpr->_head,term->_varid,comp,right._value._varid);
		}
		else { // CPBinaryRel
			printCPReification("BINTRI",cpr->_head,term->_varid,comp,right._value._bound);
		}
	}
	else if(typeid(*left) == typeid(CPSumTerm)) {
		CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
		printCPVariables(term->_varids);
		if(right._isvarid) { // CPSumWithVar
			printCPVariable(right._value._varid);
			printCPReification("SUMSTRT",cpr->_head,term->_varids,comp,right._value._varid);
		}
		else { // CPSum
			printCPReification("SUMSTRI",cpr->_head,term->_varids,comp,right._value._bound);
		}
	}
	else {
		assert(typeid(*left) == typeid(CPWSumTerm));
		CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
		printCPVariables(term->_varids);
		if(right._isvarid) { // CPSumWeightedWithVar
			printCPVariable(right._value._varid);
			printCPReification("SUMSTSIRT",cpr->_head,term->_varids,term->_weights,comp,right._value._varid);
		}
		else { // CPSumWeighted
			printCPReification("SUMSTSIRI",cpr->_head,term->_varids,term->_weights,comp,right._value._bound);
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

void SimplePrinter::visit(const Structure* s) {
	_out << s->to_string();
}

void IDPPrinter::visit(const Structure* s) {
	_currentstructure = s;
	Vocabulary* v = s->vocabulary();
	for(unsigned int n = 0; n < v->nrNBPreds(); ++n) {
		_currentsymbol = v->nbpred(n);
		if(_printtypes || _currentsymbol->nrSorts() != 1 || _currentsymbol != _currentsymbol->sort(0)->pred()) {
			s->inter(v->nbpred(n))->accept(this);
		}
	}
	for(unsigned int n = 0; n < v->nrNBFuncs(); ++n) {
		_currentsymbol = v->nbfunc(n);
		s->inter(v->nbfunc(n))->accept(this);
	}
}

void IDPPrinter::visit(const SortTable* t) {
	for(unsigned int n = 0; n < t->size(); ++n) {
		_out << ElementUtil::ElementToString(t->element(n),t->type());
		if(n < t->size()-1)
			_out << ";";
	}
}

void IDPPrinter::print(const PredTable* t) {
	for(unsigned int r = 0; r < t->size(); ++r) {
		for(unsigned int c = 0; c < t->arity(); ++c) {
			_out << ElementUtil::ElementToString(t->element(r,c),t->type(c));
			if(c < t->arity()-1) {
				if(not(_currentsymbol->ispred()) && c == t->arity()-2)
					_out << "->";
				else
					_out << ",";
			}
		}
		if(r < t->size()-1)
			_out << "; ";
	}
}

void IDPPrinter::printInter(const char* pt1name,const char* pt2name,const PredTable* pt1,const PredTable* pt2) {
	string fullname = _currentsymbol->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	printtab();
	_out << shortname << "<" << pt1name << "> = { ";
	if(pt1)
		print(pt1);
	_out << " }\n";
	printtab();
	_out << shortname << "<" << pt2name << "> = { ";
	if(pt2)
		print(pt2);
	_out << " }\n";
}

void IDPPrinter::visit(const PredInter* p) {
	string fullname = _currentsymbol->name();
	string shortname = fullname.substr(0,fullname.find('/'));
	if(_currentsymbol->nrSorts() == 0) { // proposition
		printtab();
		_out << shortname << " = "; 
		if(p->ct() != p->ctpf()->empty()) {
			assert(p->cf() == p->cfpt()->empty());
			_out << "true";
		}
		else if(p->cf() != p->cfpt()->empty()) {
			assert(p->ct() == p->ctpf()->empty());
			_out << "false";
		}
		else _out << "unknown";
		_out << "\n";	
	}
	else if(!_currentsymbol->ispred() && _currentsymbol->nrSorts() == 1) { // constant
		printtab();
		_out << shortname << " = ";
		print(p->ctpf());
		_out << "\n";
	}
	else if(p->ctpf() == p->cfpt()) {
		if(p->ct() && p->cf()) { // impossible
			assert(false);
		}
		else if(p->ct()) { // p = ctpf
			printtab();
			_out << shortname << " = { ";
			print(p->ctpf());
			_out << " }\n";
		}
		else if(p->cf()) { // p[cf] = cfpt and p[u] = comp(cfpt)
			PredTable* u = StructUtils::complement(p->cfpt(),_currentsymbol->sorts(),_currentstructure);
			printInter("cf","u",p->cfpt(),u);
			delete(u);
		}
		else { // p[ct] = {} and p[cf] = {}
			printInter("ct","cf",NULL,NULL);
		}
	}
	else {
		if(p->ct() && p->cf()) { // p[ct] = { ctpf } and p[cf] = { cfpt }
			printInter("ct","cf",p->ctpf(),p->cfpt());
		}
		else if(p->ct()) { // p[ct] = { ctpf } and p[u] = { cfpt \ ctpf }
			PredTable* u = TableUtils::difference(p->cfpt(),p->ctpf());
			printInter("ct","u",p->ctpf(),u);
			delete(u);
		}
		else if(p->cf()) { // p[cf] = { cfpt } and p[u] = { ctpf \ cfpt }
			PredTable* u = TableUtils::difference(p->ctpf(),p->cfpt());
			printInter("cf","u",p->cfpt(),u);
			delete(u);
		}
		else { // p[ct] = {} and p[cf] = {}
			printInter("ct","cf",NULL,NULL);
		}
	}
}

void IDPPrinter::visit(const FuncInter* f) {
	//TODO Currently, function interpretation is handled as predicate interpretation
	f->predinter()->accept(this);	
}

/*******************
    Vocabularies
*******************/

void SimplePrinter::visit(const Vocabulary* v) {
	_out << v->to_string();
}

void IDPPrinter::visit(const Vocabulary* v) {
	traverse(v);
}

void IDPPrinter::visit(const Sort* s) {
	printtab();
	_out << "type " << s->name();
	if(s->nrParents() > 0)
		_out << " isa " << s->parent(0)->name();
		for(unsigned int n = 1; n < s->nrParents(); ++n)
			_out << "," << s->parent(n)->name();
	_out << "\n";
}

void IDPPrinter::visit(const Predicate* p) {
	printtab();
	_out << p->name().substr(0,p->name().find('/'));
	if(p->arity() > 0) {
		_out << "(" << p->sort(0)->name();
		for(unsigned int n = 1; n < p->arity(); ++n)
			_out << "," << p->sort(n)->name();
		_out << ")";
	}
	_out << "\n";
}

void IDPPrinter::visit(const Function* f) {
	printtab();
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

/*****************
	Namespaces
*****************/

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
