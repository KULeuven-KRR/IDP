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
#include "groundtheories/GroundPolicy.hpp"

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"

//TODO is not guaranteed to generate correct idp files!
	// FIXME do we want this? Because printing cp constraints etc. should be done correctly then!
//TODO usage of stored parameters might be incorrect in some cases.

template<typename Stream>
class IDPPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	const GroundTranslator* _translator;
	const GroundTermTranslator* _termtranslator;

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
			: StreamPrinter<Stream>(stream), _translator(NULL), _termtranslator(NULL) {
	}

	virtual void setTranslator(GroundTranslator* t) {
		_translator = t;
	}
	virtual void setTermTranslator(GroundTermTranslator* t) {
		_termtranslator = t;
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
		ss << o->name() <<"[";
		bool begin = true;
		for(auto i=o->sorts().cbegin(); i<o->sorts().cend(); ++i){
			if(not begin){
				ss <<",";
			}
			begin = false;
			ss <<toString(*i);
		}
		ss << "]";
		return ss.str();
	}

	void visit(const AbstractStructure* structure) {
		Assert(isTheoryOpen());

		if (not structure->isConsistent()) {
			output() << "INCONSISTENT STRUCTURE";
			return;
		}

		auto voc = structure->vocabulary();

		printTab();
		output() << "structure " << structure->name() << " : " << voc->name() << " {" <<'\n';
		indent();

		auto origlongnameoption = getOption(BoolType::LONGNAMES);
		setOption(BoolType::LONGNAMES, true);
		for (auto it = voc->firstSort(); it != voc->lastSort(); ++it) {
			auto s = it->second;
			if (not s->builtin()) {
				printTab();
				output() << toString(s) <<"[" <<toString(s) <<"]" << " = ";
				auto st = structure->inter(s);
				visit(st);
				output() <<'\n';
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
						output() << printFullyQualified(p) <<" = ";
						visit(pi->ct());
						output() <<'\n';
					} else {
						printTab();
						output() << printFullyQualified(p) << "<ct> = ";
						visit(pi->ct());
						output() <<'\n';
						printTab();
						output() << printFullyQualified(p) << "<cf> = ";
						visit(pi->cf());
						output() <<'\n';
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
					output() <<'\n';
				} else {
					auto pi = fi->graphInter();
					auto ct = pi->ct();
					printTab();
					output() << printFullyQualified(f) << "<ct> = ";
					printAsFunc(ct);
					output() <<'\n';
					auto cf = pi->cf();
					printTab();
					output() << printFullyQualified(f) << "<cf> = ";
					printAsFunc(cf);
					output() <<'\n';
				}
			}
		}
		setOption(BoolType::LONGNAMES, origlongnameoption);
		unindent();
		printTab();
		output() << "}" <<'\n';
	}

	void visit(const Vocabulary* v) {
		printTab();
		output() << "vocabulary " << v->name() << " {" <<'\n';
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
			if (not it->second->builtin() || v == Vocabulary::std()) {  // FIXME apparently, </2 etc still get printed?
				visit(it->second);
			}
		}

		unindent();
		printTab();
		output() << "}" <<'\n';
	}

	void visit(const Namespace* s) {
		printTab();
		output() << "namespace " << s->name() << " {" <<'\n';
		indent();

		Assert(isTheoryOpen());
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

		unindent();
		printTab();
		output() << "}" <<'\n';
	}

	void visit(const GroundFixpDef*) {
		Assert(isTheoryOpen());
		/*TODO not implemented yet*/
		output() << "(printing fixpoint definitions is not yet implemented)\n";
	}

	void visit(const Theory* t) {
		printTab();
		output() << "theory " << t->name() << " : " << t->vocabulary()->name() << " {" <<'\n';
		indent();

		Assert(isTheoryOpen());
		for (auto it = t->sentences().cbegin(); it != t->sentences().cend(); ++it) {
			(*it)->accept(this);
			output() << "" <<'\n';
		}
		for (auto it = t->definitions().cbegin(); it != t->definitions().cend(); ++it) {
			(*it)->accept(this);
			output() << "" <<'\n';
		}
		for (auto it = t->fixpdefs().cbegin(); it != t->fixpdefs().cend(); ++it) {
			(*it)->accept(this);
			output() << "" <<'\n';
		}

		unindent();
		printTab();
		output() << "}" <<'\n';
	}

	template<typename Visitor, typename List>
	void visitList(Visitor v, const List& list) {
		for (auto i = list.cbegin(); i < list.cend(); ++i) {
			(*i)->accept(v);
		}
	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		Assert(isTheoryOpen());
		_translator = g->translator();
		_termtranslator = g->termtranslator();
		for (auto i = g->getClauses().cbegin(); i < g->getClauses().cend(); ++i) {
			visit(*i);
		}
		visitList(this, g->getCPReifications());
		visitList(this, g->getSets());
		visitList(this, g->getAggregates());
		visitList(this, g->getFixpDefinitions());
		for (auto i = g->getDefinitions().cbegin(); i != g->getDefinitions().cend(); i++) {
			openDefinition((*i).second->id());
			(*i).second->accept(this);
			closeDefinition();
		}
	}

	/** Formulas **/

	void visit(const PredForm* f) {
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
	}

	void visit(const EqChainForm* f) {
		Assert(isTheoryOpen());
		if (isNeg(f->sign())) {
			output() << "~";
		}
		output() << "(";
		f->subterms()[0]->accept(this);
		for (size_t n = 0; n < f->comps().size(); ++n) {
			output() << ' ' << toString(f->comps()[n]) << ' ';
			f->subterms()[n + 1]->accept(this);
			if (not f->conj() && (n + 1 < f->comps().size())) {
				output() << " | ";
				f->subterms()[n + 1]->accept(this);
			}
		}
		output() << ')';
	}

	void visit(const EquivForm* f) {
		Assert(isTheoryOpen());
		if (isNeg(f->sign())) {
			output() << "~";
		}
		output() << "(";
		f->left()->accept(this);
		output() << " <=> ";
		f->right()->accept(this);
		output() << ')';
	}

	void visit(const BoolForm* f) {
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
				output() << (f->conj() ? " & " : " | ");
				f->subformulas()[n]->accept(this);
			}
			output() << ')';
		}
	}

	void visit(const QuantForm* f) {
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
			output() << " ";
			output() << (*it)->name();
			if ((*it)->sort()) {
				output() << '[' << (*it)->sort()->name() << ']';
			}
		}
		output() << " : ";
		f->subformulas()[0]->accept(this);
		output() << ')';
	}

	void visit(const AggForm* f) {
		if (isNeg(f->sign())) {
			output() << '~';
		}
		output() << '(';
		f->left()->accept(this);
		output() << ' ' << toString(f->comp()) << ' ';
		f->right()->accept(this);
		output() << ')';
	}

	/** Definitions **/

	void visit(const Rule* r) {
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
	}

	void visit(const Definition* d) {
		Assert(isTheoryOpen());
		printTab();
		output() << "{\n";
		indent();
		for (auto it = d->rules().cbegin(); it != d->rules().cend(); ++it) {
			(*it)->accept(this);
			output() << "\n";
		}
		unindent();
		printTab();
		output() << "}";
	}

	void visit(const FixpDef* d) {
		Assert(isTheoryOpen());
		printTab();
		output() << (d->lfp() ? "LFD" : "GFD") << " [\n";
		indent();
		for (auto it = d->rules().cbegin(); it != d->rules().cend(); ++it) {
			(*it)->accept(this);
			output() << "\n";
		}
		for (auto it = d->defs().cbegin(); it != d->defs().cend(); ++it) {
			(*it)->accept(this);
		}
		unindent();
		printTab();
		output() << "]";
	}

	/** Terms **/

	void visit(const VarTerm* t) {
		Assert(isTheoryOpen());
		output() << t->var()->name();
	}

	void visit(const FuncTerm* t) {
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
	}

	void visit(const DomainTerm* t) {
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
		} else
			output() << '@' << str;
	}

	void visit(const AggTerm* t) {
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
	}

	/** Set expressions **/

	void visit(const EnumSetExpr* s) {
		output() << "[ ";
		if (not s->subformulas().empty()) {
			output() << "( ";
			s->subformulas()[0]->accept(this);
			output() << ", ";
			s->subterms()[0]->accept(this);
			for (unsigned int n = 1; n < s->subformulas().size(); ++n) {
				output() << ") ; (";
				s->subformulas()[n]->accept(this);
				output() << ", ";
				s->subterms()[n]->accept(this);
			}
			output() << ")";
		}
		output() << " ]";
	}

	void visit(const QuantSetExpr* s) {
		output() << '{';
		for (auto it = s->quantVars().cbegin(); it != s->quantVars().cend(); ++it) {
			output() << ' ';
			output() << (*it)->name();
			if ((*it)->sort()) {
				output() << '[' << (*it)->sort()->name() << ']';
			}
		}
		output() << " : ";
		s->subformulas()[0]->accept(this);
		if (not s->subterms().empty()) {
			output() << " : ";
			s->subterms()[0]->accept(this);
		}
		output() << " }";
	}

	void visit(const GroundClause& g) {
		if (g.empty()) {
			output() << "false";
		} else {
			for (unsigned int m = 0; m < g.size(); ++m) {
				if (g[m] < 0)
					output() << '~';
				printAtom(g[m]);
				if (m < g.size() - 1)
					output() << " | ";
			}
		}
		output() << "." << "\n";
	}

	void openDefinition(int defid) {
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
		Assert(isTheoryOpen());
		for (auto it = d->begin(); it != d->end(); ++it) {
			(*it).second->accept(this);
		}
	}

	void visit(const PCGroundRule* b) {
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
			if (b->type() == RuleType::CONJ)
				output() << "true";
			else
				output() << "false";
		}
		output() << ".\n";
	}

	void visit(const AggGroundRule* b) {
		Assert(isTheoryOpen());
		printAtom(b->head());
		output() << " <- ";
		printAggregate(b->bound(), b->lower(), b->aggtype(), b->setnr());
	}

	void visit(const GroundAggregate* a) {
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
	}

	void visit(const CPReification* cpr) {
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
		if (right._isvarid)
			printTerm(right._varid);
		else
			output() << right._bound;
		output() << ".\n";
	}

	void visit(const CPSumTerm* cpt) {
		Assert(isTheoryOpen());
		output() << "sum[ ";
		for (auto vit = cpt->varids().cbegin(); vit != cpt->varids().cend(); ++vit) {
			printTerm(*vit);
			if (*vit != cpt->varids().back())
				output() << "; ";
		}
		output() << " ]";
	}

	void visit(const CPWSumTerm* cpt) {
		Assert(isTheoryOpen());
		output() << "wsum[ ";
		auto vit = cpt->varids().cbegin();
		auto wit = cpt->weights().cbegin();
		for (; vit != cpt->varids().cend() && wit != cpt->weights().cend(); ++vit, ++wit) {
			output() << '(';
			printTerm(*vit);
			output() << ',' << *wit << ')';
			if (*vit != cpt->varids().back())
				output() << "; ";
		}
		output() << " ]";
	}

	void visit(const CPVarTerm* cpt) {
		Assert(isTheoryOpen());
		printTerm(cpt->varid());
	}

	void visit(const PredTable* table) {
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
	}

