/************************************
	PrintGroundTheory.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "groundtheories/PrintGroundTheory.hpp"
#include "monitors/interactiveprintmonitor.hpp"

#include "print.hpp"

PrintGroundTheory::PrintGroundTheory(InteractivePrintMonitor* monitor, AbstractStructure* str):
			AbstractGroundTheory(str),
			monitor_(monitor),
			printer_(new EcnfPrinter(true)){ //FIXME translation option as argument to constructor

}

void PrintGroundTheory::addClause(GroundClause& cl, bool skipfirst) {
	//transformForAdd(cl,VIT_DISJ,ID_FOR_UNDEFINED,skipfirst);
	//FIXME printer()->visit(cl);
}

void PrintGroundTheory::addSet(int setnr, int defnr, bool weighted) {
	if(_printedsets.find(setnr) == _printedsets.end()) {
		_printedsets.insert(setnr);
		//TsSet& tsset = getTranslator().groundset(setnr);
		//transformForAdd(tsset.literals(),VIT_SET,defnr);
		//FIXME printer()->traverse(tsset);
	}
}

void PrintGroundTheory::addFixpDef(GroundFixpDef* d) {
	printer().visit(d);
}

void PrintGroundTheory::addAggRule(int defnr, int tseitin, AggTsBody* body){}
void PrintGroundTheory::addAggregate(int head, AggTsBody* body) {
	assert(body->type() != TS_RULE);
	/*addSet(setnr,definitionID,(aggtype != AGG_CARD));
	if(_verbosity > 0) clog << "aggregate: " << _translator->printAtom(head) << ' ' << (sem == TS_RULE ? "<- " : "<=> ");
	MinisatID::Aggregate agg;
	agg.sign = lowerbound ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
	agg.setID = setnr;
	switch (aggtype) {
		case AGG_CARD:
			agg.type = MinisatID::CARD;
			if(_verbosity > 0) clog << "card ";
			break;
		case AGG_SUM:
			agg.type = MinisatID::SUM;
			if(_verbosity > 0) clog << "sum ";
			break;
		case AGG_PROD:
			agg.type = MinisatID::PROD;
			if(_verbosity > 0) clog << "prod ";
			break;
		case AGG_MIN:
			agg.type = MinisatID::MIN;
			if(_verbosity > 0) clog << "min ";
			break;
		case AGG_MAX:
			if(_verbosity > 0) clog << "max ";
			agg.type = MinisatID::MAX;
			break;
	}
	if(_verbosity > 0) clog << setnr << ' ';
	switch(sem) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL:
			agg.sem = MinisatID::COMP;
			break;
		case TS_RULE:
			agg.sem = MinisatID::DEF;
			break;
	}
	if(_verbosity > 0) clog << (lowerbound ? " >= " : " =< ") << bound << endl;
	agg.defID = definitionID;
	agg.head = createAtom(head);
	agg.bound = createWeight(bound);
	getSolver().add(agg);*/
}

void PrintGroundTheory::addDefinition(GroundDefinition* d) {
	printer().visit(d);
}

void PrintGroundTheory::addCPReification(int tseitin, CPTsBody* body) {

}

/*void PrintGroundTheory::addPCRule(int defnr, int head, vector<int> body, bool conjunctive){
	transformForAdd(body,(conjunctive ? VIT_CONJ : VIT_DISJ),defnr);
	MinisatID::Rule rule;
	rule.head = createAtom(head);
	if(_verbosity > 0) clog << (rule.conjunctive ? "conjunctive" : "disjunctive") << "rule " << _translator->printAtom(head) << " <- ";
	for(unsigned int n = 0; n < body.size(); ++n) {
		rule.body.push_back(createLiteral(body[n]));
		if(_verbosity > 0) clog << (body[n] > 0 ? "" : "~") << _translator->printAtom(body[n]) << ' ';
	}
	rule.conjunctive = conjunctive;
	rule.definitionID = defnr;
	getSolver().add(rule);
	if(_verbosity > 0) clog << endl;
}

void PrintGroundTheory::addPCRule(int defnr, int head, PCGroundRuleBody* grb) {
	addPCRule(defnr,head,grb->body(),(grb->type() == RT_CONJ));
}*/

void PrintGroundTheory::addPCRule(int defnr, int head, PCTsBody* tsb) {
//	addPCRule(defnr,head,tsb->body(),tsb->conj());
}
