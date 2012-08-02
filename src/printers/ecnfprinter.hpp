/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ECNFPRINTER_HPP_
#define ECNFPRINTER_HPP_

#include "printers/print.hpp"
#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/GroundPolicy.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

#include "inferences/SolverConnection.hpp"

#include "ECNFPrinter.hpp" // NOTE: MINISATID include!!! Can be removed with gcc4.6
using namespace SolverConnection;

template<typename Stream>
class EcnfPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	int _currenthead;
	DefId _currentdefnr;
	AbstractStructure* _structure;
	const GroundTranslator* _translator;
	std::set<VarId> _printedvarids;
	bool writeTranslation_;

	MinisatID::RealECNFPrinter<Stream>* printer;

	bool writeTranlation() const {
		return writeTranslation_;
	}

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
	EcnfPrinter(bool writetranslation, Stream& stream)
			: 	StreamPrinter<Stream>(stream),
				_currenthead(-1),
				_currentdefnr(DefId(0)),
				_structure(NULL),
				_translator(NULL),
				writeTranslation_(writetranslation),
				printer(new MinisatID::RealECNFPrinter<Stream>(new MinisatID::LiteralPrinter(), stream, false)) {

	}

	~EcnfPrinter() {
		delete (printer);
	}

	virtual void startTheory() {
		if (!isTheoryOpen()) {
			printer->notifyStart();
			openTheory();
		}
	}
	virtual void endTheory() {
		if (isTheoryOpen()) {
			printer->notifyEnd();
		}
	}

	virtual void checkOrOpen(int defid) {
		Printer::checkOrOpen(defid);
		_currentdefnr = defid;
	}

	virtual void setStructure(AbstractStructure* t) {
		_structure = t;
	}
	virtual void setTranslator(GroundTranslator* t) {
		_translator = t;
	}

	void visit(const Vocabulary*) {
		output() << "(vocabulary cannot be printed in ecnf)";
	}

	void visit(const Namespace*) {
		output() << "(namespace cannot be printed in ecnf)";
	}

	void visit(const AbstractStructure*) {
		output() << "(structure cannot be printed in ecnf)";
	}

	void visit(const Query*) {
		throw notyetimplemented("Printing queries in ecnf format");
	}

	void visit(const GroundClause& g) {
		Assert(isTheoryOpen());
		printer->add(MinisatID::Disjunction(createList(g)));
	}

	template<typename Visitor, typename List>
	void visitList(Visitor v, const List& list) {
		for (auto i = list.cbegin(); i < list.cend(); ++i) {
			CHECKTERMINATION
			(*i)->accept(v);
		}
	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		Assert(isTheoryOpen());
		setStructure(g->structure());
		setTranslator(g->translator());
		startTheory();
		for (auto i = g->getClauses().cbegin(); i < g->getClauses().cend(); ++i) {
			CHECKTERMINATION
			visit(*i);
		}
		visitList(this, g->getCPReifications());
		visitList(this, g->getSets()); //IMPORTANT: Print sets before aggregates!!
		visitList(this, g->getAggregates());
		visitList(this, g->getFixpDefinitions());
		for (auto i = g->getDefinitions().cbegin(); i != g->getDefinitions().cend(); i++) {
			CHECKTERMINATION
			_currentdefnr = (*i).second->id();
			openDefinition(_currentdefnr);
			(*i).second->accept(this);
			closeDefinition();
		}

		if (writeTranlation()) {
			output() << "=== Atomtranslation ===" << "\n";
			GroundTranslator* translator = g->translator();
			int atom = 1;
			while (translator->isStored(atom)) {
				if (translator->isInputAtom(atom)) {
					output() << atom << "|" << translator->printLit(atom) << "\n";
				}
				atom++;
			}
			output() << "==== ====" << "\n";
		}
		endTheory();
	}

	void visit(const GroundFixpDef*) {
		throw notyetimplemented("Printing ground fixpoint definitions");
	}

	void openDefinition(DefId defid) {
		Assert(isDefClosed());
		openDef(defid);
	}

	void closeDefinition() {
		Assert(!isDefClosed());
		closeDef();
	}

	void visit(const GroundDefinition* d) {
		Assert(isTheoryOpen());
		for (auto it = d->begin(); it != d->end(); ++it) {
			CHECKTERMINATION
			(*it).second->accept(this);
		}
	}

	void visit(const PCGroundRule* b) {
		Assert(isTheoryOpen());
		Assert(isDefOpen(_currentdefnr));
		printer->add(MinisatID::Rule(createAtom(b->head()), createList(b->body()), b->type() == RuleType::CONJ, _currentdefnr.id));
	}

	void visit(const AggGroundRule* a) {
		Assert(isTheoryOpen());
		Assert(isDefOpen(_currentdefnr));
		printAggregate(a->aggtype(), TsType::RULE, _currentdefnr.id, a->lower(), a->head(), a->setnr().id, a->bound());
	}

	void visit(const GroundAggregate* b) {
		Assert(isTheoryOpen());
		Assert(b->arrow()!=TsType::RULE);
		//TODO -1 should be the minisatid constant for an undefined aggregate (or create some shared ecnf format)
		printAggregate(b->type(), b->arrow(), -1, b->lower(), b->head(), b->setnr().id, b->bound());
	}

	void visit(const GroundSet* s) {
		Assert(isTheoryOpen());
		MinisatID::WLSet set(s->setnr().id);
		for (unsigned int n = 0; n < s->size(); ++n) {
			set.wl.push_back(MinisatID::WLtuple(createLiteral(s->literal(n)), s->weighted() ? createWeight(s->weight(n)) : 1));
		}
		printer->add(set);
	}

	void visit(const CPReification* cpr) { // TODO duplication with solverpolicy (probably larger parts of this file too)
		Assert(isTheoryOpen());
		CompType comp = cpr->_body->comp();
		CPTerm* left = cpr->_body->left();
		CPBound right = cpr->_body->right();
		if (isa<CPVarTerm>(*left)) {
			CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
			printCPVariable(term->varid());
			if (right._isvarid) { // CPBinaryRelVar
				printCPVariable(right._varid);
				printer->add(MinisatID::CPBinaryRelVar(createAtom(cpr->_head), term->varid().id, convert(comp), right._varid.id));
			} else { // CPBinaryRel
				printer->add(MinisatID::CPBinaryRel(createAtom(cpr->_head), term->varid().id, convert(comp), createWeight(right._bound)));
			}
		} else if (isa<CPWSumTerm>(*left)) {
			CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
			if (right._isvarid) {
				std::vector<VarId> varids = term->varids();
				std::vector<int> weights = term->weights();

				int bound = 0;
				varids.push_back(right._varid);
				weights.push_back(-1);

				addWeightedSum(cpr->_head, varids, weights, bound, comp);
			} else {
				addWeightedSum(cpr->_head, term->varids(), term->weights(), right._bound, comp);
			}
		} else {
			Assert(isa<CPWProdTerm>(*left));
			//TODO
		}
	}

