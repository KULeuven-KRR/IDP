/************************************
	idpprinter.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef IDPPRINTER_HPP_
#define IDPPRINTER_HPP_

#include "printers/print.hpp"
#include "ground.hpp"
#include "theory.hpp"
#include "vocabulary.hpp"
#include "ecnf.hpp"
#include "namespace.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/GroundPolicy.hpp"

//TODO is not guaranteed to generate correct idp files!
//TODO usage of stored parameters might be incorrect in some cases.

template<typename Stream>
class IDPPrinter: public StreamPrinter<Stream> {
private:
	bool						_longnames;
	const GroundTranslator*		_translator;
	const GroundTermTranslator*	_termtranslator;

	using StreamPrinter<Stream>::output;
	using StreamPrinter<Stream>::printtab;
	using StreamPrinter<Stream>::unindent;
	using StreamPrinter<Stream>::indent;
	using StreamPrinter<Stream>::isDefClosed;
	using StreamPrinter<Stream>::isDefOpen;
	using StreamPrinter<Stream>::closeDef;
	using StreamPrinter<Stream>::openDef;
	using StreamPrinter<Stream>::isTheoryOpen;
	using StreamPrinter<Stream>::closeTheory;
	using StreamPrinter<Stream>::openTheory;

public:
	IDPPrinter(bool longnames, Stream& stream):
			StreamPrinter<Stream>(stream),
			_longnames(longnames),
			_translator(NULL),
			_termtranslator(NULL){ }

	virtual void setLongNames(bool longnames){ _longnames = longnames; }
	virtual void setTranslator(GroundTranslator* t){ _translator = t; }
	virtual void setTermTranslator(GroundTermTranslator* t){ _termtranslator = t; }

	virtual void startTheory(){
		openTheory();
	}
	virtual void endTheory(){
		closeTheory();
	}

	void visit(const AbstractStructure* structure) {
		assert(isTheoryOpen());
		Vocabulary* voc = structure->vocabulary();

		for(std::map<std::string,std::set<Sort*> >::const_iterator it = voc->firstsort(); it != voc->lastsort(); ++it) {
			for(std::set<Sort*>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
				Sort* s = *jt;
				if(!s->builtin()) {
					output() << *s << " = ";
					SortTable* st = structure->inter(s);
					visit(st);
					output() << '\n';
				}
			}
		}
		for(std::map<std::string,Predicate*>::const_iterator it = voc->firstpred(); it != voc->lastpred(); ++it) {
			std::set<Predicate*> sp = it->second->nonbuiltins();
			for(std::set<Predicate*>::iterator jt = sp.begin(); jt != sp.end(); ++jt) {
				Predicate* p = *jt;
				if(p->arity() != 1 || p->sorts()[0]->pred() != p) {
					PredInter* pi = structure->inter(p);
					if(pi->approxtwovalued()) {
						output() << *p << " = ";
						const PredTable* pt = pi->ct();
						visit(pt);
						output() << '\n';
					}
					else {
						const PredTable* ct = pi->ct();
						output() << *p << "<ct> = ";
						visit(ct);
						output() << '\n';
						const PredTable* cf = pi->cf();
						output() << *p << "<cf> = ";
						visit(cf);
						output() << '\n';
					}
				}
			}
		}
		for(std::map<std::string,Function*>::const_iterator it = voc->firstfunc(); it != voc->lastfunc(); ++it) {
			std::set<Function*> sf = it->second->nonbuiltins();
			for(std::set<Function*>::iterator jt = sf.begin(); jt != sf.end(); ++jt) {
				Function* f = *jt;
				FuncInter* fi = structure->inter(f);
				if(fi->approxtwovalued()) {
					FuncTable* ft = fi->functable();
					output() << *f << " = ";
					visit(ft);
					output() << '\n';
				}
				else {
					PredInter* pi = fi->graphinter();
					const PredTable* ct = pi->ct();
					output() << *f << "<ct> = ";
					printasfunc(ct);
					output() << '\n';
					const PredTable* cf = pi->cf();
					output() << *f << "<cf> = ";
					printasfunc(cf);
					output() << '\n';
				}
			}
		}
	}

	void visit(const Vocabulary* v) {
		assert(isTheoryOpen());
		for(std::map<std::string,std::set<Sort*> >::const_iterator it = v->firstsort(); it != v->lastsort(); ++it) {
			for(std::set<Sort*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
				if(!(*jt)->builtin() || v == Vocabulary::std()) visit(*jt);
			}
		}
		for(std::map<std::string,Predicate*>::const_iterator it = v->firstpred(); it != v->lastpred(); ++it) {
			if(!it->second->builtin() || v == Vocabulary::std()) visit(it->second);
		}
		for(std::map<std::string,Function*>::const_iterator it = v->firstfunc(); it != v->lastfunc(); ++it) {
			if(!it->second->builtin() || v == Vocabulary::std()) visit(it->second);
		}
	}

	void visit(const Namespace* s) {
		assert(isTheoryOpen());
		for(auto i=s->vocabularies().begin(); i!=s->vocabularies().end(); ++i) {
			printtab();
			output() << "#vocabulary " << (*i).second->name() << " {\n";
			indent();
			visit((*i).second);
			unindent();
			printtab();
			output() << "}\n";
		}
		for(auto i=s->theories().begin(); i!=s->theories().end(); ++i) {
			printtab();
			output() << "#theory " << (*i).second->name() <<" : " << (*i).second->vocabulary()->name() << " {\n";
			indent();
			Printer::visit((*i).second);
			unindent();
			printtab();
			output() << "}\n";
		}
		for(auto i=s->structures().begin(); i!=s->structures().end(); ++i) {
			printtab();
			output() << "#structure " << (*i).second->name();
			output() << " : " << (*i).second->vocabulary()->name() << " {\n";
			indent();
			visit((*i).second);
			unindent();
			printtab();
			output() << "}\n";
		}
		for(auto i=s->subspaces().begin(); i!=s->subspaces().end(); ++i) {
			printtab();
			output() << "#namespace " << (*i).second->name() << " {\n";
			indent();
			visit((*i).second);
			unindent();
			printtab();
			output() << "}\n";
		}
	}

	void visit(const GroundFixpDef*) {
		assert(isTheoryOpen());
		/*TODO not implemented yet*/
		output() <<"(printing fixpoint definitions is not yet implemented)\n";
	}

	void visit(const Theory* t) {
		assert(isTheoryOpen());
		for(auto it = t->sentences().begin(); it != t->sentences().end(); ++it) {
			(*it)->accept(this); output() << ".\n";
		}
		for(auto it = t->definitions().begin(); it != t->definitions().end(); ++it) {
			(*it)->accept(this);
		}
		for(auto it = t->fixpdefs().begin(); it != t->fixpdefs().end(); ++it) {
			(*it)->accept(this);
		}
	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		assert(isTheoryOpen());
		_translator = g->translator();
		_termtranslator = g->termtranslator();
		for(unsigned int n = 0; n < g->nrClauses(); ++n) {
			visit(g->clause(n));
		}
		for(unsigned int n = 0; n < g->nrDefinitions(); ++n){
			openDefinition(g->definition(n)->id());
			g->definition(n)->accept(this);
			closeDefinition();
		}
		for(unsigned int n = 0; n < g->nrSets(); ++n){
			g->set(n)->accept(this);
		}
		for(unsigned int n = 0; n < g->nrAggregates(); ++n){
			g->aggregate(n)->accept(this);
		}
		for(unsigned int n = 0; n < g->nrFixpDefs(); ++n){
			g->fixpdef(n)->accept(this);
		}
		for(unsigned int n = 0; n < g->nrCPReifications(); ++n){
			g->cpreification(n)->accept(this);
		}
	}

	/** Formulas **/

	void visit(const PredForm* f) {
		assert(isTheoryOpen());
		if(! f->sign())	output() << "~";
		output() <<f->symbol()->to_string(_longnames);
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

	void visit(const EqChainForm* f) {
		assert(isTheoryOpen());
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

	void visit(const EquivForm* f) {
		assert(isTheoryOpen());
		if(! f->sign())	output() << "~";
		output() << "(";
		f->left()->accept(this);
		output() << " <=> ";
		f->right()->accept(this);
		output() << ")";
	}

	void visit(const BoolForm* f) {
		assert(isTheoryOpen());
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

	void visit(const QuantForm* f) {
		assert(isTheoryOpen());
		if(! f->sign())	output() << "~";
		output() << "(";
		if(f->univ())
			output() << "!";
		else
			output() << "?";
		for(std::set<Variable*>::const_iterator it = f->quantvars().begin(); it != f->quantvars().end(); ++it) {
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

	void visit(const Rule* r) {
		assert(isTheoryOpen());
		printtab();
		if(!r->quantvars().empty()) {
			output() << "!";
			for(std::set<Variable*>::const_iterator it = r->quantvars().begin(); it != r->quantvars().end(); ++it) {
				output() << " " << *(*it);
			}
			output() << " : ";
		}
		r->head()->accept(this);
		output() << " <- ";
		r->body()->accept(this);
		output() << ".";
	}

	void visit(const Definition* d) {
		assert(isTheoryOpen());
		printtab();
		output() << "{\n";
		indent();
		for(std::vector<Rule*>::const_iterator it = d->rules().begin(); it != d->rules().end(); ++it) {
			(*it)->accept(this);
			output() << "\n";
		}
		unindent();
		printtab();
		output() << "}\n";
	}

	void visit(const FixpDef* d) {
		assert(isTheoryOpen());
		printtab();
		output() << (d->lfp() ? "LFD" : "GFD") << " [\n";
		indent();
		for(std::vector<Rule*>::const_iterator it = d->rules().begin(); it != d->rules().end(); ++it) {
			(*it)->accept(this);
			output() << "\n";
		}
		for(std::vector<FixpDef*>::const_iterator it = d->defs().begin(); it != d->defs().end(); ++it) {
			(*it)->accept(this);
		}
		unindent();
		printtab();
		output() << "]\n";
	}

	/** Terms **/

	void visit(const VarTerm* t) {
		assert(isTheoryOpen());
		output() << t->var()->name();
	}

	void visit(const FuncTerm* t) {
		assert(isTheoryOpen());
		output() <<t->function()->to_string(_longnames);
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

	void visit(const DomainTerm* t) {
		assert(isTheoryOpen());
		std::string str = t->value()->to_string();
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

	void visit(const AggTerm* t) {
		assert(isTheoryOpen());
		switch(t->function()) {
			case AGG_CARD: output() << '#'; break;
			case AGG_SUM: output() << "sum"; break;
			case AGG_PROD: output() << "prod"; break;
			case AGG_MIN: output() << "min"; break;
			case AGG_MAX: output() << "max"; break;
		}
		t->set()->accept(this);
	}

	void visit(const GroundClause& g){
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

	void openDefinition(int defid){
		assert(isDefClosed());
		openDef(defid);
		printtab();
		output() << "{\n";
		indent();
	}

	void closeDefinition(){
		assert(!isDefClosed());
		closeDef();
		unindent();
		output() << "}\n";
	}

	void visit(GroundDefinition* d){
		assert(isTheoryOpen());
		for(auto it=d->begin(); it!=d->end(); ++it){
			(*it)->accept(this);
		}
	}

	void visit(const PCGroundRule* b) {
		assert(isTheoryOpen());
		printAtom(b->head());
		output() << " <- ";
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

	void visit(const AggGroundRule* b) {
		assert(isTheoryOpen());
		printAtom(b->head());
		output() << " <- ";
		printAggregate(b->bound(),b->lower(),b->aggtype(),b->setnr());
		output() << ".\n";
	}

	void visit(const GroundAggregate* a) {
		assert(isTheoryOpen());
		printAtom(a->head());
		switch(a->arrow()) {
			case TS_IMPL: 	output() << " => "; break;
			case TS_RIMPL: 	output() << " <= "; break;
			case TS_EQ: 	output() << " <=> "; break;
			case TS_RULE: break;
		}
		printAggregate(a->bound(),a->lower(),a->type(),a->setnr());
	}

	void visit(const CPReification* cpr) {
		assert(isTheoryOpen());
		printAtom(cpr->_head);
		switch(cpr->_body->type()) {
			case TS_RULE: 	output() << " <- "; break;
			case TS_IMPL: 	output() << " => "; break;
			case TS_RIMPL: 	output() << " <= "; break;
			case TS_EQ: 	output() << " <=> "; break;
		}
		cpr->_body->left()->accept(this);
		switch(cpr->_body->comp()) {
			case CT_EQ:		output() << " = "; break;
			case CT_NEQ:	output() << " ~= "; break;
			case CT_LEQ:	output() << " =< "; break;
			case CT_GEQ:	output() << " >= "; break;
			case CT_LT:		output() << " < "; break;
			case CT_GT:		output() << " > "; break;
		}
		CPBound right = cpr->_body->right();
		if(right._isvarid) printTerm(right._varid);
		else output() << right._bound;
		output() << ".\n";
	}

	void visit(const CPSumTerm* cpt) {
		assert(isTheoryOpen());
		output() << "sum[ ";
		for(std::vector<unsigned int>::const_iterator vit = cpt->_varids.begin(); vit != cpt->_varids.end(); ++vit) {
			printTerm(*vit);
			if(*vit != cpt->_varids.back()) output() << "; ";
		}
		output() << " ]";
	}

	void visit(const CPWSumTerm* cpt) {
		assert(isTheoryOpen());
		std::vector<unsigned int>::const_iterator vit;
		std::vector<int>::const_iterator wit;
		output() << "wsum[ ";
		for(vit = cpt->_varids.begin(), wit = cpt->_weights.begin(); vit != cpt->_varids.end() && wit != cpt->_weights.end(); ++vit, ++wit) {
			output() << '('; printTerm(*vit); output() << ',' << *wit << ')';
			if(*vit != cpt->_varids.back()) output() << "; ";
		}
		output() << " ]";
	}

	void visit(const CPVarTerm* cpt) {
		assert(isTheoryOpen());
		printTerm(cpt->_varid);
	}


	void visit(const PredTable* table) {
		assert(isTheoryOpen());
		if(table->approxfinite()) {
			TableIterator kt = table->begin();
			if(table->arity()) {
				output() << "{ ";
				if(kt.hasNext()) {
					ElementTuple tuple = *kt;
					output() << tuple[0]->to_string();
					for(ElementTuple::const_iterator lt = ++tuple.begin(); lt != tuple.end(); ++lt) {
						output() << ',' << (*lt)->to_string();
					}
					++kt;
					for(; kt.hasNext(); ++kt) {
						output() << "; ";
						tuple = *kt;
						output() << tuple[0]->to_string();
						for(ElementTuple::const_iterator lt = ++tuple.begin(); lt != tuple.end(); ++lt) {
							output() << ',' << (*lt)->to_string();
						}
					}
				}
				output() << " }";
			}
			else if(kt.hasNext()) output() << "true";
			else output() << "false";
		}
		else output() << "possibly infinite table";
	}

	void printasfunc(const PredTable* table) {
		assert(isTheoryOpen());
		if(table->approxfinite()) {
			TableIterator kt = table->begin();
			output() << "{ ";
			if(kt.hasNext()) {
				ElementTuple tuple = *kt;
				if(tuple.size() > 1) output() << tuple[0]->to_string();
				for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
					output() << ',' << tuple[n]->to_string();
				}
				output() << "->" << tuple.back()->to_string();
				++kt;
				for(; kt.hasNext(); ++kt) {
					output() << "; ";
					tuple = *kt;
					if(tuple.size() > 1) output() << tuple[0]->to_string();
					for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
						output() << ',' << tuple[n]->to_string();
					}
					output() << "->" << tuple.back()->to_string();
				}
			}
			output() << " }";
		}
		else output() << "possibly infinite table";
	}

	void visit(FuncTable* table) {
		assert(isTheoryOpen());
		std::vector<SortTable*> vst = table->universe().tables();
		vst.pop_back();
		Universe univ(vst);
		if(univ.approxfinite()) {
			TableIterator kt = table->begin();
			if(table->arity() != 0) {
				output() << "{ ";
				if(kt.hasNext()) {
					ElementTuple tuple = *kt;
					output() << tuple[0]->to_string();
					for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
						output() << ',' << tuple[n]->to_string();
					}
					output() << "->" << tuple.back()->to_string();
					++kt;
					for(; kt.hasNext(); ++kt) {
						output() << "; ";
						tuple = *kt;
						output() << tuple[0]->to_string();
						for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
							output() << ',' << tuple[n]->to_string();
						}
						output() << "->" << tuple.back()->to_string();
					}
				}
				output() << " }";
			}
			else if(kt.hasNext()) output() << (*kt)[0]->to_string();
			else output() << "{ }";
		}
		else output() << "possibly infinite table";
	}

	void visit(const Sort* s) {
		assert(isTheoryOpen());
		printtab();
		output() << "type " << s->name();
		if(!(s->parents().empty())) {
			output() << " isa " << (*(s->parents().begin()))->name();
			std::set<Sort*>::const_iterator it = s->parents().begin(); ++it;
			for(; it != s->parents().end(); ++it)
				output() << "," << (*it)->name();
		}
		output() << "\n";
	}

	void visit(const Predicate* p) {
		assert(isTheoryOpen());
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

	void visit(const Function* f) {
		assert(isTheoryOpen());
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

	void visit(SortTable* table) {
		assert(isTheoryOpen());
		SortIterator it = table->sortbegin();
		output() << "{ ";
		if(it.hasNext()) {
			output() << (*it)->to_string();
			++it;
			for(; it.hasNext(); ++it) {
				output() << "; " << (*it)->to_string();
			}
		}
		output() << " }";
	}

	void visit(const GroundSet* s) {
		assert(isTheoryOpen());
		output() << "set_" << s->setnr() << " = [ ";
		for(unsigned int n = 0; n < s->size(); ++n) {
				if(s->weighted()) output() << '(';
				printAtom(s->literal(n));
				if(s->weighted()) output() << ',' << s->weight(n) << ')';
				if(n < s->size()-1) output() << "; ";
		}
		output() << " ]\n";
	}

private:
	void printAtom(int atomnr) {
		if(_translator==NULL){
			assert(false);
			return;
		}

		// The sign of the literal is handled on higher level.
		atomnr = abs(atomnr);
		// Get the atom's symbol from the translator.
		PFSymbol* pfs = _translator->atom2symbol(atomnr);
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
			const std::vector<const DomainElement*>& args = _translator->args(atomnr);
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
			assert(!pfs);
			output() << "tseitin_" << atomnr;
		}
	}

	void printTerm(unsigned int termnr) {
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
			const std::vector<GroundTerm>& args = _termtranslator->args(termnr);
			// Print the arguments.
			if(not args.empty()) {
				output() << "(";
				bool begin = true;
				for(std::vector<GroundTerm>::const_iterator gtit = args.begin(); gtit != args.end(); ++gtit) {
					if(!begin){
						output() << ",";
					}
					begin = false;
					if((*gtit)._isvarid) {
						printTerm((*gtit)._varid);
					} else {
						output() << (*gtit)._domelement->to_string();
					}
				}
				output() << ")";
			}
		} else {
			output() << "var_" << termnr;
	//		CPTsBody* cprelation = _termtranslator->cprelation(varid);
	//		CPReification(1,cprelation).accept(this);
		}
	}

	void printAggregate(double bound, bool lower, AggFunction aggtype, unsigned int setnr) {
		output() << bound << (lower ? " =< " : " >= ");
		switch(aggtype) {
			case AGG_CARD: 	output() << "card("; break;
			case AGG_SUM: 	output() << "sum("; break;
			case AGG_PROD: 	output() << "prod("; break;
			case AGG_MIN: 	output() << "min("; break;
			case AGG_MAX: 	output() << "max("; break;
		}
		output() << "set_" << setnr << ")." <<"\n";
	}
};

#endif /* IDPPRINTER_HPP_ */
