/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef IDPPRINTER_HPP_
#define IDPPRINTER_HPP_

#include "printers/print.hpp"
#include "IncludeComponents.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "theory/Query.hpp"
#include "groundtheories/GroundPolicy.hpp"

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
	IDPPrinter(Stream& stream, bool printTermsAsBlock = true)
			: 	StreamPrinter<Stream>(stream),
				_translator(NULL),
				_printTermsAsBlock(printTermsAsBlock) {
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
		ss << o->name() << "[";
		bool begin = true;
		for (auto i = o->sorts().cbegin(); i < o->sorts().cend(); ++i) {
			if (not begin) {
				ss << ",";
			}
			begin = false;
			ss << toString(*i);
		}
		ss << "]";
		return ss.str();
	}

	void visit(const AbstractStructure* structure) {
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

		auto origlongnameoption = getOption(BoolType::LONGNAMES);
		setOption(BoolType::LONGNAMES, true);
		for (auto it = voc->firstSort(); it != voc->lastSort(); ++it) {
			auto s = it->second;
			if (not s->builtin()) {
				printTab();
				output() << toString(s) << "[" << toString(s) << "]" << " = ";
				auto st = structure->inter(s);
				visit(st);
				output() << '\n';
			}
		}
		for (auto it = voc->firstPred(); it != voc->lastPred(); ++it) {
			auto sp = it->second->nonbuiltins();
			for (auto jt = sp.cbegin(); jt != sp.cend(); ++jt) {
				auto p = *jt;
				if (p->arity() != 1 || p->sorts()[0]->pred() != p) {
					auto pi = structure->inter(p);
					if (pi->approxTwoValued()) {
						printTab();
						output() << printFullyQualified(p) << " = ";
						visit(pi->ct());
						output() << '\n';
					} else {
						printTab();
						output() << printFullyQualified(p) << "<ct> = ";
						visit(pi->ct());
						output() << '\n';
						printTab();
						output() << printFullyQualified(p) << "<cf> = ";
						visit(pi->cf());
						output() << '\n';
					}
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
					output() << printFullyQualified(f) << " = ";
					visit(ft);
					output() << '\n';
				} else {
					auto pi = fi->graphInter();
					auto ct = pi->ct();
					printTab();
					output() << printFullyQualified(f) << "<ct> = ";
					printAsFunc(ct);
					output() << '\n';
					auto cf = pi->cf();
					printTab();
					output() << printFullyQualified(f) << "<cf> = ";
					printAsFunc(cf);
					output() << '\n';
				}
			}
		}
		setOption(BoolType::LONGNAMES, origlongnameoption);
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

	void visit(const Vocabulary* v) {
		printTab();
		output() << "vocabulary " << v->name() << " {" << '\n';
		indent();

		Assert(isTheoryOpen());
		for (auto it = v->firstSort(); it != v->lastSort(); ++it) {
			if (not it->second->builtin() || v == Vocabulary::std()) {
				visit(it->second);
			}
		}
		for (auto it = v->firstPred(); it != v->lastPred(); ++it) {
			if (not it->second->builtin() || v == Vocabulary::std()) {
				visit(it->second);
			}
		}
		for (auto it = v->firstFunc(); it != v->lastFunc(); ++it) {
			if (not it->second->builtin() || v == Vocabulary::std()) { // FIXME apparently, </2 etc still get printed?
				visit(it->second);
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
			// FIXME visit((*i).second); //see #200
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

	void visit(const GroundFixpDef*) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		/*TODO not implemented yet*/
		throw notyetimplemented("(printing fixpoint definitions is not yet implemented)");
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
		for (auto it = t->sentences().cbegin(); it != t->sentences().cend(); ++it) {
			(*it)->accept(this);
			output() << "" << '\n';
		}
		for (auto it = t->definitions().cbegin(); it != t->definitions().cend(); ++it) {
			(*it)->accept(this);
			output() << "" << '\n';
		}
		for (auto it = t->fixpdefs().cbegin(); it != t->fixpdefs().cend(); ++it) {
			(*it)->accept(this);
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
			CHECKTERMINATION
			(*i)->accept(v);
		}

	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		setTranslator(g->translator());
		for (auto i = g->getClauses().cbegin(); i < g->getClauses().cend(); ++i) {
			CHECKTERMINATION
			visit(*i);
		}
		visitList(this, g->getCPReifications());
		visitList(this, g->getSets());
		visitList(this, g->getAggregates());
		visitList(this, g->getFixpDefinitions());
		for (auto i = g->getDefinitions().cbegin(); i != g->getDefinitions().cend(); i++) {
			CHECKTERMINATION
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
		if (isNeg(f->sign()))
			output() << "~";
		output() << toString(f->symbol());
		if (not f->subterms().empty()) {
			output() << "(";
			f->subterms()[0]->accept(this);
			for (size_t n = 1; n < f->subterms().size(); ++n) {
				output() << ',';
				f->subterms()[n]->accept(this);
			}
			output() << ')';
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
			CHECKTERMINATION
			output() << ' ' << toString(f->comps()[n]) << ' ';
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
			CHECKTERMINATION
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
		output() << ' ' << toString(f->comp()) << ' ';
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
			CHECKTERMINATION
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
			CHECKTERMINATION
			(*it)->accept(this);
			output() << "\n";
		}
		for (auto it = d->defs().cbegin(); it != d->defs().cend(); ++it) {
			CHECKTERMINATION
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
		output() << toString(t->function());
		if (not t->subterms().empty()) {
			output() << "(";
			t->subterms()[0]->accept(this);
			for (unsigned int n = 1; n < t->subterms().size(); ++n) {
				output() << ",";
				t->subterms()[n]->accept(this);
			}
			output() << ")";
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
				output() << '\'' << str << '\'';
			} else if (SortUtils::isSubsort(t->sort(), get(STDSORT::STRINGSORT))) {
				output() << '\"' << str << '\"';
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
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		switch (t->function()) {
		case AggFunction::CARD:
			output() << '#';
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
		t->set()->accept(this);
		_printTermsAsBlock = backup;
		finishTermPrinting();
	}

	/** Set expressions **/

	void visit(const EnumSetExpr* s) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		output() << "[ ";
		bool begin = true;
		for (auto i = s->getSets().cbegin(); i < s->getSets().cend(); ++i) {
			if (not begin) {
				output() << ", ";
			}
			begin = false;
			(*i)->accept(this);
		}
		output() << " ]";
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
		output() << " : ";
		s->getTerm()->accept(this);
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
		for (auto it = d->begin(); it != d->end(); ++it) {
			(*it).second->accept(this);
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

	void visit(const CPWSumTerm* cpt) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << "wsum[ ";
		auto vit = cpt->varids().cbegin();
		auto wit = cpt->weights().cbegin();
		for (; vit != cpt->varids().cend() && wit != cpt->weights().cend(); ++vit, ++wit) {
			output() << '(';
			printTerm(*vit);
			output() << ',' << *wit << ')';
			if (*vit != cpt->varids().back()) {
				output() << "; ";
			}
		}
		output() << " ]";
		_printTermsAsBlock = backup;
	}

	void visit(const CPWProdTerm* cpt) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << "wprod[ ";
		for (auto vit = cpt->varids().cbegin(); vit != cpt->varids().cend(); ++vit) {
			printTerm(*vit);
			output() << "; ";
		}
		output() << cpt->weight() << " ]";
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
		}
		TableIterator kt = table->begin();
		if (table->arity() > 0) {
			output() << "{ ";
			if (not kt.isAtEnd()) {
				bool beginlist = true;
				for (; not kt.isAtEnd(); ++kt) {
					CHECKTERMINATION
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
						output() << toString(*lt);
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
					output() << toString(tuple[0]);
					for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
						output() << ',' << toString(tuple[n]);
					}
					output() << "->" << toString(tuple.back());
					++kt;
					for (; not kt.isAtEnd(); ++kt) {
						CHECKTERMINATION
						output() << "; ";
						tuple = *kt;
						output() << toString(tuple[0]);
						for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
							output() << ',' << toString(tuple[n]);
						}
						output() << "->" << toString(tuple.back());
					}
				}
				output() << " }";
			} else if (not kt.isAtEnd())
				output() << toString((*kt)[0]);
			else
				output() << "{ }";
		} else {
			output() << "possibly infinite table";
		}
		_printTermsAsBlock = backup;
	}

	void visit(const Sort* s) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printTab();
		output() << "type " << s->name();
		if (not s->parents().empty()) {
			output() << " isa " << (*(s->parents().cbegin()))->name();
			auto it = s->parents().cbegin();
			++it;
			for (; it != s->parents().cend(); ++it) {
				output() << "," << (*it)->name();
			}
		}
		output() << "\n";
		_printTermsAsBlock = backup;
	}

	void visit(const Predicate* p) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printTab();
		if (p->overloaded()) { // FIXME what should happen in this case to get correct idpfiles?
			output() << "overloaded predicate " << p->name() << '\n';
		} else {
			output() << p->name().substr(0, p->name().find('/'));
			if (p->arity() > 0) {
				output() << "(" << p->sort(0)->name();
				for (unsigned int n = 1; n < p->arity(); ++n) {
					output() << "," << p->sort(n)->name();
				}
				output() << ")";
			}
			output() << "\n";
		}
		_printTermsAsBlock = backup;
	}

	void visit(const Function* f) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		printTab();
		if (f->overloaded()) { // FIXME what should happen in this case to get correct idpfiles?
			output() << "overloaded function " << f->name() << '\n';
		} else {
			if (f->partial())
				output() << "partial ";
			output() << f->name().substr(0, f->name().find('/'));
			if (f->arity() > 0) {
				output() << "(" << f->insort(0)->name();
				for (unsigned int n = 1; n < f->arity(); ++n) {
					output() << "," << f->insort(n)->name();
				}
				output() << ")";
			}
			output() << " : " << f->outsort()->name() << "\n";
		}
		_printTermsAsBlock = backup;
	}

	void visit(const SortTable* table) {
		auto backup = _printTermsAsBlock;
		_printTermsAsBlock = false;
		Assert(isTheoryOpen());
		output() << "{ ";
		if (table->isRange()) {
			output() << toString(table->first()) << ".." << toString(table->last());
		} else {
			auto it = table->sortBegin();
			if (not it.isAtEnd()) {
				output() << toString((*it));
				++it;
				for (; not it.isAtEnd(); ++it) {
					CHECKTERMINATION
					output() << "; " << toString((*it));
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
			CHECKTERMINATION
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
		CHECKTERMINATION
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
					output() << toString(args[n]);
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
					output() << toString(args[n]);
					if (n != args.size() - 2) {
						output() << ",";
					}
				}
				output() << ")";
			}
			output() << " = " << toString(args.back());
		}
	}

	void printTerm(VarId termnr) {
		CHECKTERMINATION
		// Make sure there is a translator.
		Assert(_translator != NULL);
		// Get information from the term translator.
		const Function* func = _translator->getFunction(termnr);
		if (func) {
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
			const std::vector<GroundTerm>& args = _translator->args(termnr);
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
						output() << toString((*gtit)._domelement);
					}
				}
				output() << ")";
			}
		} else {
			output() << "var_" << termnr;
			//		CPTsBody* cprelation = _translator->cprelation(varid);
			//		CPReification(1,cprelation).accept(this);
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
				output() << toString(tuple[0]);
			}
			for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
				output() << ',' << toString(tuple[n]);
			}
			output() << "->" << toString(tuple.back());
			++kt;
			for (; not kt.isAtEnd(); ++kt) {
				CHECKTERMINATION
				output() << "; ";
				tuple = *kt;
				if (tuple.size() > 1)
					output() << toString(tuple[0]);
				for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
					output() << ',' << toString(tuple[n]);
				}
				output() << "->" << toString(tuple.back());
			}
		}
		output() << " }";
	}
};

#endif /* IDPPRINTER_HPP_ */