private:
	void addWeightedSum(int head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound, CompType rel) {
		Assert(varids.size()==weights.size());
		for (auto i = varids.cbegin(); i < varids.cend(); ++i) {
			printCPVariable(*i);
		}
		MinisatID::weightlist w;
		for (auto i = weights.cbegin(); i < weights.cend(); ++i) {
			w.push_back(createWeight(*i));
		}
		std::vector<uint> vars;
		for (auto i = varids.cbegin(); i < varids.cend(); ++i) {
			vars.push_back(i->id);
		}
		printer->add(MinisatID::CPSumWeighted(createAtom(head), vars, w, convert(rel), createWeight(bound)));
	}

	void printAggregate(AggFunction aggtype, TsType arrow, DefId defnr, bool geqthanbound, int head, SetId setnr, double bound) {
		auto newsem = MinisatID::AggSem::COMP;
		auto newhead = head;
		auto newbound = bound;
		auto newsign = geqthanbound ? MinisatID::AggSign::UB : MinisatID::AggSign::LB;
		switch (arrow) {
		case TsType::EQ:
			break;
		case TsType::IMPL: // not head or aggregate
			newsem = MinisatID::AggSem::OR;
			newhead = -head;
			break;
		case TsType::RIMPL: // head or not aggregate
			newsem = MinisatID::AggSem::OR;
			if (newsign == MinisatID::AggSign::LB) {
				newsign = MinisatID::AggSign::UB;
				newbound -= 1;
			} else {
				newsign = MinisatID::AggSign::LB;
				newbound += 1;
			}
			break;
		case TsType::RULE:
			newsem = MinisatID::AggSem::DEF;
			break;
		}
		printer->add(MinisatID::Aggregate(createLiteral(newhead), setnr.id, createWeight(newbound), convert(aggtype), newsign, newsem, defnr.id));
	}

	void printCPVariables(std::vector<VarId> varids) {
		for (auto it = varids.cbegin(); it != varids.cend(); ++it) {
			printCPVariable(*it);
		}
	}

	void printCPVariable(VarId varid) {
		if (_printedvarids.find(varid) == _printedvarids.cend()) {
			_printedvarids.insert(varid);

			Assert(_translator != NULL);
			auto domain = _translator->domain(varid);
			if (domain->isRange()) {
				int minvalue = domain->first()->value()._int;
				int maxvalue = domain->last()->value()._int;
				printer->add(MinisatID::IntVarRange(varid.id, minvalue, maxvalue));
			} else {
				std::vector<MinisatID::Weight> valuelist;
				for (auto it = domain->sortBegin(); not it.isAtEnd(); ++it) {
					CHECKTERMINATION
					Assert((*it)->type()==DomainElementType::DET_INT);
					valuelist.push_back(createWeight((*it)->value()._int));
				}
				printer->add(MinisatID::IntVarEnum(varid.id, valuelist));
			}
		}
	}
};

#endif /* ECNFPRINTER_HPP_ */
