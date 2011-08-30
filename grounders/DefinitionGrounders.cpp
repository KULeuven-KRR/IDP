/************************************
 DefinitionGrounders.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "grounders/DefinitionGrounders.hpp"

#include "grounders/TermGrounders.hpp"

#include "grounders/FormulaGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"

#include "generator.hpp"
#include "checker.hpp"
#include "common.hpp"
#include "vocabulary.hpp"
#include "term.hpp"

#include <iostream>

using namespace std;

HeadGrounder::HeadGrounder(AbstractGroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s,
		const vector<TermGrounder*>& sg, const vector<SortTable*>& vst)
	: _grounding(gt), _subtermgrounders(sg), _truechecker(pc), _falsechecker(cc), _symbol(gt->translator()->addSymbol(s)), _tables(vst) {
}

int HeadGrounder::run() const {
	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n]._isvarid) {
			alldomelts = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
		}
	}
	//XXX All subterm grounders should return domain elements.
	assert(alldomelts);

	// Checking partial functions
	for(unsigned int n = 0; n < args.size(); ++n) {
		//TODO: only check positions that can be out of bounds or ...!
		//TODO: produce a warning!
		if(not args[n]) return _false;
		if(not _tables[n]->contains(args[n])) return _false;
	}

	// Run instance checkers and return grounding
	int atom = _grounding->translator()->translate(_symbol,args);
	if(_truechecker->isInInterpretation(args)) {
		_grounding->addUnitClause(atom);
	}else if(_falsechecker->isInInterpretation(args)) {
		_grounding->addUnitClause(-atom);
	}
	return atom;
}

void RuleGrounder::run(unsigned int defid, GroundDefinition* grounddefinition) const {
	bool conj = _bodygrounder->conjunctive();
	if(not _bodygenerator->first()){
		return;
	}
	do {
		vector<int>	body;
		_bodygrounder->run(body);
		if(not _headgenerator->first()) {
			continue;
		}
		bool falsebody = (body.empty() && !conj) || (body.size() == 1 && body[0] == _false);
		bool truebody = (body.empty() && conj) || (body.size() == 1 && body[0] == _true);
		if(falsebody){
			continue;
		}

		do{
			Lit head = _headgrounder->run();
			assert(head != _true);
			if(head != _false) {
				if(truebody){
					addTrueRule(grounddefinition, head);
				} else{
					addPCRule(grounddefinition, head,body,conj,_context._tseitin == TsType::RULE);
				}
			}
		}while(_headgenerator->next());
	}while(_bodygenerator->next());
}

void RuleGrounder::addTrueRule(GroundDefinition* const grounddefinition, int head) const {
	addPCRule(grounddefinition, head,vector<int>(0),true,false);
}

void RuleGrounder::addFalseRule(GroundDefinition* const grounddefinition, int head) const {
	addPCRule(grounddefinition, head,vector<int>(0),false,false);
}

void RuleGrounder::addPCRule(GroundDefinition* grounddefinition, int head, const vector<int>& body, bool conj, bool recursive) const {
	grounddefinition->addPCRule(head, body, conj, recursive);
}

void RuleGrounder::addAggRule(GroundDefinition* grounddefinition, int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive) const {
	grounddefinition->addAggRule(head, setnr, aggtype, lower, bound, recursive);
}

unsigned int DefinitionGrounder::_currentdefnb = 1;

DefinitionGrounder::DefinitionGrounder(AbstractGroundTheory* gt, std::vector<RuleGrounder*> subgr,int verb)
		: TopLevelGrounder(gt,verb), _defnb(_currentdefnb++), _subgrounders(subgr), _grounddefinition(new GroundDefinition(_defnb, gt->translator())) {
}

bool DefinitionGrounder::run() const {
	for(auto grounder = _subgrounders.begin(); grounder<_subgrounders.end(); ++grounder){
		(*grounder)->run(id(), _grounddefinition);
	}
	return true;
}
