/************************************
	PrintGroundTheory.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "groundtheories/PrintGroundTheory.hpp"
#include "monitors/interactiveprintmonitor.hpp"

#include "printers/ecnfprinter.hpp"
#include "ground.hpp"
#include "options.hpp"
#include "error.hpp"

PrintGroundTheory::PrintGroundTheory(InteractivePrintMonitor* monitor, AbstractStructure* str, Options* opts):
			AbstractGroundTheory(str),
			monitor_(monitor),
			printer_(Printer::create(opts, *monitor)){ //FIXME translation option as argument to constructor
	printer().setTranslator(translator());
	printer().setTermTranslator(termtranslator());
	printer().setStructure(str);
}

void PrintGroundTheory::addClause(GroundClause& cl, bool skipfirst) {
	transformForAdd(cl,VIT_DISJ,getIDForUndefined(),skipfirst);
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

		_printedsets.insert(setnr);
	}
}

void PrintGroundTheory::addFixpDef(GroundFixpDef* d) {
	printer().visit(d);
}

void PrintGroundTheory::addAggregate(int head, AggTsBody* body) {
	addSet(body->setnr(),getIDForUndefined(),(body->aggtype() != AGG_CARD));
	GroundAggregate* agg = new GroundAggregate(body->aggtype(),body->lower(),body->type(),head,body->setnr(),body->bound());
	printer().visit(agg);
	delete(agg);
}

void PrintGroundTheory::addPCRule(int defnr, int tseitin, PCTsBody* body, bool recursive) {
	transformForAdd(body->body(),(body->conj() ? VIT_CONJ : VIT_DISJ), defnr);
	PCGroundRuleBody* rule = new PCGroundRuleBody(body->conj()?RuleType::RT_CONJ:RuleType::RT_DISJ, body->body(), recursive);
	printer().visit(defnr, tseitin, rule);
	delete(rule);
}

void PrintGroundTheory::addAggRule(int defnr, int tseitin, AggTsBody* body, bool) {
	addSet(body->setnr(),defnr,(body->aggtype() != AGG_CARD));
	GroundAggregate* agg = new GroundAggregate(body->aggtype(),body->lower(),body->type(),tseitin,body->setnr(),body->bound());
	printer().visit(defnr, agg);
	delete(agg);
}

void PrintGroundTheory::addDefinition(GroundDefinition* d) {
	printer().visit(d);
}

void PrintGroundTheory::addCPReification(int tseitin, CPTsBody* body) {
	CPTsBody* foldedbody = new CPTsBody(body->type(),foldCPTerm(body->left()),body->comp(),body->right());
	CPReification* reif = new CPReification(tseitin,foldedbody);

	printer().visit(reif);

	delete(reif);
}

void PrintGroundTheory::accept(TheoryVisitor* v) const	{
	assert(false); Error::error("When not storing the grounding, it cannot be visited.");
}
AbstractTheory*	PrintGroundTheory::accept(TheoryMutatingVisitor* v)	{
	assert(false); Error::error("When not storing the grounding, it cannot be visited.");
	return NULL;
}
