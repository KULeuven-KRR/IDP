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

#pragma once

#include "printers/print.hpp"
#include "IncludeComponents.hpp"
#include "printers/bddprinter.hpp"


#include "groundtheories/GroundTheory.hpp"
#include "theory/Query.hpp"
#include "internalargument.hpp" // Only for UserProcedure
#include "groundtheories/GroundPolicy.hpp"

#include "utils/StringUtils.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

//TODO is not guaranteed to generate correct idp files!
// FIXME do we want this? Because printing cp constraints etc. should be done correctly then!
//TODO usage of stored parameters might be incorrect in some cases.

template<typename Stream>
class IDPPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	const GroundTranslator* _translator;
	bool _printTermsAsBlock;
	bool _printSetTerm;

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
	IDPPrinter(Stream& stream)
			: 	StreamPrinter<Stream>(stream),
				_translator(NULL),
				_printTermsAsBlock(true),
				_printSetTerm(true) {
	}

	virtual void setTranslator(GroundTranslator* t) {
		_translator = t;
	}

	virtual void startTheory() {
		openTheory();
	}
	virtual void endTheory() {
		closeTheory();
	}

	template<typename T>
		std::string printFullyQualified(T o) const {
	    	std::stringstream ss;
	        ss << o->nameNoArity();
	           	if (o->sorts().size() > 0) {
	           		ss << "[";
	                printList(ss, o->sorts(), ",", not o->isFunction());
	                if (o->isFunction()) {
	                  	ss << ":";
	                    ss << print(*o->sorts().back());
	                }
	                ss << "]";
	            }
	        return ss.str();
	    }

	void visit(const Structure* structure) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());

		if (not structure->isConsistent()) {
			output() << "INCONSISTENT STRUCTURE";
			return;
		}

		auto voc = structure->vocabulary();

		printTab();
		output() << "structure " << structure->name() << " : " << voc->name() << " {" << '\n';
		indent();

		for (auto it = voc->firstSort(); it != voc->lastSort(); ++it) {
			auto s = it->second;
			if (not s->builtin() && not s->isConstructed()) {
				printTab();
				output() << print(s) << " = ";
				auto st = structure->inter(s);
				visit(st);
				output() << '\n';
			}
		}
		for (auto it = voc->firstPred(); it != voc->lastPred(); ++it) {
			auto sp = it->second->nonbuiltins();
			for (auto jt = sp.cbegin(); jt != sp.cend(); ++jt) {
				auto p = *jt;
				if (PredUtils::isTypePredicate(p)) { // If it is in fact a sort, ignore it
					continue;
				}
				auto pi = structure->inter(p);
				if (pi->approxTwoValued()) {
					printTab();
					output() << print(p) << " = ";
					visit(pi->ct());
					output() << '\n';
				} else {
					printTab();
					output() << print(p) << "<ct> = ";
					visit(pi->ct());
					output() << '\n';

					printTab();
					output() << print(p) << "<cf> = ";
					visit(pi->cf());
					output() << '\n';
				}
			}
		}
		for (auto it = voc->firstFunc(); it != voc->lastFunc(); ++it) {
			auto sf = it->second->nonbuiltins();
			for (auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
				auto f = *jt;
				auto fi = structure->inter(f);
				if (fi->approxTwoValued()) {
					auto ft = fi->funcTable();
					printTab();
					output() << print(f) << " = ";
					visit(ft);
					output() << '\n';
				} else {
					auto pi = fi->graphInter();
					printTab();
					output() << print(f) << "<ct> = ";
					printAsFunc(pi->ct());
					output() << '\n';

					printTab();
					output() << print(f) << "<cf> = ";
					printAsFunc(pi->cf());
					output() << '\n';
				}
			}
		}
		unindent();
		printTab();
		output() << "}" << '\n';
		_printTermsAsBlock = backup;
	}

	void visit(const Query* q) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;

		Assert(isTheoryOpen());

		auto voc = q->vocabulary();

		printTab();
		output() << "query " << q->name() << " : " << voc->name() << " {\n";
		indent();
		printTab();
		output() << "{";
		for (auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
			output() << ' ';
			output() << (*it)->name();
			if ((*it)->sort()) {
				output() << '[' << (*it)->sort()->name() << ']';
			}
		}
		output() << " : ";
		q->query()->accept(this);
		output() << "}" << '\n';
		unindent();
		printTab();
		output() << "}" << '\n';
		_printTermsAsBlock = backup;
	}
	void visit(const FOBDD* b) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printTab();
		output() << "fobdd " << " :  {\n";
		indent();
		output() << print(b);
		unindent();
		printTab();
		output() << '\n' << "}" << '\n';
		_printTermsAsBlock = backup;
	}
	void visit(const Compound* a) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << *(a->function());
		if (a->function()->arity() > 0) {
			output() << '(' << *a->arg(0);
			for (size_t n = 1; n < a->function()->arity(); ++n) {
				output()<< ',' << *a->arg(n);
			}
			output() << ')';
		}
		_printTermsAsBlock = backup;
	}

	void visit(const Vocabulary* v) {
		printTab();
		output() << "vocabulary " << v->name() << " {" << '\n';
		indent();
		Assert(isTheoryOpen());
		std::vector<Sort*> printedsorts;
		for (auto it = v->firstSort(); it != v->lastSort(); ++it) {
			printSortRecursively(it->second,printedsorts,v);
		}
		auto symbols = v->getNonBuiltinNonOverloadedSymbols();
		for (auto symbol : symbols) {
			if (isa<Predicate>(*symbol)) {
				auto pred = dynamic_cast<Predicate*>(symbol);
				Assert(pred != NULL);
				if (v != Vocabulary::std() and pred->builtin()) { // Only print builtins when printing the std voc
					continue;
				}
				if (PredUtils::isTypePredicate(pred)) { // Do not print sort-predicates
					continue;
				}
				printTab();
				visit(pred);
				output() << "\n";
			} else {
				auto func = dynamic_cast<Function*>(symbol);
				Assert(func != NULL);
				printTab();
				visit(func);
				output() << "\n";
			}
		}
		unindent();
		printTab();
		output() << "}" << '\n';

	}

	void visit(const Namespace* s) {
		printTab();
		output() << "namespace " << s->name() << " {" << '\n';
		indent();

		Assert(isTheoryOpen());
		// FIXME allow to loop over all components of a namespace
		for (auto i = s->vocabularies().cbegin(); i != s->vocabularies().cend(); ++i) {
			visit((*i).second);
		}
		for (auto i = s->theories().cbegin(); i != s->theories().cend(); ++i) {
			(*i).second->accept(this);
		}
		for (auto i = s->structures().cbegin(); i != s->structures().cend(); ++i) {
			visit((*i).second);
		}
		for (auto i = s->subspaces().cbegin(); i != s->subspaces().cend(); ++i) {
			visit((*i).second);
		}
		for (auto i = s->procedures().cbegin(); i != s->procedures().cend(); ++i) {
			visit((*i).second);
		}
		for (auto i = s->terms().cbegin(); i != s->terms().cend(); ++i) {
			(*i).second->accept(this);
		}
		for (auto i = s->queries().cbegin(); i != s->queries().cend(); ++i) {
			visit((*i).second);
		}
		unindent();
		printTab();
		output() << "}" << '\n';
	}

	void visit(const UserProcedure* p){
		output() << "procedure "<< p->name()<<"(";
		auto beginargs=true;
		for(auto a:p->args()){
			if(not beginargs){
				output()<< ", ";
			}else{
				beginargs=false;
			}
			output()<<a;
		}
		output() << "){ \n";
		output() << p->getProcedurecode() << "\n";
		output() << "}\n";
	}

	void visit(const GroundFixpDef*) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		/*TODO not implemented yet*/
		throw notyetimplemented("Trying to print out theory component which cannot be printed in IDP format.");
		_printTermsAsBlock = backup;
	}

	void visit(const Theory* t) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		printTab();
		output() << "theory " << t->name() << " : " << t->vocabulary()->name() << " {" << '\n';
		indent();
		printTab();

		Assert(isTheoryOpen());
		for (auto sentence : t->sentences()) {
			sentence->accept(this);
			output() << "." << '\n';
		}
		for (auto definition : t->definitions()) {
			definition->accept(this);
			output() << "" << '\n';
		}
		for (auto fixpdef : t->fixpdefs()) {
			fixpdef->accept(this);
			output() << "" << '\n';
		}
		unindent();
		printTab();
		output() << "}" << '\n';
		_printTermsAsBlock = backup;
	}

	template<typename Visitor, typename List>
	void visitList(Visitor v, const List& list) {
		for (auto i = list.cbegin(); i < list.cend(); ++i) {
			CHECKTERMINATION;
			(*i)->accept(v);
		}

	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		setTranslator(g->translator());
		for (auto i = g->getClauses().cbegin(); i < g->getClauses().cend(); ++i) {
			CHECKTERMINATION;
			visit(*i);
		}
		visitList(this, g->getCPReifications());
		visitList(this, g->getSets());
		visitList(this, g->getAggregates());
		visitList(this, g->getFixpDefinitions());
		for (auto i = g->getDefinitions().cbegin(); i != g->getDefinitions().cend(); i++) {
			CHECKTERMINATION;
			openDefinition((*i).second->id());
			(*i).second->accept(this);
			closeDefinition();
		}
		_printTermsAsBlock = backup;
	}

	/** Formulas **/

	void visit(const PredForm* f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		if (isNeg(f->sign())) {
			output() << "~";
		}
		if(f->symbol()->infix()){
			f->subterms()[0]->accept(this);
			output() << print(f->symbol());
			f->subterms()[1]->accept(this);
		}else{
		output() << print(f->symbol());
		if (not f->subterms().empty()) {
			if(f->isGraphedFunction()){ //f is a function, not a predicate, and should be printed as one
				auto size = f->subterms().size();
				if(size>1){
					output() << "(";
					f->subterms()[0]->accept(this);
					for (size_t n = 1; n < f->subterms().size()-1; ++n) {
					output() << ',';
						f->subterms()[n]->accept(this);
					}
					output() << ")";
				}
				output() << "=";
				f->subterms()[size-1]->accept(this);
			}else{
				output() << "(";
				f->subterms()[0]->accept(this);
				for (size_t n = 1; n < f->subterms().size(); ++n) {
					output() << ',';
					f->subterms()[n]->accept(this);
				}
				output() << ")";
			}
		}
		}
		_printTermsAsBlock = backup;
	}

	void visit(const EqChainForm* f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		if (isNeg(f->sign())) {
			output() << "~";
		}
		output() << "(";
		f->subterms()[0]->accept(this);
		for (size_t n = 0; n < f->comps().size(); ++n) {
			CHECKTERMINATION;
			output() << ' ' << print(f->comps()[n]) << ' ';
			f->subterms()[n + 1]->accept(this);
			if (not f->conj() && (n + 1 < f->comps().size())) {
				output() << " | ";
				f->subterms()[n + 1]->accept(this);
			}
		}
		output() << ')';
		_printTermsAsBlock = backup;
	}

	void visit(const EquivForm* f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		if (isNeg(f->sign())) {
			output() << "~";
		}
		output() << "(";
		f->left()->accept(this);
		output() << " <=> ";
		f->right()->accept(this);
		output() << ')';
		_printTermsAsBlock = backup;
	}

	void visit(const BoolForm* f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		if (f->subformulas().empty()) {
			if (f->isConjWithSign()) {
				output() << "true";
			} else {
				output() << "false";
			}
		} else {
			if (isNeg(f->sign())) {
				output() << "~";
			}
			output() << "(";
			f->subformulas()[0]->accept(this);
			for (size_t n = 1; n < f->subformulas().size(); ++n) {
				CHECKTERMINATION
				output() << (f->conj() ? " & " : " | ");
				f->subformulas()[n]->accept(this);
			}
			output() << ')';
		}
		_printTermsAsBlock = backup;
	}

	void visit(const QuantForm* f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		if (isNeg(f->sign())) {
			output() << "~";
		}
		output() << "(";
		if (f->isUniv()) {
			output() << "!";
		} else {
			output() << "?";
		}
		for (auto it = f->quantVars().cbegin(); it != f->quantVars().cend(); ++it) {
			CHECKTERMINATION;
			output() << " ";
			output() << (*it)->name();
			if ((*it)->sort()) {
				output() << '[' << (*it)->sort()->name() << ']';
			}
		}
		output() << " : ";
		f->subformulas()[0]->accept(this);
		output() << ')';
		_printTermsAsBlock = backup;
	}

	void visit(const AggForm* f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		if (isNeg(f->sign())) {
			output() << '~';
		}
		output() << '(';
		f->getBound()->accept(this);
		output() << ' ' << print(f->comp()) << ' ';
		f->getAggTerm()->accept(this);
		output() << ')';
		_printTermsAsBlock = backup;
	}

	/** Definitions **/

	void visit(const Rule* r) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printTab();
		if (not r->quantVars().empty()) {
			output() << "!";
			for (auto it = r->quantVars().cbegin(); it != r->quantVars().cend(); ++it) {
				output() << " " << *(*it);
			}
			output() << " : ";
		}
		r->head()->accept(this);
		output() << " <- ";
		r->body()->accept(this);
		output() << ".";
		_printTermsAsBlock = backup;
	}

	void visit(const Definition* d) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printTab();
		output() << "{\n";
		indent();
		for (auto it = d->rules().cbegin(); it != d->rules().cend(); ++it) {
			CHECKTERMINATION;
			(*it)->accept(this);
			output() << "\n";
		}
		unindent();
		printTab();
		output() << "}";
		_printTermsAsBlock = backup;
	}

	void visit(const FixpDef* d) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printTab();
		output() << (d->lfp() ? "LFD" : "GFD") << " [\n";
		indent();
		for (auto it = d->rules().cbegin(); it != d->rules().cend(); ++it) {
			CHECKTERMINATION;
			(*it)->accept(this);
			output() << "\n";
		}
		for (auto it = d->defs().cbegin(); it != d->defs().cend(); ++it) {
			CHECKTERMINATION;
			(*it)->accept(this);
		}
		unindent();
		printTab();
		output() << "]";
		_printTermsAsBlock = backup;
	}

	/** Terms **/

	void printTermName(const Term* t) {
		if (_printTermsAsBlock) {
			printTab();
			Assert(t->name() != "");
			Assert(t->vocabulary()!=NULL);
			output() << "term " << t->name() << " : " << t->vocabulary()->name() << " {" << '\n';
			indent();
			printTab();
		}
	}

	void finishTermPrinting() {
		if (_printTermsAsBlock) {
			printTab();
			output() << "\n";
			unindent();
			printTab();
			output() << "}\n";
		}
	}

	void visit(const VarTerm* t) {
		printTermName(t);
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << t->var()->name();
		_printTermsAsBlock = backup;
		finishTermPrinting();
	}

	void visit(const FuncTerm* t) {
		printTermName(t);
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		/*
		 * If a function name is overloaded, the extra details should also be printed (sorts)
		 *
		 * It is the function name that is overloaded, not the function itself.
		 * Therefore we should find the internal function associated to that name and check if that function is overloaded.
		 */
		bool overloaded = false;
		for(auto i:t->function()->getVocabularies()){
			auto functionlist = i->getFuncs();
			if((*(functionlist.find(t->function()->name()))).second->overloaded()){
				overloaded = true;
			}
		}

		// We don't want to print for every overloaded built in function (like +) all the sorts
		// However, we do need this for constants like MAX and MIN
		// TODO: Can be dangerous if the user overloads these built in functions
		if(overloaded && (not t->function()->builtin() or t->function()->arity()==0)){
			output() << t->function()->nameNoArity();
			if (t->function()->sorts().size() > 0) {
				output() << "[";
				auto begin = true;
				for(auto sort:t->function()->sorts()){
					if(sort!=t->function()->sorts().back()){
						if(not begin){
							output() <<",";
						}
						begin = false;
						auto voc = sort->getVocabularies().cbegin();
						output() << (*voc)->name()<< "::" << sort->name();
					}
				}
				output() << ":";
				auto outvoc = t->function()->sorts().back()->getVocabularies().cbegin();
				output() << (*outvoc)->name()<< "::" << print(t->function()->sorts().back());
				}
				output() << "]";
		}else{
			output() << print(t->function());
		}
		if (not t->subterms().empty()) {
			if(t->function()->name()=="-/1" and t->subterms().size()==1 and t->subterms()[0]->type()==TermType::DOM){
				t->subterms()[0]->accept(this);
			}else{
				output() << "(";
				t->subterms()[0]->accept(this);
				for (unsigned int n = 1; n < t->subterms().size(); ++n) {
					output() << ",";
					t->subterms()[n]->accept(this);
				}
				output() << ")";
			}
		}
		_printTermsAsBlock = backup;
		finishTermPrinting();
	}

	void visit(const DomainTerm* t) {
		printTermName(t);
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		std::string str = toString(t->value());
		if (t->sort()) {
			if (SortUtils::isSubsort(t->sort(), get(STDSORT::CHARSORT))) {
				output() << str;
			} else if (SortUtils::isSubsort(t->sort(), get(STDSORT::STRINGSORT))) {
				output()  << str ;
			} else {
				output() << str;
			}
		} else {
			output() << '@' << str;
		}
		_printTermsAsBlock = backup;
		finishTermPrinting();

	}

	void visit(const AggTerm* t) {
		printTermName(t);
		auto backup = _printTermsAsBlock;
		auto backupTermPrint = _printSetTerm;
		_printTermsAsBlock = false;
		_printSetTerm = true;
		Assert(isTheoryOpen());

		if(t->set()->getSets().size()>1){
			switch (t->function()) {
				case AggFunction::CARD:
					output() << "sum";
					break;
				case AggFunction::SUM:
					output() << "sum";
					break;
				case AggFunction::PROD:
					output() << "prod";
					break;
				case AggFunction::MIN:
					output() << "min";
					break;
				case AggFunction::MAX:
					output() << "max";
					break;
			}
			output() <<'(';
			printEnumSetExpr(t->set(),t->function());
			output() <<')';
		}else{
			printEnumSetExpr(t->set(),t->function());
		}
		_printTermsAsBlock = backup;
		_printSetTerm = backupTermPrint;
		finishTermPrinting();
	}

	/** Set expressions **/

	void visit(const EnumSetExpr* s) {
		Assert(s->getSets().size()>0);
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		bool begin = true;
		for (auto i = s->getSets().cbegin(); i < s->getSets().cend(); ++i) {
			if (not begin) {
				output() << " U ";
			}
			begin = false;
			(*i)->accept(this);
		}
		_printTermsAsBlock = backup;
	}

	void visit(const QuantSetExpr* s) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		output() << '{';
		for (auto it = s->quantVars().cbegin(); it != s->quantVars().cend(); ++it) {
			output() << ' ';
			output() << (*it)->name();
			if ((*it)->sort()) {
				output() << '[' << (*it)->sort()->name() << ']';
			}
		}
		output() << " : ";
		s->getCondition()->accept(this);
		if(_printSetTerm){
			output() << " : ";
			s->getTerm()->accept(this);
		}
		output() << " }";
		_printTermsAsBlock = backup;
	}

	void visit(const GroundClause& g) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		if (g.empty()) {
			output() << "false";
		} else {
			for (unsigned int m = 0; m < g.size(); ++m) {
				if (g[m] < 0) {
					output() << '~';
				}
				printAtom(g[m]);
				if (m < g.size() - 1) {
					output() << " | ";
				}
			}
		}
		output() << "." << "\n";
		_printTermsAsBlock = backup;
	}

	void openDefinition(DefId defid) {
		Assert(isDefClosed());
		openDef(defid);
		printTab();
		output() << "{\n";
		indent();
	}

	void closeDefinition() {
		Assert(not isDefClosed());
		closeDef();
		unindent();
		output() << "}\n";
	}

	void visit(const GroundDefinition* d) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		for (auto rule : d->rules()) {
			rule.second->accept(this);
		}
		_printTermsAsBlock = backup;
	}

	void visit(const PCGroundRule* b) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printAtom(b->head());
		output() << " <- ";
		char c = (b->type() == RuleType::CONJ ? '&' : '|');
		if (not b->empty()) {
			for (unsigned int n = 0; n < b->size(); ++n) {
				if (b->literal(n) < 0) {
					output() << '~';
				}
				printAtom(b->literal(n));
				if (n != b->size() - 1) {
					output() << ' ' << c << ' ';
				}
			}
		} else {
			Assert(b->empty());
			if (b->type() == RuleType::CONJ) {
				output() << "true";
			} else {
				output() << "false";
			}
		}
		output() << ".\n";
		_printTermsAsBlock = backup;
	}

	void visit(const AggGroundRule* b) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printAtom(b->head());
		output() << " <- ";
		printAggregate(b->bound(), b->lower(), b->aggtype(), b->setnr());
		_printTermsAsBlock = backup;
	}

	void visit(const GroundAggregate* a) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printAtom(a->head());
		switch (a->arrow()) {
			case TsType::IMPL:
			output() << " => ";
			break;
			case TsType::RIMPL:
			output() << " <= ";
			break;
			case TsType::EQ:
			output() << " <=> ";
			break;
			case TsType::RULE:
			break;
		}
		printAggregate(a->bound(), a->lower(), a->type(), a->setnr());
		_printTermsAsBlock = backup;
	}

	void visit(const CPReification* cpr) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printAtom(cpr->_head);
		switch (cpr->_body->type()) {
			case TsType::RULE:
			output() << " <- ";
			break;
			case TsType::IMPL:
			output() << " => ";
			break;
			case TsType::RIMPL:
			output() << " <= ";
			break;
			case TsType::EQ:
			output() << " <=> ";
			break;
		}
		cpr->_body->left()->accept(this);
		switch (cpr->_body->comp()) {
			case CompType::EQ:
			output() << " = ";
			break;
			case CompType::NEQ:
			output() << " ~= ";
			break;
			case CompType::LEQ:
			output() << " =< ";
			break;
			case CompType::GEQ:
			output() << " >= ";
			break;
			case CompType::LT:
			output() << " < ";
			break;
			case CompType::GT:
			output() << " > ";
			break;
		}
		CPBound right = cpr->_body->right();
		if (right._isvarid) {
			printTerm(right._varid);
		} else {
			output() << right._bound;
		}
		output() << ".\n";
		_printTermsAsBlock = backup;
	}

	void visit(const CPSetTerm* cpt) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << "w" <<print(cpt->type());
		bool printweightinset = cpt->weights().size() > 1;
		if(cpt->weights().size() == 1){
			output() <<cpt->weights().back() <<"*";
		}
		output() <<"[ ";
		for (uint i=0; i < cpt->varids().size(); ++i) {
			printAtom(cpt->conditions()[i]);
			output() << " ? ";
			printTerm(cpt->varids()[i]);
			if(printweightinset){
				output() << "*" << cpt->weights()[i];
			}
			output() << "; ";
		}
		output() << " ]";
		_printTermsAsBlock = backup;
	}

	void visit(const CPVarTerm* cpt) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printTerm(cpt->varid());
		_printTermsAsBlock = backup;
	}

	void visit(const PredTable* table) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		if (not table->finite()) {
			std::clog << "Requested to print infinite table, did not do this.\n";
			return;
		}
		TableIterator kt = table->begin();
		if (table->arity() > 0) {
			output() << "{ ";
			if (not kt.isAtEnd()) {
				bool beginlist = true;
				for (; not kt.isAtEnd(); ++kt) {
					CHECKTERMINATION;
					if (not beginlist) {
						output() << "; ";
					}
					beginlist = false;
					ElementTuple tuple = *kt;
					bool begintuple = true;
					for (auto lt = tuple.cbegin(); lt != tuple.cend(); ++lt) {
						if (not begintuple) {
							output() << ',';
						}
						begintuple = false;
						output() << print(*lt);
					}
				}
			}
			output() << " }";
		} else if (not kt.isAtEnd()) {
			output() << "true";
		} else {
			output() << "false";
		}
		_printTermsAsBlock = backup;
	}

	void visit(FuncTable* table) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		std::vector<SortTable*> vst = table->universe().tables();
		vst.pop_back();
		Universe univ(vst);
		if (univ.approxFinite()) {
			TableIterator kt = table->begin();
			if (table->arity() != 0) {
				output() << "{ ";
				if (not kt.isAtEnd()) {
					ElementTuple tuple = *kt;
					output() << print(tuple[0]);
					for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
						output() << ',' << print(tuple[n]);
					}
					output() << "->" << print(tuple.back());
					++kt;
					for (; not kt.isAtEnd(); ++kt) {
						CHECKTERMINATION;
						output() << "; ";
						tuple = *kt;
						output() << print(tuple[0]);
						for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
							output() << ',' << print(tuple[n]);
						}
						output() << "->" << print(tuple.back());
					}
				}
				output() << " }";
			} else if (not kt.isAtEnd()) {
				output() << print((*kt)[0]);
			} else {
				output() << "{ }";
			}
		} else {
			output() << "possibly infinite table";
		}
		_printTermsAsBlock = backup;
	}

	void visit(const Sort* s) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << "type " << s->name();
		if (not s->parents().empty()) {
			output() << " isa " << (*(s->parents().cbegin()))->name();
			auto it = s->parents().cbegin();
			++it;
			for (; it != s->parents().cend(); ++it) {
				output() << "," << (*it)->name();
			}
		}
		if(s->isConstructed()){
			output() << " constructed from {";
			bool first = true;
			for(auto cons: s->getConstructors()){
				if(not first)
					output() << ", ";
				first=false;
				printFuncWithoutOutsort(cons);
			}
			output() << "}";
		}
		_printTermsAsBlock = backup;
	}

	void printNonOverloadedPredicate(const Predicate* p) {
		output() << p->nameNoArity();
		if (p->arity() > 0) {
			output() << "(" << p->sort(0)->name();
			for (unsigned int n = 1; n < p->arity(); ++n) {
				output() << "," << p->sort(n)->name();
			}
			output() << ")";
		}
	}

	void visit(const Predicate* p) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		if (p->overloaded()) {
			Predicate* p2 = const_cast<Predicate*>(p);
			for(auto e: p2->nonbuiltins()) {
				printNonOverloadedPredicate(e);
			}
		} else {
			printNonOverloadedPredicate(p);
		}
		_printTermsAsBlock = backup;
	}

	void visit(const Function* f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		if (f->overloaded()) {
			Function* f2 = const_cast<Function*>(f);
			for(auto e: f2->nonbuiltins()) {
				if (e->partial()) {
					output() << "partial ";
				}
				output() << e->name().substr(0, e->name().find('/'));
				if (e->arity() > 0) {
					output() << "(" << e->insort(0)->name();
					for (unsigned int n = 1; n < e->arity(); ++n) {
						output() << "," << e->insort(n)->name();
					}
					output() << ")";
				}
				output() << " : " << e->outsort()->name() << "\n";
			}
		} else {
			printTab();
			if (f->partial()) {
				output() << "partial ";
			}
			output() << f->name().substr(0, f->name().find('/'));
			if (f->arity() > 0) {
				output() << "(" << f->insort(0)->name();
				for (unsigned int n = 1; n < f->arity(); ++n) {
					output() << "," << f->insort(n)->name();
				}
				output() << ")";
			}
			output() << " : " << f->outsort()->name();
		}
		_printTermsAsBlock = backup;
	}

	void visit(const SortTable* table) {
		if(not table->finite()){
			// TODO this is an issue for user tables, but should not even be printed for constructed tables
			std::clog << "Requested to print infinite table, did not do this.\n";
			return;
		}
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << "{ ";
		if (table->isRange()) {
			output() << print(table->first()) << ".." << print(table->last());
		} else {
			auto it = table->sortBegin();
			if (not it.isAtEnd()) {
				output() << print((*it));
				++it;
				for (; not it.isAtEnd(); ++it) {
					CHECKTERMINATION;
					output() << "; " << print((*it));
				}
			}
		}
		output() << " }";
		_printTermsAsBlock = backup;
	}

	void visit(const GroundSet* s) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << "set_" << s->setnr() << " = [ ";
		for (size_t n = 0; n < s->size(); ++n) {
			CHECKTERMINATION;
			if (s->weighted()) {
				output() << '(';
			}
			printAtom(s->literal(n));
			if (s->weighted()) {
				output() << ',' << s->weight(n) << ')';
			}
			if (n < s->size() - 1) {
				output() << "; ";
			}
		}
		output() << " ]\n";
		_printTermsAsBlock = backup;
	}

