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
				_manager(make_shared<FOBDDManager>()) {
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

	virtual void visit(const GroundDefinition*) {
		throw IdpException("Ground definitions cannot be printed as BDDs");
	}
	virtual void visit(const PCGroundRule*) {
		throw IdpException("Ground rules cannot be printed as BDDs");
	}
	virtual void visit(const AggGroundRule*) {
		throw IdpException("Ground rules cannot be printed as BDDs");
	}
	virtual void visit(const GroundSet*) {
		throw IdpException("Ground sets cannot be printed as BDDs");
	}
	virtual void visit(const GroundAggregate*) {
		throw IdpException("Ground aggregates cannot be printed as BDDs");
	}
	virtual void visit(const Vocabulary*) {
		throw IdpException("Vocabularies cannot be printed as BDDs");
	}
	virtual void visit(const Structure*) {
		throw IdpException("Structures cannot be printed as BDDs");
	}
	void visit(const PredTable*){
		throw IdpException("Tables cannot be printed as BDDs");
	}
	virtual void visit(const Query*) {
		throw IdpException("Queries cannot be printed as BDDs");
	}
	virtual void visit(const FOBDD*) {
		throw IdpException("FOBDDS cannot be printed as BDDs TODO"); //todo
	}
	virtual void visit(const Namespace*) {
		throw IdpException("Namespaces cannot be printed as BDDs");
	}
	virtual void visit(const UserProcedure*) {
		throw notyetimplemented("printing Procedure as BDD");
	}
	virtual void visit(const GroundClause&) {
		throw IdpException("Ground clauses cannot be printed as BDDs");
	}
	virtual void visit(const GroundFixpDef*) {
		throw IdpException("Ground fixpoint definitions cannot be printed as BDDs");
	}
	virtual void visit(const CPReification*) {
		throw IdpException("CP reifications cannot be printed as BDDs");
	}
};
