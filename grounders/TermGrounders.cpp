/************************************
 TermGrounders.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "grounders/TermGrounders.hpp"

#include "grounders/SetGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"

#include "common.hpp"
#include "vocabulary.hpp"
#include "term.hpp"

#include <iostream>

using namespace std;

void TermGrounder::setorig(const Term* t, const map<Variable*,const DomElemContainer*>& mvd, int verb) {
	_verbosity = verb;
	map<Variable*,Variable*> mvv;
	for(set<Variable*>::const_iterator it = t->freevars().begin(); it != t->freevars().end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		_varmap[v] = mvd.find(*it)->second;
	}
	_origterm = t->clone(mvv);
}

void TermGrounder::printorig() const {
	clog << "Grounding term " << _origterm->to_string();
	if(not _origterm->freevars().empty()) {
		clog << " with instance ";
		for(set<Variable*>::const_iterator it = _origterm->freevars().begin(); it != _origterm->freevars().end(); ++it) {
			clog << (*it)->to_string() << " = ";
			const DomainElement* e = _varmap.find(*it)->second->get();
			clog << e->to_string() << ' ';
		}
	}
	clog << "\n";
}

GroundTerm VarTermGrounder::run() const {
	return GroundTerm(_value->get());
}

GroundTerm FuncTermGrounder::run() const {
	if(_verbosity > 2) {
		printorig();
	}
	bool calculable = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n]._isvarid) {
			calculable = false;
		} else {
			assert(groundsubterms[n]._domelement!=NULL);
			args[n] = groundsubterms[n]._domelement;
		}
	}
	if(calculable && _functable) { // All ground subterms are domain elements!
		const DomainElement* result = (*_functable)[args];
		if(result) {
			if(_verbosity > 2) {
				clog << "Result = " << *result << "\n";
			}
			return GroundTerm(result);
		}
	}
	// assert(isCPSymbol(_function->symbol())) && some of the ground subterms are CP terms.
	VarId varid = _termtranslator->translate(_function,groundsubterms);
	if(_verbosity > 2) {
		clog << "Result = " << _termtranslator->printTerm(varid, false) << "\n"; // TODO longnames
	}
	return GroundTerm(varid);
}

// Without CP support
//GroundTerm FuncTermGrounder::run() const {
//	if(_verbosity > 2) {
//		printorig();
//	}
//	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
//	ElementTuple args(_subtermgrounders.size());
//	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
//		groundsubterms[n] = _subtermgrounders[n]->run();
//		assert(not groundsubterms[n]._isvarid);
//		args[n] = groundsubterms[n]._domelement;
//	}
//	const DomainElement* result = (*_functable)[args];
//	if(_verbosity > 2) {
//		clog << "Result = " << *result << "\n";
//	}
//	return GroundTerm(result);
//}

GroundTerm SumTermGrounder::run() const {
	if(_verbosity > 2) {
		printorig();
	}
	// Run subtermgrounders
	const GroundTerm& left = _lefttermgrounder->run();
	const GroundTerm& right = _righttermgrounder->run();
	SortTable* leftdomain = _lefttermgrounder->domain();
	SortTable* rightdomain = _righttermgrounder->domain();

	// Compute domain for the sum term
	if(not _domain || not _domain->approxfinite()) {
		if(leftdomain && rightdomain && leftdomain->approxfinite() && rightdomain->approxfinite()) {
			int leftmin = leftdomain->first()->value()._int;
			int rightmin = rightdomain->first()->value()._int;
			int leftmax = leftdomain->last()->value()._int;
			int rightmax = rightdomain->last()->value()._int;
			_domain = new SortTable(new IntRangeInternalSortTable(leftmin+rightmin,leftmax+rightmax));
		} else {
			//TODO one of the domains is unknown or infinite...
			//TODO one case when left or right is a domain element!
			//assert(false);
			// Create domain
			//rightdomain = new SortTable(new EnumeratedInternalSortTable());
			//rightdomain->add(right._domelement);
			// Create domain
			//leftdomain = new SortTable(new EnumeratedInternalSortTable());
			//leftdomain->add(left._domelement);
		}
	}

	VarId varid;
	if(left._isvarid) {
		if(right._isvarid) {
			CPTerm* sumterm = new CPSumTerm(left._varid,right._varid);
			varid = _termtranslator->translate(sumterm,_domain);
		} else {
			assert(not right._isvarid);
			VarId rightvarid = _termtranslator->translate(right._domelement);
			// Create tseitin
			CPTsBody* cpelement = _termtranslator->cprelation(rightvarid);
			int tseitin = _grounding->translator()->translate(cpelement->left(),cpelement->comp(),cpelement->right(),TsType::EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			CPTerm* sumterm = new CPSumTerm(left._varid,rightvarid);
			varid = _termtranslator->translate(sumterm,_domain);
		}
	} else {
		assert(not left._isvarid);

		if(right._isvarid) {
			VarId leftvarid = _termtranslator->translate(left._domelement);
			// Create tseitin
			CPTsBody* cpelement = _termtranslator->cprelation(leftvarid);
			int tseitin = _grounding->translator()->translate(cpelement->left(),cpelement->comp(),cpelement->right(),TsType::EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			CPTerm* sumterm = new CPSumTerm(leftvarid,right._varid);
			varid = _termtranslator->translate(sumterm,_domain);
		} else { // Both subterms are domain elements, so lookup the result in the function table.
			assert(not right._isvarid);
			assert(_functable);
			ElementTuple args(2); args[0] = left._domelement; args[1] = right._domelement;
			const DomainElement* result = (*_functable)[args];
			assert(result);
			if(_verbosity > 2) {
				clog << "Result = " << *result << "\n";
			}
			return GroundTerm(result);
		}
	}

	// Ask for a new tseitin for this cp constraint and add it to the grounding.
	//FIXME Following should be done more efficiently... overhead because of lookup in translator...
	//CPTsBody* cprelation = _termtranslator->cprelation(varid);
	//int sumtseitin = _grounding->translator()->translate(cprelation->left(),cprelation->comp(),cprelation->right(),TS_EQ);
	//_grounding->addUnitClause(sumtseitin);

	// Return result
	if(_verbosity > 2) {
		clog << "Result = " << _termtranslator->printTerm(varid, false) << "\n"; // TODO longnames
	}
	return GroundTerm(varid);
}

GroundTerm AggTermGrounder::run() const {
//TODO Should this grounder return a VarId in some cases?
	int setnr = _setgrounder->run();
	const TsSet& tsset = _translator->groundset(setnr);
	assert(tsset.empty());
	double value = applyAgg(_type,tsset.trueweights());
	const DomainElement* result = DomainElementFactory::instance()->create(value);
	if(_verbosity > 2) {
		clog << "Result = " << *result << "\n";
	}
	return GroundTerm(result);
}
