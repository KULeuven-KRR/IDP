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
#include "inferences/grounding/GroundTermTranslator.hpp"

template<typename Stream>
class EcnfPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	int _currenthead;
	unsigned int _currentdefnr;
	AbstractStructure* _structure;
	const GroundTermTranslator* _termtranslator;
	std::set<unsigned int> _printedvarids;
	bool writeTranslation_;

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
			: StreamPrinter<Stream>(stream), _currenthead(-1), _currentdefnr(0), _structure(NULL), _termtranslator(NULL), writeTranslation_(writetranslation) {

	}

	virtual void startTheory() {
		if (!isTheoryOpen()) {
			output() << "p ecnf\n";
			openTheory();
		}
	}
	virtual void endTheory() {
		if (isTheoryOpen()) {
			closeTheory();
		}
	}

	virtual void checkOrOpen(int defid) {
		Printer::checkOrOpen(defid);
		_currentdefnr = defid;
	}

	virtual void setStructure(AbstractStructure* t) {
		_structure = t;
	}
	virtual void setTermTranslator(GroundTermTranslator* t) {
		_termtranslator = t;
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

	void visit(const GroundClause& g) {
		Assert(isTheoryOpen());
		for (unsigned int m = 0; m < g.size(); ++m) {
			output() << g[m] << ' ';
		}
		output() << '0' << "\n";
	}

	template<typename Visitor, typename List>
	void visitList(Visitor v, const List& list) {
		for (auto i = list.cbegin(); i < list.cend(); ++i) {
			(*i)->accept(v);
		}
	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		Assert(isTheoryOpen());
		setStructure(g->structure());
		setTermTranslator(g->termtranslator());
		startTheory();
		for (auto i = g->getClauses().cbegin(); i < g->getClauses().cend(); ++i) {
			visit(*i);
		}
		visitList(this, g->getCPReifications());
		visitList(this, g->getSets()); //IMPORTANT: Print sets before aggregates!!
		visitList(this, g->getAggregates());
		visitList(this, g->getFixpDefinitions());
		for (auto i = g->getDefinitions().cbegin(); i != g->getDefinitions().cend(); i++) {
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
		/*TODO not implemented*/
		output() << "c Warning, fixpoint definitions are not printed yet\n";
	}

	void openDefinition(int defid) {
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
			(*it).second->accept(this);
		}
	}

	void visit(const PCGroundRule* b) {
		Assert(isTheoryOpen());
		Assert(isDefOpen(_currentdefnr));
		output() << (b->type() == RuleType::CONJ ? "C " : "D ");
		output() << "<- " << _currentdefnr << ' ' << b->head() << ' ';
		for (unsigned int n = 0; n < b->size(); ++n) {
			output() << b->literal(n) << ' ';
		}
		output() << "0\n";
	}

	void visit(const AggGroundRule* a) {
		Assert(isTheoryOpen());
		Assert(isDefOpen(_currentdefnr));
		printAggregate(a->aggtype(), TsType::RULE, _currentdefnr, a->lower(), a->head(), a->setnr(), a->bound());
	}

	void visit(const GroundAggregate* b) {
		Assert(isTheoryOpen());
		Assert(b->arrow()!=TsType::RULE);
		//TODO -1 should be the minisatid constant for an undefined aggregate (or create some shared ecnf format)
		printAggregate(b->type(), b->arrow(), -1, b->lower(), b->head(), b->setnr(), b->bound());
	}

	void visit(const GroundSet* s) {
		Assert(isTheoryOpen());
		output() << (s->weighted() ? "WSet" : "Set") << ' ' << s->setnr();
		for (unsigned int n = 0; n < s->size(); ++n) {
			output() << ' ' << s->literal(n);
			if (s->weighted())
				output() << '=' << s->weight(n);
		}
		output() << " 0\n";
	}

	void visit(const CPReification* cpr) {
		Assert(isTheoryOpen());
		CompType comp = cpr->_body->comp();
		CPTerm* left = cpr->_body->left();
		CPBound right = cpr->_body->right();
		if (sametypeid<CPVarTerm>(*left)) {
			CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
			printCPVariable(term->varid());
			if (right._isvarid) { // CPBinaryRelVar
				printCPVariable(right._varid);
				printCPReification("BINTRT", cpr->_head, term->varid(), comp, right._varid);
			} else { // CPBinaryRel
				printCPReification("BINTRI", cpr->_head, term->varid(), comp, right._bound);
			}
		} else if (sametypeid<CPSumTerm>(*left)) {
			CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
			std::vector<int> weights;
			weights.resize(term->varids().size(), 1);

			if (right._isvarid) {
				std::vector<VarId> varids = term->varids();
				int bound = 0;
				varids.push_back(right._varid);
				weights.push_back(-1);

				addWeightedSum(cpr->_head, varids, weights, bound, comp);
			} else {
				addWeightedSum(cpr->_head, term->varids(), weights, right._bound, comp);
			}
		} else {
			Assert(sametypeid<CPWSumTerm>(*left));
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
		}
	}

private:

	void addWeightedSum(int head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound, CompType rel) {
		Assert(varids.size()==weights.size());
		for (auto i = varids.cbegin(); i < varids.cend(); ++i) {
			printCPVariable(*i);
		}
		printCPReification("SUMSTSIRI", head, varids, weights, rel, bound);
	}

	void printAggregate(AggFunction aggtype, TsType arrow, unsigned int defnr, bool lower, int head, unsigned int setnr, double bound) {
		switch (aggtype) {
		case AggFunction::CARD:
			output() << "Card ";
			break;
		case AggFunction::SUM:
			output() << "Sum ";
			break;
		case AggFunction::PROD:
			output() << "Prod ";
			break;
		case AggFunction::MIN:
			output() << "Min ";
			break;
		case AggFunction::MAX:
			output() << "Max ";
			break;
		}
		// FIXME:
		Warning::warning("The ecnf output format does not support implications for aggregates yet, so currently replacing them with equivalences!\n");
		switch (arrow) {
		case TsType::EQ:
		case TsType::IMPL:
		case TsType::RIMPL: // (Reverse) implication is not supported by solver (yet)
			output() << "C ";
			break;
		case TsType::RULE:
			Assert(isDefOpen(defnr));
			output() << "<- " << defnr << ' ';
			break;
		}
		output() << (lower ? 'G' : 'L') << ' ' << head << ' ' << setnr << ' ' << bound << " 0" << "\n";
	}

	void printCPVariables(std::vector<unsigned int> varids) {
		for (auto it = varids.cbegin(); it != varids.cend(); ++it) {
			printCPVariable(*it);
		}
	}

	void printCPVariable(unsigned int varid) {
		if (_printedvarids.find(varid) == _printedvarids.cend()) {
			_printedvarids.insert(varid);
			SortTable* domain = _termtranslator->domain(varid);
			if (domain->isRange()) {
				int minvalue = domain->first()->value()._int;
				int maxvalue = domain->last()->value()._int;
				output() << "INTVAR " << varid << ' ' << minvalue << ' ' << maxvalue << ' ';
			} else {
				output() << "INTVARDOM " << varid << ' ';
				for (SortIterator it = domain->sortBegin(); not it.isAtEnd(); ++it) {
					int value = (*it)->value()._int;
					output() << value << ' ';
				}
			}
			output() << '0' << "\n";
		}
	}

	std::string toString(CompType comp) {
		switch (comp) {
		case CompType::EQ:
			return "=";
		case CompType::NEQ:
			return "~=";
		case CompType::LEQ:
			return "=<";
		case CompType::GEQ:
			return ">=";
		case CompType::GT:
			return ">";
		case CompType::LT:
			return "<";
		}Assert(false);
		return "";
	}

	void printCPReification(std::string type, int head, unsigned int left, CompType comp, long right) {
		output() << type << ' ' << head << ' ' << left << ' ' << toString(comp) << ' ' << right << " 0" << "\n";
	}

	void printCPReification(std::string type, int head, std::vector<unsigned int> varids, std::vector<int> weights, CompType comp, long right) {
		output() << type << ' ' << head << ' ';
		for (auto it = varids.cbegin(); it != varids.cend(); ++it) {
			output() << *it << ' ';
		}
		output() << " | ";
		for (auto it = weights.cbegin(); it != weights.cend(); ++it) {
			output() << *it << ' ';
		}
		output() << toString(comp) << ' ' << right << " 0" << "\n";
	}
};

#endif /* ECNFPRINTER_HPP_ */
