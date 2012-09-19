/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef BDDPRINTER_HPP_
#define BDDPRINTER_HPP_

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

//TODO is not guaranteed to generate correct idp files!
// FIXME do we want this? Because printing cp constraints etc. should be done correctly then!
//TODO usage of stored parameters might be incorrect in some cases.

template<typename Stream>
class BDDPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	const GroundTranslator* _translator;
	FOBDDManager* _manager;

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
				_manager(new FOBDDManager()) {
	}

private:
	void printFormula(const Formula* f) {
		FOBDDFactory factory(_manager);
		auto bdd = factory.turnIntoBdd(f);
		output() << "FORMULA\n"<< toString(f)<<"\nBECOMES \n"<< toString(bdd) << "\n\n";
	}
public:

	virtual void visit(const Theory* t) {
		Warning::warning("Only printing formulas as bdd is supported. I will ignore ALL definitions etc.");
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
		throw notyetimplemented("printing Vocabulary as BDD");
	}
	virtual void visit(const PCGroundRule*) {
		throw notyetimplemented("printing Vocabulary as BDD");
	}
	virtual void visit(const AggGroundRule*) {
		throw notyetimplemented("printing Vocabulary as BDD");
	}
	virtual void visit(const GroundSet*) {
		throw notyetimplemented("printing Vocabulary as BDD");
	}
	virtual void visit(const GroundAggregate*) {
		throw notyetimplemented("printing Vocabulary as BDD");
	}

	virtual void visit(const Vocabulary*) {
		throw notyetimplemented("printing Vocabulary as BDD");
	}
	virtual void visit(const AbstractStructure*) {
		throw notyetimplemented("printing AbstractStructure as BDD");
	}
	virtual void visit(const Query*) {
		throw notyetimplemented("printing Query as BDD");
	}
	virtual void visit(const Namespace*) {
		throw notyetimplemented("printing Namespace as BDD");
	}
	virtual void visit(const GroundClause&) {
		throw notyetimplemented("printing GroundClause as BDD");
	}
	virtual void visit(const GroundFixpDef*) {
		throw notyetimplemented("printing GroundFixpDef as BDD");
	}
	virtual void visit(const CPReification*) {
		throw notyetimplemented("printing CPReification as BDD");
	}
};
#endif //BDDPRINTER_HPP_
