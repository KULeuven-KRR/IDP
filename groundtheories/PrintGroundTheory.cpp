/************************************
	PrintGroundTheory.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "groundtheories/PrintGroundTheory.hpp"
#include "monitors/interactiveprintmonitor.hpp"

#include "print.hpp"
#include "ground.hpp"
#include "ecnf.hpp"

const int ID_FOR_UNDEFINED = -1; // FIXME REMOVE

PrintGroundTheory::PrintGroundTheory(InteractivePrintMonitor* monitor, AbstractStructure* str):
			AbstractGroundTheory(str),
			monitor_(monitor),
			printer_(new EcnfPrinter(true, *monitor)){ //FIXME translation option as argument to constructor

}

void PrintGroundTheory::addClause(GroundClause& cl, bool skipfirst) {
	transformForAdd(cl,VIT_DISJ,ID_FOR_UNDEFINED,skipfirst);
	printer().visit(cl);
}

void PrintGroundTheory::addSet(int setnr, int defnr, bool weighted) {
	if(_printedsets.find(setnr) == _printedsets.end()) {
		TsSet& tsset = _translator->groundset(setnr);
		transformForAdd(tsset.literals(),VIT_SET,defnr);
		std::vector<double> weights;
		if(weighted) weights = tsset.weights();
		GroundSet* set = new GroundSet(setnr,tsset.literals(),weights);
		printer().visit(set);
		delete(set);
	}
}

void PrintGroundTheory::addFixpDef(GroundFixpDef* d) {
	printer().visit(d);
}

void PrintGroundTheory::addAggregate(int head, AggTsBody* body) {
	addSet(body->setnr(),ID_FOR_UNDEFINED,(body->aggtype() != AGG_CARD));
	GroundAggregate* agg = new GroundAggregate(body->aggtype(),body->lower(),body->type(),head,body->setnr(),body->bound());
	printer().visit(agg);
	delete(agg);
}

void PrintGroundTheory::addPCRule(int defnr, int tseitin, PCTsBody* body) {
	transformForAdd(body->body(),(body->conj() ? VIT_CONJ : VIT_DISJ), defnr);
	//FIXME
/*	PCGroundRuleBody* rule = new PCGroundRuleBody(body->conj()?RuleType::RT_CONJ:RuleType::RT_DISJ, tseitin,body->body());
	printer().visit(rule);
	delete(rule);*/
}

void PrintGroundTheory::addAggRule(int defnr, int tseitin, AggTsBody* body) {
	addSet(body->setnr(),defnr,(body->aggtype() != AGG_CARD));
	//FIXME
/*	AggGroundRuleBody* agg = new AggGroundRuleBody(tseitin, body->setnr(), body->aggtype(),body->lower(),body->bound(), TODO recursive???);
	printer().visit(agg);
	delete(agg);*/
}

void PrintGroundTheory::addDefinition(GroundDefinition* d) {
	int defnr = 1; //FIXME: this should not be necessary
	printer().visit(d);
	// FIXME maybe this also not
	for(auto it = d->begin(); it != d->end(); ++it) {
		int head = it->first;
		if(_printedtseitins.find(head) == _printedtseitins.end()) {
			GroundRuleBody* grb = it->second;
			if(typeid(*grb) == typeid(PCGroundRuleBody)) {
				PCGroundRuleBody* pcgrb = dynamic_cast<PCGroundRuleBody*>(grb);
				transformForAdd(pcgrb->body(),(pcgrb->type() == RT_CONJ ? VIT_CONJ : VIT_DISJ),defnr);
			}
			else {
				assert(typeid(*grb) == typeid(AggGroundRuleBody));
				AggGroundRuleBody* agggrb = dynamic_cast<AggGroundRuleBody*>(grb);
				addSet(agggrb->setnr(),defnr,(agggrb->aggtype() != AGG_CARD));
			}
		}
	}
}

void PrintGroundTheory::addCPReification(int tseitin, CPTsBody* body) {
	CPTsBody* foldedbody = new CPTsBody(body->type(),foldCPTerm(body->left()),body->comp(),body->right());
	CPReification* reif = new CPReification(tseitin,foldedbody);

	printer().visit(reif);

	delete(reif);
}
