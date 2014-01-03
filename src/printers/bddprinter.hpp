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

#include "groundtheories/GroundTheory.hpp"
#include "theory/Query.hpp"
#include "groundtheories/GroundPolicy.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBdd.hpp"
#include "errorhandling/error.hpp"

#include "inferences/grounding/GroundTranslator.hpp"


template<typename Stream>
class BDDPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	const GroundTranslator* _translator;
	std::shared_ptr<FOBDDManager> _manager;

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
	BDDPrinter(Stream& stream)
			: 	StreamPrinter<Stream>(stream),
				_translator(NULL),
				_manager(FOBDDManager::createManager()) {
	}

private:
	void printFormula(const Formula* f) {
		FOBDDFactory factory(_manager);
		auto bdd = factory.turnIntoBdd(f);
		output() << ">>> Formula\n\t"<< print(f)<<"\n>>> is the BDD\n"<< print(bdd) << "\n\n";
	}
public:

	virtual void visit(const Theory* t) {
		Warning::warning("Only the sentences of the given theory are printed as BDDs.");
		for (auto sent = t->sentences().cbegin(); sent != t->sentences().cend(); sent++) {
			printFormula(*sent);
		}
	}

	virtual void startTheory() {
		openTheory();
	}
	virtual void endTheory() {
		closeTheory();
	}

	virtual void visit(const PredForm* f) {
		printFormula(f);
	}
	virtual void visit(const EqChainForm*f) {
		printFormula(f);
	}
	virtual void visit(const EquivForm*f) {
		printFormula(f);
	}
	virtual void visit(const BoolForm*f) {
		printFormula(f);
	}
	virtual void visit(const QuantForm*f) {
		printFormula(f);
	}
	virtual void visit(const AggForm*f) {
		printFormula(f);
	}

	virtual void visit(const AbstractGroundTheory*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const GroundTheory<GroundPolicy>*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const GroundDefinition*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const Rule*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const Definition*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const FixpDef*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const AggTerm*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const CPVarTerm*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const CPSetTerm*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const EnumSetExpr*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const QuantSetExpr*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const PCGroundRule*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const AggGroundRule*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const GroundSet*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const GroundAggregate*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const Vocabulary*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const Structure*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	void visit(const PredTable*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const Query*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const FOBDD*) {
		throw IdpException("FOBDDS cannot by a printer that converts theories to BDDs, use print(..) instead"); //todo
	}
	void visit(const Compound*) {
		throw notyetimplemented("Compounds cannot be printed as BDDs");
	}
	virtual void visit(const Namespace*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const UserProcedure*) {
		throw notyetimplemented("printing Procedure as BDD");
	}
	virtual void visit(const GroundClause&) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const GroundFixpDef*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const CPReification*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const VarTerm*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const DomainTerm*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
	virtual void visit(const FuncTerm*) {
		throw notyetimplemented(
				"Trying to print out theory component which cannot be printed as a BDD.");
	}
};
