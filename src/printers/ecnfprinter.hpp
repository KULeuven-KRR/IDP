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
#include "errorhandling/error.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/GroundPolicy.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

#include "inferences/SolverConnection.hpp"

#include "groundtheories/IDP2ECNF.hpp"

using namespace SolverConnection;

template<typename Stream>
class EcnfPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	int _currenthead;
	DefId _currentdefnr;
	Structure* _structure;
	GroundTranslator* _translator;
	bool writeTranslation_;

	class PrintThroughMinisatID{
	private:
		MinisatID::RealECNFPrinter<Stream>* printer;
	public:
		PrintThroughMinisatID(Stream& stream):
			printer(new MinisatID::RealECNFPrinter<Stream>(new MinisatID::LiteralPrinter(), stream, false)){

		}
		~PrintThroughMinisatID(){
			delete(printer);
		}

		template<class Obj>
		void operator() (Obj o){
			printer->add(o);
		}

		void notifyStart(){
			printer->notifyStart();
		}
		void notifyEnd(){
			printer->notifyEnd();
		}
	};

	PrintThroughMinisatID* subprinter;
	IDP2ECNF<PrintThroughMinisatID&> printer;

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
				subprinter(new PrintThroughMinisatID(stream)),
				printer(*subprinter, _translator) {

	}

	~EcnfPrinter(){
		delete(subprinter);
	}

	virtual void startTheory() {
		if (!isTheoryOpen()) {
			printer.getExec().notifyStart();
			openTheory();
		}
	}
	virtual void endTheory() {
		if (isTheoryOpen()) {
			printer.getExec().notifyEnd();
		}
	}

	virtual void checkOrOpen(DefId defid) {
		Printer::checkOrOpen(defid);
		_currentdefnr = defid;
	}

	virtual void setStructure(Structure* t) {
		_structure = t;
	}
	virtual void setTranslator(GroundTranslator* t) {
		_translator = t;
		printer.setTranslator(t);
	}

	void visit(const GroundClause& g) {
		Assert(isTheoryOpen());
		printer.add(g);
	}

	template<typename Visitor, typename List>
	void visitList(Visitor v, const List& list) {
		for (auto elem : list) {
			CHECKTERMINATION
			elem->accept(v);
		}
	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		Assert(isTheoryOpen());
		setStructure(g->structure());
		setTranslator(g->translator());
		startTheory();
		for (auto clause : g->getClauses()) {
			CHECKTERMINATION
			visit(clause);
		}
		visitList(this, g->getCPReifications());
		visitList(this, g->getSets()); //IMPORTANT: Print sets before aggregates!!
		visitList(this, g->getAggregates());
		visitList(this, g->getFixpDefinitions());
		for (auto def : g->getDefinitions()) {
			CHECKTERMINATION
			_currentdefnr = def.second->id();
			openDefinition(_currentdefnr);
			def.second->accept(this);
			closeDefinition();
		}

		if (writeTranlation()) {
			output() << "=== Atomtranslation ===" << "\n";
			auto translator = g->translator();
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
		for (auto rule : d->rules()) {
			CHECKTERMINATION
			rule.second->accept(this);
		}
	}

	void visit(const PCGroundRule* b) {
		Assert(isTheoryOpen());
		Assert(isDefOpen(_currentdefnr));
		printer.add(_currentdefnr, *b);
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
		printer.add(s->_setnr, s->_setlits, s->weighted(), s->_litweights);
	}

	void visit(const CPReification* cpr) {
		Assert(isTheoryOpen());
		printer.add(cpr->_head, cpr->_body);
	}

private:
	void printAggregate(AggFunction aggtype, TsType arrow, DefId defnr, bool geqthanbound, int head, SetId setnr, double bound) {
		printer.add(defnr, head, bound, geqthanbound, setnr, aggtype, arrow);
	}

protected:
	void visit(const Vocabulary*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	void visit(const Namespace*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	void visit(const Structure*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	void visit(const PredTable*){
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	void visit(const Query*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	void visit(const GroundFixpDef*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const Theory*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const PredForm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const EqChainForm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const EquivForm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const BoolForm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const QuantForm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const AggForm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const VarTerm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const FuncTerm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const DomainTerm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const AbstractGroundTheory*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const Rule*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const Definition*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const FixpDef*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const AggTerm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const CPVarTerm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const CPSetTerm*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const EnumSetExpr*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const QuantSetExpr*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const UserProcedure*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	virtual void visit(const FOBDD*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
	void visit(const Compound*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in ECNF format.");
	}
};