//	void printasfunc(const PredTable* table) {
//		Assert(isTheoryOpen());
//		if (table->approxFinite()) {
//			TableIterator kt = table->begin();
//			output() << "{ ";
//			if (not kt.isAtEnd()) {
//				ElementTuple tuple = *kt;
//				if (tuple.size() > 1) {
//					output() << toString(tuple[0]);
//				}
//				for (size_t n = 1; n < tuple.size() - 1; ++n) {
//					output() << ',' << toString(tuple[n]);
//				}
//				output() << "->" << toString(tuple.back());
//				++kt;
//				for (; not kt.isAtEnd(); ++kt) {
//					output() << "; ";
//					tuple = *kt;
//					if (tuple.size() > 1) {
//						output() << toString(tuple[0]);
//					}
//					for (size_t n = 1; n < tuple.size() - 1; ++n) {
//						output() << ',' << toString(tuple[n]);
//					}
//					output() << "->" << toString(tuple.back());
//				}
//			}
//			output() << " }";
//		} else
//			output() << "possibly infinite table";
//	}

	void visit(FuncTable* table) {
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
		} else
			output() << "possibly infinite table";
	}

	void visit(const Sort* s) {
		Assert(isTheoryOpen());
		printTab();
		output() << "type " << s->name();
		if (not s->parents().empty()) {
			output() << " isa " << (*(s->parents().cbegin()))->name();
			std::set<Sort*>::const_iterator it = s->parents().cbegin();
			++it;
			for (; it != s->parents().cend(); ++it)
				output() << "," << (*it)->name();
		}
		output() << "\n";
	}

	void visit(const Predicate* p) {
		Assert(isTheoryOpen());
		printTab();
		if (p->overloaded()) { // FIXME what should happen in this case to get correct idpfiles?
			output() << "overloaded predicate " << p->name() << '\n';
		} else {
			output() << p->name().substr(0, p->name().find('/'));
			if (p->arity() > 0) {
				output() << "(" << p->sort(0)->name();
				for (unsigned int n = 1; n < p->arity(); ++n)
					output() << "," << p->sort(n)->name();
				output() << ")";
			}
			output() << "\n";
		}
	}

	void visit(const Function* f) {
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
				for (unsigned int n = 1; n < f->arity(); ++n)
					output() << "," << f->insort(n)->name();
				output() << ")";
			}
			output() << " : " << f->outsort()->name() << "\n";
		}
	}

	void visit(const SortTable* table) {
		Assert(isTheoryOpen());
		output() << "{ ";
		if(table->isRange()){
			output() <<toString(table->first()) <<".." <<toString(table->last());
		}else{
			auto it = table->sortBegin();
			if (not it.isAtEnd()) {
				output() << toString((*it));
				++it;
				for (; not it.isAtEnd(); ++it) {
					output() << "; " << toString((*it));
				}
			}
		}
		output() << " }";
	}

	void visit(const GroundSet* s) {
		Assert(isTheoryOpen());
		output() << "set_" << s->setnr() << " = [ ";
		for (size_t n = 0; n < s->size(); ++n) {
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
	}

private:
	void printAtom(int atomnr) {
		if (_translator == NULL) {
			Assert(false);
			return;
		}

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
		const std::vector<const DomainElement*>& args = _translator->getArgs(atomnr);
		// Print the atom's arguments.
		if (typeid(*pfs) == typeid(Predicate)) {
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
			Assert(typeid(*pfs) == typeid(Function));
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

	void printTerm(unsigned int termnr) {
		// Make sure there is a translator.
		Assert(_termtranslator);
		// Get information from the term translator.
		const Function* func = _termtranslator->function(termnr);
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
			const std::vector<GroundTerm>& args = _termtranslator->args(termnr);
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
			//		CPTsBody* cprelation = _termtranslator->cprelation(varid);
			//		CPReification(1,cprelation).accept(this);
		}
	}

	void printAggregate(double bound, bool lower, AggFunction aggtype, unsigned int setnr) {
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
