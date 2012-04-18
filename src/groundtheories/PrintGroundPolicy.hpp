/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef PRINTGROUNDTHEORY_HPP_
#define PRINTGROUNDTHEORY_HPP_

#include "IncludeComponents.hpp"

#include "printers/print.hpp"

#include "monitors/interactiveprintmonitor.hpp"

#include "printers/ecnfprinter.hpp"
#include "options.hpp"
#include "errorhandling/error.hpp"

class ResidualAndFreeInst;
class LazyGroundingManager;
class DelayGrounder;

class TsSet;

/**
 * A ground theory which does not store the grounding, but directly writes it to its monitors.
 */
class PrintGroundPolicy {
private:
	InteractivePrintMonitor* monitor_;
	Printer* printer_;

public:
	void polNotifyUnknBound(Context, const Lit&, const ElementTuple&, std::vector<DelayGrounder*>){}
	void polNotifyLazyResidual(ResidualAndFreeInst*, TsType, LazyGroundingManager const* const){}

	void polRecursiveDelete() {
	}

	Printer& printer() {
		return *printer_;
	}

	void initialize(InteractivePrintMonitor* monitor, AbstractStructure* str, GroundTranslator* translator, GroundTermTranslator* termtranslator) {
		monitor_ = monitor;
		printer_ = Printer::create(*monitor);
		//TODO translation option as argument to constructor
		printer().setTranslator(translator);
		printer().setTermTranslator(termtranslator);
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

	void polAdd(const TsSet& tsset, int setnr, bool) {
		auto set = new GroundSet(setnr, tsset.literals(), tsset.weights());
		printer().print(set);
		delete (set);
	}

	void polAdd(int head, AggTsBody* body) {
		auto agg = new GroundAggregate(body->aggtype(), body->lower(), body->type(), head, body->setnr(), body->bound());
		printer().print(agg);
		delete (agg);
	}

	void polAdd(int defnr, PCGroundRule* rule) {
		printer().checkOrOpen(defnr);
		printer().print(rule);
		delete (rule);
	}

	void polAdd(int defnr, AggGroundRule* rule) {
		printer().checkOrOpen(defnr);
		printer().print(rule);
	}

	void polAdd(int tseitin, CPTsBody* body) {
		auto reif = new CPReification(tseitin, body);
		printer().print(reif);
		delete (reif);
	}

	void polAddOptimization(AggFunction, int) {
		throw notyetimplemented("Printing an optimization constraint.\n");
	}

	void polAddOptimization(VarId) {
		throw notyetimplemented("Printing an optimization constraint.\n");
	}

	std::ostream& polPut(std::ostream& s, GroundTranslator*, GroundTermTranslator*) const {
		Assert(false);
		return s;
	}
	std::string polToString(GroundTranslator*, GroundTermTranslator*) const {
		Assert(false);
		return "";
	}
};

#endif /* PRINTGROUNDTHEORY_HPP_ */
