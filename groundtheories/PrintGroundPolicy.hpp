/************************************
	PrintGroundTheory.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTGROUNDTHEORY_HPP_
#define PRINTGROUNDTHEORY_HPP_

#include <string>
#include <vector>
#include <ostream>

#include "theory.hpp"
#include "ecnf.hpp"
#include "commontypes.hpp"

#include "printers/print.hpp"

#include "monitors/interactiveprintmonitor.hpp"

#include "printers/ecnfprinter.hpp"
#include "options.hpp"
#include "error.hpp"

class TsSet;

/**
 * A ground theory which does not store the grounding, but directly writes it to its monitors.
 */
//TODO monitor policy
class PrintGroundPolicy {
private:
	InteractivePrintMonitor* monitor_;
	Printer*			printer_;

public:
	void polRecursiveDelete() { }

	Printer&	printer() { return *printer_; }

	void initialize(InteractivePrintMonitor* monitor, AbstractStructure* str, GroundTranslator* translator, GroundTermTranslator* termtranslator, Options* opts){
		monitor_ = monitor;
		printer_ = Printer::create(opts, *monitor);
		//TODO translation option as argument to constructor
		printer().setTranslator(translator);
		printer().setTermTranslator(termtranslator);
		printer().setStructure(str);
		printer().startTheory();
	}

	void polStartTheory(GroundTranslator* ){
	}

	void polEndTheory(){
		printer().endTheory();
	}

	void polAdd(const GroundClause& cl) {
		printer().visit(cl);
	}

	void polAdd(const TsSet& tsset, int setnr, bool weighted) {
		GroundSet* set = new GroundSet(setnr,tsset.literals(),tsset.weights());
		printer().visit(set);
		delete(set);
	}

	void polAdd(int head, AggTsBody* body) {
		GroundAggregate* agg = new GroundAggregate(body->aggtype(),body->lower(),body->type(),head,body->setnr(),body->bound());
		printer().visit(agg);
		delete(agg);
	}


	void polAdd(GroundDefinition* def){
		for(auto i=def->begin(); i!=def->end(); ++i){
			if(typeid(PCGroundRule*)==typeid((*i).second)){
				polAdd(def->id(), dynamic_cast<PCGroundRule*>((*i).second));
			}else{
				polAdd(def->id(), dynamic_cast<AggGroundRule*>((*i).second));
			}
		}
	}

	void polAdd(int defnr, PCGroundRule* rule) {
		printer().checkOrOpen(defnr);
		printer().visit(rule);
		delete(rule);
	}

	void polAdd(int defnr, AggGroundRule* rule) {
		printer().checkOrOpen(defnr);
		printer().visit(rule);
	}

	void polAdd(int tseitin, CPTsBody* body) {
		CPReification* reif = new CPReification(tseitin,body);
		printer().visit(reif);
		delete(reif);
	}

	std::ostream& 	polPut(std::ostream& s, GroundTranslator*, GroundTermTranslator*, bool longnames = false)	const { assert(false); return s;	}
	std::string 	polToString(GroundTranslator*, GroundTermTranslator*, bool longnames = false) 				const { assert(false); return "";	}
};

#endif /* PRINTGROUNDTHEORY_HPP_ */