private:
	void printAtom(int atomnr) {
		Assert(_translator != NULL);

		// The sign of the literal is handled on higher level.
		atomnr = abs(atomnr);
		// Get the atom's symbol from the translator.

		if (not _translator->isInputAtom(atomnr)) {
			output() << "tseitin_" << atomnr;
			return;
		}

		auto pfs = _translator->getSymbol(atomnr);

		// Print the symbol's name.
		output() << pfs->name().substr(0, pfs->name().find('/'));
		// Print the symbol's sorts.
		if (pfs->nrSorts()) {
			output() << '[';
			for (size_t n = 0; n < pfs->nrSorts(); ++n) {
				if (pfs->sort(n)) {
					output() << pfs->sort(n)->name();
					if (n != pfs->nrSorts() - 1) {
						output() << ',';
					}
				}
			}
			output() << ']';
		}
		// Get the atom's arguments for the translator.
		const auto& args = _translator->getArgs(atomnr);
		// Print the atom's arguments.
		if (isa<Predicate>(*pfs)) {
			if (not args.empty()) {
				output() << "(";
				for (size_t n = 0; n < args.size(); ++n) {
					output() << print(args[n]);
					if (n != args.size() - 1) {
						output() << ",";
					}
				}
				output() << ")";
			}
		} else {
			Assert(isa<Function>(*pfs));
			if (args.size() > 1) {
				output() << "(";
				for (size_t n = 0; n < args.size() - 1; ++n) {
					output() << print(args[n]);
					if (n != args.size() - 2) {
						output() << ",";
					}
				}
				output() << ")";
			}
			output() << " = " << print(args.back());
		}
	}

	void printTerm(VarId termnr) {
		// Make sure there is a translator.
		Assert(_translator != NULL);
		// Get information from the term translator.

		if(not _translator->hasVarIdMapping(termnr)) {
			output() << "var_" << termnr;
			//		CPTsBody* cprelation = _translator->cprelation(varid);
			//		CPReification(1,cprelation).accept(this);
			return;
		}

		auto func = _translator->getFunction(termnr);
		// Print the symbol's name.
		output() << func->name().substr(0, func->name().find('/'));
		// Print the symbol's sorts.
		if (func->nrSorts()) {
			output() << '[';
			for (size_t n = 0; n < func->nrSorts(); ++n) {
				if (func->sort(n)) {
					output() << func->sort(n)->name();
					if (n != func->nrSorts() - 1) {
						output() << ',';
					}
				}
			}
			output() << ']';
		}
		// Get the arguments from the translator.
		const auto& args = _translator->getArgs(termnr);
		// Print the arguments.
		if (not args.empty()) {
			output() << "(";
			bool begin = true;
			for (auto gtit = args.cbegin(); gtit != args.cend(); ++gtit) {
				if (not begin) {
					output() << ",";
				}
				begin = false;
				if ((*gtit).isVariable) {
					printTerm((*gtit)._varid);
				} else {
					output() << print((*gtit)._domelement);
				}
			}
			output() << ")";
		}
	}

	void printAggregate(double bound, bool lower, AggFunction aggtype, SetId setnr) {
		output() << bound << (lower ? " =< " : " >= ");
		switch (aggtype) {
			case AggFunction::CARD:
			output() << "card(";
			break;
			case AggFunction::SUM:
			output() << "sum(";
			break;
			case AggFunction::PROD:
			output() << "prod(";
			break;
			case AggFunction::MIN:
			output() << "min(";
			break;
			case AggFunction::MAX:
			output() << "max(";
			break;
		}
		output() << "set_" << setnr << ")." << "\n";
	}

	void printFuncWithoutOutsort(const Function* f){
		Assert(isTheoryOpen());
		if (f->overloaded()) {
			Function* f2 = const_cast<Function*>(f);
			for(auto e: f2->nonbuiltins()) {
				if (e->partial()) {
					output() << "partial ";
				}
				output() << e->name().substr(0, e->name().find('/'));
				if (e->arity() > 0) {
					output() << "(" << e->insort(0)->name();
					for (unsigned int n = 1; n < e->arity(); ++n) {
						output() << "," << e->insort(n)->name();
					}
					output() << ")";
				}
				output() << " : " << e->outsort()->name();
			}
		} else {
			printTab();
			if (f->partial()) {
				output() << "partial ";
			}
			output() << f->name().substr(0, f->name().find('/'));
			if (f->arity() > 0) {
				output() << "(" << f->insort(0)->name();
				for (unsigned int n = 1; n < f->arity(); ++n) {
					output() << "," << f->insort(n)->name();
				}
				output() << ")";
			}
		}
	}
	void printAsFunc(const PredTable* table) {
		if (not table->finite()) {
			std::clog << "Requested to print infinite predtable, did not print it.\n";
			return;
		}
		TableIterator kt = table->begin();
		output() << "{ ";
		if (not kt.isAtEnd()) {
			ElementTuple tuple = *kt;
			if (tuple.size() > 1) {
				output() << print(tuple[0]);
			}
			for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
				output() << ',' << print(tuple[n]);
			}
			output() << "->" << print(tuple.back());
			++kt;
			for (; not kt.isAtEnd(); ++kt) {
				CHECKTERMINATION;
				output() << "; ";
				tuple = *kt;
				if (tuple.size() > 1) {
					output() << print(tuple[0]);
				}
				for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
					output() << ',' << print(tuple[n]);
				}
				output() << "->" << print(tuple.back());
			}
		}
		output() << " }";
	}

	virtual void visit(const AbstractGroundTheory*){
		throw notyetimplemented("Trying to print out theory component which cannot be printed in IDP format.");
	}

	void printSortRecursively(Sort* sort, std::vector<Sort*>& printedsorts,const Vocabulary* v){
		for(auto it:sort->parents()){
			printSortRecursively(it,printedsorts,v);
		}
		if(not contains(printedsorts, sort)){
			if (not sort->builtin() || v == Vocabulary::std()) {
				printTab();
				visit(sort);
				printedsorts.push_back(sort);
				output() << "\n";
			}
		}
	}

	void printEnumSetExpr(EnumSetExpr* s, AggFunction f) {
		Assert(s->getSets().size() > 0);
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		bool begin = true;
		for (auto i = s->getSets().cbegin(); i < s->getSets().cend(); ++i) {
			if (not begin) {
				output() << ", ";
			}
			begin = false;
			printQuantSetExpr((*i), f);
		}
		_printTermsAsBlock = backup;
	}

	void printQuantSetExpr(QuantSetExpr* s, AggFunction f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		switch (f) {
		case AggFunction::CARD:
			output() << "#";
			_printSetTerm = false;
			break;
		case AggFunction::SUM:
			output() << "sum";
			break;
		case AggFunction::PROD:
			output() << "prod";
			break;
		case AggFunction::MIN:
			output() << "min";
			break;
		case AggFunction::MAX:
			output() << "max";
			break;
		}
		output() << '{';
		for (auto qv : s->quantVars()) {
			output() << ' ';
			output() << qv->name();
			if (qv->sort()) {
				output() << '[' << qv->sort()->name() << ']';
			}
		}
		output() << " : ";
		s->getCondition()->accept(this);
		if (_printSetTerm) {
			output() << " : ";
			s->getTerm()->accept(this);
		}
		output() << " }";
		_printTermsAsBlock = backup;
	}
};
