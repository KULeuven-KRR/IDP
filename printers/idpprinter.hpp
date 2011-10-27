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
	using StreamPrinter<Stream>::printTab;
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

		for(auto it = voc->firstSort(); it != voc->lastSort(); ++it) {
			for(auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
				Sort* s = *jt;
				if(not s->builtin()) {
					output() << s->toString(_longnames) << " = ";
					SortTable* st = structure->inter(s);
					visit(st);
					output() << '\n';
				}
			}
		}
		for(auto it = voc->firstPred(); it != voc->lastPred(); ++it) {
			std::set<Predicate*> sp = it->second->nonbuiltins();
			for(auto jt = sp.cbegin(); jt != sp.cend(); ++jt) {
				Predicate* p = *jt;
				if(p->arity() != 1 || p->sorts()[0]->pred() != p) {
					PredInter* pi = structure->inter(p);
					if(pi->approxTwoValued()) {
						output() << p->toString(_longnames) << " = ";
						const PredTable* pt = pi->ct();
						visit(pt);
						output() << '\n';
					}
					else {
						const PredTable* ct = pi->ct();
						output() << p->toString(_longnames) << "<ct> = ";
						visit(ct);
						output() << '\n';
						const PredTable* cf = pi->cf();
						output() << p->toString(_longnames) << "<cf> = ";
						visit(cf);
						output() << '\n';
					}
				}
			}
		}
		for(auto it = voc->firstFunc(); it != voc->lastFunc(); ++it) {
			std::set<Function*> sf = it->second->nonbuiltins();
			for(auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
				Function* f = *jt;
				FuncInter* fi = structure->inter(f);
				if(fi->approxTwoValued()) {
					FuncTable* ft = fi->funcTable();
					output() << f->toString(_longnames) << " = ";
					visit(ft);
					output() << '\n';
				}
				else {
					PredInter* pi = fi->graphInter();
					const PredTable* ct = pi->ct();
					output() << f->toString(_longnames) << "<ct> = ";
					printAsFunc(ct);
					output() << '\n';
					const PredTable* cf = pi->cf();
					output() << f->toString(_longnames) << "<cf> = ";
					printAsFunc(cf);
					output() << '\n';
				}
			}
		}
	}

	void visit(const Vocabulary* v) {
		assert(isTheoryOpen());
		for(auto it = v->firstSort(); it != v->lastSort(); ++it) {
			for(auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
				if(not (*jt)->builtin() || v == Vocabulary::std()) { visit(*jt); }
			}
		}
		for(auto it = v->firstPred(); it != v->lastPred(); ++it) {
			if(not it->second->builtin() || v == Vocabulary::std()) { visit(it->second); }
		}
		for(auto it = v->firstFunc(); it != v->lastFunc(); ++it) {
			if(not it->second->builtin() || v == Vocabulary::std()) { visit(it->second); }
		}
	}

	void visit(const Namespace* s) {
		assert(isTheoryOpen());
		for(auto i = s->vocabularies().cbegin(); i != s->vocabularies().cend(); ++i) {
			printTab();
			output() << "vocabulary " << (*i).second->name() << " {\n";
			indent();
			visit((*i).second);
			unindent();
			printTab();
			output() << "}\n";
		}
		for(auto i = s->theories().cbegin(); i != s->theories().cend(); ++i) {
			printTab();
			output() << "theory " << (*i).second->name() <<" : " << (*i).second->vocabulary()->name() << " {\n";
			indent();
			Printer::visit((*i).second);
			unindent();
			printTab();
			output() << "}\n";
		}
		for(auto i = s->structures().cbegin(); i != s->structures().cend(); ++i) {
			printTab();
			output() << "structure " << (*i).second->name();
			output() << " : " << (*i).second->vocabulary()->name() << " {\n";
			indent();
			visit((*i).second);
			unindent();
			printTab();
			output() << "}\n";
		}
		for(auto i = s->subspaces().cbegin(); i != s->subspaces().cend(); ++i) {
			printTab();
			output() << "namespace " << (*i).second->name() << " {\n";
			indent();
			visit((*i).second);
			unindent();
			printTab();
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
		for(auto it = t->sentences().cbegin(); it != t->sentences().cend(); ++it) {
			(*it)->accept(this); output() << ".\n";
		}
		for(auto it = t->definitions().cbegin(); it != t->definitions().cend(); ++it) {
			(*it)->accept(this);
		}
		for(auto it = t->fixpdefs().cbegin(); it != t->fixpdefs().cend(); ++it) {
			(*it)->accept(this);
		}
	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		assert(isTheoryOpen());
		_translator = g->translator();
		_termtranslator = g->termtranslator();
		for(size_t n = 0; n < g->nrClauses(); ++n) {
			visit(g->clause(n));
		}
		for(auto i=g->definitions().cbegin(); i!=g->definitions().cend(); i++){
			openDefinition((*i).second->id());
			(*i).second->accept(this);
			closeDefinition();
		}

		for(size_t n = 0; n < g->nrSets(); ++n){
			g->set(n)->accept(this);
		}
		for(size_t n = 0; n < g->nrAggregates(); ++n){
			g->aggregate(n)->accept(this);
		}
		for(size_t n = 0; n < g->nrFixpDefs(); ++n){
			g->fixpdef(n)->accept(this);
		}
		for(size_t n = 0; n < g->nrCPReifications(); ++n){
			g->cpreification(n)->accept(this);
		}
	}

	/** Formulas **/

	void visit(const PredForm* f) {
		assert(isTheoryOpen());
		if(isNeg(f->sign()))	output() << "~";
		output() <<f->symbol()->toString(_longnames);
		if(not f->subterms().empty()) {
			output() << "(";
			f->subterms()[0]->accept(this);
			for(size_t n = 1; n < f->subterms().size(); ++n) {
				output() << ',';
				f->subterms()[n]->accept(this);
			}
			output() << ')';
		}
	}

	void visit(const EqChainForm* f) {
		assert(isTheoryOpen());
		if(isNeg(f->sign())) { output() << "~"; }
		output() << "(";
		f->subterms()[0]->accept(this);
		for(size_t n = 0; n < f->comps().size(); ++n) {
			output() << ' ' << f->comps()[n] << ' ';
			f->subterms()[n+1]->accept(this);
			if(not f->conj() && (n+1 < f->comps().size())) {
				output() << " | ";
				f->subterms()[n+1]->accept(this);
			}
		}
		output() << ')';
	}

	void visit(const EquivForm* f) {
		assert(isTheoryOpen());
		if(isNeg(f->sign())){
			output() << "~";
		}
		output() << "(";
		f->left()->accept(this);
		output() << " <=> ";
		f->right()->accept(this);
		output() << ')';
	}

	void visit(const BoolForm* f) {
		assert(isTheoryOpen());
		if(f->subformulas().empty()) {
			if(f->isConjWithSign()){
				output() << "true";
			} else{
				output() << "false";
			}
		} else {
			if(isNeg(f->sign())){
				output() << "~";
			}
			output() << "(";
			f->subformulas()[0]->accept(this);
			for(size_t n = 1; n < f->subformulas().size(); ++n) {
				output() << (f->conj() ? " & " : " | ");
				f->subformulas()[n]->accept(this);
			}
			output() << ')';
		}
	}

	void visit(const QuantForm* f) {
		assert(isTheoryOpen());
		if(isNeg(f->sign())){
			output() << "~";
		}
		output() << "(";
		if(f->isUniv()){
			output() << "!";
		}else{
			output() << "?";
		}
		for(auto it = f->quantVars().cbegin(); it != f->quantVars().cend(); ++it) {
			output() << " ";
			output() << (*it)->name();
			if((*it)->sort()) { output() << '[' << (*it)->sort()->name() << ']'; }
		}
		output() << " : ";
		f->subformulas()[0]->accept(this);
		output() << ')';
	}

	void visit(const AggForm* f) {
		if(isNeg(f->sign())) { output() << '~'; }
		output() << '(';
		f->left()->accept(this);
		output() << ' ' << f->comp() << ' ';
		f->right()->accept(this);
		output() << ')';
	}


	/** Definitions **/

	void visit(const Rule* r) {
		assert(isTheoryOpen());
		printTab();
		if(not r->quantVars().empty()) {
			output() << "!";
			for(auto it = r->quantVars().cbegin(); it != r->quantVars().cend(); ++it) {
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
		printTab();
		output() << "{\n";
		indent();
		for(auto it = d->rules().cbegin(); it != d->rules().cend(); ++it) {
			(*it)->accept(this);
			output() << "\n";
		}
		unindent();
		printTab();
		output() << "}\n";
	}

	void visit(const FixpDef* d) {
		assert(isTheoryOpen());
		printTab();
		output() << (d->lfp() ? "LFD" : "GFD") << " [\n";
		indent();
		for(auto it = d->rules().cbegin(); it != d->rules().cend(); ++it) {
			(*it)->accept(this);
			output() << "\n";
		}
		for(auto it = d->defs().cbegin(); it != d->defs().cend(); ++it) {
			(*it)->accept(this);
		}
		unindent();
		printTab();
		output() << "]\n";
	}

	/** Terms **/

	void visit(const VarTerm* t) {
		assert(isTheoryOpen());
		output() << t->var()->name();
	}

	void visit(const FuncTerm* t) {
		assert(isTheoryOpen());
		output() << t->function()->toString(_longnames);
		if(not t->subterms().empty()) {
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
		std::string str = t->value()->toString();
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
			case AggFunction::CARD: output() << '#'; break;
			case AggFunction::SUM: output() << "sum"; break;
			case AggFunction::PROD: output() << "prod"; break;
			case AggFunction::MIN: output() << "min"; break;
			case AggFunction::MAX: output() << "max"; break;
		}
		t->set()->accept(this);
	}

	/** Set expressions **/

	void visit(const EnumSetExpr* s) {
		output() << "[ ";
		if(not s->subformulas().empty()) {
			s->subformulas()[0]->accept(this);
			for(unsigned int n = 1; n < s->subformulas().size(); ++n) {
				output() << ";";
				s->subformulas()[n]->accept(this);
			}
		}
		output() << " ]";
	}
	
	void visit(const QuantSetExpr* s) {
		output() << '{';
		for(auto it = s->quantVars().cbegin(); it != s->quantVars().cend(); ++it) {
			output() << ' ';
			output() << (*it)->name();
			if((*it)->sort()) { output() << '[' << (*it)->sort()->name() << ']'; }
		}
		output() << " : ";
		s->subformulas()[0]->accept(this);
		if(not s->subterms().empty()) {
			output() << " : ";
			s->subterms()[0]->accept(this);
		}
		output() << " }";
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
		printTab();
		output() << "{\n";
		indent();
	}

	void closeDefinition(){
		assert(not isDefClosed());
		closeDef();
		unindent();
		output() << "}\n";
	}

	void visit(GroundDefinition* d){
		assert(isTheoryOpen());
		for(auto it=d->begin(); it!=d->end(); ++it) {
			(*it).second->accept(this);
		}
	}

	void visit(const PCGroundRule* b) {
		assert(isTheoryOpen());
		printAtom(b->head());
		output() << " <- ";
		char c = (b->type() == RT_CONJ ? '&' : '|');
		if(not b->empty()) {
			for(unsigned int n = 0; n < b->size(); ++n) {
				if(b->literal(n) < 0) { output() << '~'; }
				printAtom(b->literal(n));
				if(n != b->size()-1) { output() << ' ' << c << ' '; }
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
			case TsType::IMPL: 	output() << " => "; break;
			case TsType::RIMPL: 	output() << " <= "; break;
			case TsType::EQ: 	output() << " <=> "; break;
			case TsType::RULE: break;
		}
		printAggregate(a->bound(),a->lower(),a->type(),a->setnr());
	}

	void visit(const CPReification* cpr) {
		assert(isTheoryOpen());
		printAtom(cpr->_head);
		switch(cpr->_body->type()) {
			case TsType::RULE: 	output() << " <- "; break;
			case TsType::IMPL: 	output() << " => "; break;
			case TsType::RIMPL: 	output() << " <= "; break;
			case TsType::EQ: 	output() << " <=> "; break;
		}
		cpr->_body->left()->accept(this);
		switch(cpr->_body->comp()) {
			case CompType::EQ:		output() << " = "; break;
			case CompType::NEQ:	output() << " ~= "; break;
			case CompType::LEQ:	output() << " =< "; break;
			case CompType::GEQ:	output() << " >= "; break;
			case CompType::LT:		output() << " < "; break;
			case CompType::GT:		output() << " > "; break;
		}
		CPBound right = cpr->_body->right();
		if(right._isvarid) printTerm(right._varid);
		else output() << right._bound;
		output() << ".\n";
	}

	void visit(const CPSumTerm* cpt) {
		assert(isTheoryOpen());
		output() << "sum[ ";
		for(auto vit = cpt->varids().cbegin(); vit != cpt->varids().cend(); ++vit) {
			printTerm(*vit);
			if(*vit != cpt->varids().back()) output() << "; ";
		}
		output() << " ]";
	}

	void visit(const CPWSumTerm* cpt) {
		assert(isTheoryOpen());
		output() << "wsum[ ";
		auto vit = cpt->varids().cbegin();
		auto wit = cpt->weights().cbegin();
		for(; vit != cpt->varids().cend() && wit != cpt->weights().cend(); ++vit, ++wit) {
			output() << '('; printTerm(*vit); output() << ',' << *wit << ')';
			if(*vit != cpt->varids().back()) output() << "; ";
		}
		output() << " ]";
	}

	void visit(const CPVarTerm* cpt) {
		assert(isTheoryOpen());
		printTerm(cpt->varid());
	}

	void visit(const PredTable* table) {
		assert(isTheoryOpen());
		if(table->approxFinite()) {
			TableIterator kt = table->begin();
			if(table->arity()) {
				output() << "{ ";
				if(not kt.isAtEnd()) {
					ElementTuple tuple = *kt;
					output() << tuple[0]->toString();
					for(auto lt = ++tuple.cbegin(); lt != tuple.cend(); ++lt) {
						output() << ',' << (*lt)->toString();
					}
					++kt;
					for(; not kt.isAtEnd(); ++kt) {
						output() << "; ";
						tuple = *kt;
						output() << tuple[0]->toString();
						for(auto lt = ++tuple.cbegin(); lt != tuple.cend(); ++lt) {
							output() << ',' << (*lt)->toString();
						}
					}
				}
				output() << " }";
			}
			else if(not kt.isAtEnd()) output() << "true";
			else output() << "false";
		}
		else output() << "possibly infinite table";
	}

	void printasfunc(const PredTable* table) {
		assert(isTheoryOpen());
		if(table->approxFinite()) {
			TableIterator kt = table->begin();
			output() << "{ ";
			if(not kt.isAtEnd()) {
				ElementTuple tuple = *kt;
				if(tuple.size() > 1) { output() << tuple[0]->toString(); }
				for(size_t n = 1; n < tuple.size() - 1; ++n) {
					output() << ',' << tuple[n]->toString();
				}
				output() << "->" << tuple.back()->toString();
				++kt;
				for(; not kt.isAtEnd(); ++kt) {
					output() << "; ";
					tuple = *kt;
					if(tuple.size() > 1) { output() << tuple[0]->toString(); }
					for(size_t n = 1; n < tuple.size() - 1; ++n) {
						output() << ',' << tuple[n]->toString();
					}
					output() << "->" << tuple.back()->toString();
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
		if(univ.approxFinite()) {
			TableIterator kt = table->begin();
			if(table->arity() != 0) {
				output() << "{ ";
				if(not kt.isAtEnd()) {
					ElementTuple tuple = *kt;
					output() << tuple[0]->toString();
					for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
						output() << ',' << tuple[n]->toString();
					}
					output() << "->" << tuple.back()->toString();
					++kt;
					for(; not kt.isAtEnd(); ++kt) {
						output() << "; ";
						tuple = *kt;
						output() << tuple[0]->toString();
						for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
							output() << ',' << tuple[n]->toString();
						}
						output() << "->" << tuple.back()->toString();
					}
				}
				output() << " }";
			}
			else if(not kt.isAtEnd()) output() << (*kt)[0]->toString();
			else output() << "{ }";
		}
		else output() << "possibly infinite table";
	}

	void visit(const Sort* s) {
		assert(isTheoryOpen());
		printTab();
		output() << "type " << s->name();
		if(not s->parents().empty()) {
			output() << " isa " << (*(s->parents().cbegin()))->name();
			std::set<Sort*>::const_iterator it = s->parents().cbegin(); ++it;
			for(; it != s->parents().cend(); ++it)
				output() << "," << (*it)->name();
		}
		output() << "\n";
	}

	void visit(const Predicate* p) {
		assert(isTheoryOpen());
		printTab();
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
		printTab();
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
		SortIterator it = table->sortBegin();
		output() << "{ ";
		if(not it.isAtEnd()) {
			output() << (*it)->toString();
			++it;
			for(; not it.isAtEnd(); ++it) {
				output() << "; " << (*it)->toString();
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

		if(not _translator->isInputAtom(atomnr)){
			output() << "tseitin_" << atomnr;
			return;
		}

		PFSymbol* pfs = _translator->getSymbol(atomnr);

		// Print the symbol's name.
		output() << pfs->name().substr(0,pfs->name().find('/'));
		// Print the symbol's sorts.
		if(pfs->nrSorts()) {
			output() << '[';
			for(size_t n = 0; n < pfs->nrSorts(); ++n) {
				if(pfs->sort(n)) {
					output() << pfs->sort(n)->name();
					if(n != pfs->nrSorts()-1) { output() << ','; }
				}
			}
			output() << ']';
		}
		// Get the atom's arguments for the translator.
		const std::vector<const DomainElement*>& args = _translator->getArgs(atomnr);
		// Print the atom's arguments.
		if(typeid(*pfs) == typeid(Predicate)) {
			if(not args.empty()) {
				output() << "(";
				for(size_t n = 0; n < args.size(); ++n) {
					output() << args[n]->toString();
					if(n != args.size()-1) { output() << ","; }
				}
				output() << ")";
			}
		}
		else {
			assert(typeid(*pfs) == typeid(Function));
			if(args.size() > 1) {
				output() << "(";
				for(size_t n = 0; n < args.size()-1; ++n) {
					output() << args[n]->toString();
					if(n != args.size()-2) { output() << ","; }
				}
				output() << ")";
			}
			output() << " = " << args.back()->toString();
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
				for(size_t n = 0; n < func->nrSorts(); ++n) {
					if(func->sort(n)) {
						output() << func->sort(n)->name();
						if(n != func->nrSorts()-1) { output() << ','; }
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
				for(auto gtit = args.cbegin(); gtit != args.cend(); ++gtit) {
					if(not begin){
						output() << ",";
					}
					begin = false;
					if((*gtit).isVariable) {
						printTerm((*gtit)._varid);
					} else {
						output() << (*gtit)._domelement->toString();
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
			case AggFunction::CARD: 	output() << "card("; break;
			case AggFunction::SUM: 	output() << "sum("; break;
			case AggFunction::PROD: 	output() << "prod("; break;
			case AggFunction::MIN: 	output() << "min("; break;
			case AggFunction::MAX: 	output() << "max("; break;
		}
		output() << "set_" << setnr << ")." <<"\n";
	}

	void printAsFunc(const PredTable* table) {
		if(table->approxFinite()) {
			TableIterator kt = table->begin();
			output() << "{ ";
			if(not kt.isAtEnd()) {
				ElementTuple tuple = *kt;
				if(tuple.size() > 1) output() << tuple[0]->toString();
				for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
					output() << ',' << tuple[n]->toString();
				}
				output() << "->" << tuple.back()->toString();
				++kt;
				for(; not kt.isAtEnd(); ++kt) {
					output() << "; ";
					tuple = *kt;
					if(tuple.size() > 1) output() << tuple[0]->toString();
					for(unsigned int n = 1; n < tuple.size() - 1; ++n) {
						output() << ',' << tuple[n]->toString();
					}
					output() << "->" << tuple.back()->toString();
				}
			}
			output() << " }";
		}
		else output() << "possibly infinite table";
	}

};

#endif /* IDPPRINTER_HPP_ */
