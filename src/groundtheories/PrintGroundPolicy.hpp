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

#include "IncludeComponents.hpp"

#include "printers/print.hpp"

#include "monitors/interactiveprintmonitor.hpp"

#include "printers/ecnfprinter.hpp"
#include "options.hpp"
#include "errorhandling/error.hpp"

struct LazyInstantiation;
class DelayGrounder;
class LazyGroundingManager;

class TsSet;

class PFSymbol;
struct GroundTerm;
class AbstractGroundTheory;
struct GroundEquivalence;

/**
 * A ground theory which does not store the grounding, but directly writes it to its monitors.
 */
class PrintGroundPolicy {
private:
	InteractivePrintMonitor* monitor_;
	Printer* printer_;

public:
	void polAddLazyAddition(const litlist&, int){
		throw notyetimplemented("Printing ground theories with lazy ground elements");
	}
	void polStartLazyFormula(LazyInstantiation*, TsType, bool){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polNotifyLazyResidual(LazyInstantiation*, TsType){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polAddLazyElement(Lit, PFSymbol*, const std::vector<GroundTerm>&, AbstractGroundTheory*, bool){
		throw notyetimplemented("Storing ground theories with lazy element constraints");
	}
	void polNotifyLazyWatch(Atom, TruthValue, LazyGroundingManager*){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polAdd(Lit, VarId){
		throw notyetimplemented("Storing denotation tseitins");
	}

	void polRecursiveDelete() {
	}

	Printer& printer() {
		return *printer_;
	}

	void initialize(InteractivePrintMonitor* monitor, Structure* str, GroundTranslator* translator) {
		monitor_ = monitor;
		printer_ = Printer::create(*monitor);
		//TODO translation option as argument to constructor
		printer().setTranslator(translator);
		printer().setStructure(str);
		printer().startTheory();
	}

	void polStartTheory(GroundTranslator*) {
	}

	void polEndTheory() {
		printer().endTheory();
	}

	void polAdd(const GroundClause& cl) {
		printer().print(cl);
	}

	void polAdd(const TsSet& tsset, SetId setnr, bool) {
		auto set = new GroundSet(setnr, tsset.literals(), tsset.weights());
		printer().print(set);
		delete (set);
	}

	void polAdd(int head, AggTsBody* body) {
		auto agg = new GroundAggregate(body->aggtype(), body->lower(), body->type(), head, body->setnr(), body->bound());
		printer().print(agg);
		delete (agg);
	}

	void polAdd(DefId defnr, const PCGroundRule& rule) {
		printer().checkOrOpen(defnr);
		printer().print(&rule);
	}

	void polAdd(DefId defnr, AggGroundRule* rule) {
		printer().checkOrOpen(defnr);
		printer().print(rule);
	}

	void polAdd(int tseitin, CPTsBody* body) {
		auto reif = new CPReification(tseitin, body);
		printer().print(reif);
		delete (reif);
	}
	
	void polAdd(const GroundEquivalence& geq){
		printer().print(geq);
	}

	void polAddOptimization(AggFunction, SetId) {
		throw notyetimplemented("Printing an optimization constraint.\n");
	}

	void polAddOptimization(VarId) {
		throw notyetimplemented("Printing an optimization constraint.\n");
	}

	void polAdd(const std::vector<std::map<Lit, Lit> >& ){
		throw notyetimplemented("Printing symmetries.\n");
	}

	std::ostream& polPut(std::ostream& s, GroundTranslator*) const {
		Assert(false);
		return s;
	}
};
