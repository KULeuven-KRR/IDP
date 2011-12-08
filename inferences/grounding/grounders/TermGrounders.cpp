#include "TermGrounders.hpp"

#include "SetGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"

#include "common.hpp"
#include "vocabulary.hpp"
#include "term.hpp"

#include <iostream>

using namespace std;

void TermGrounder::setOrig(const Term* t, const map<Variable*,const DomElemContainer*>& mvd, int verb) {
	_verbosity = verb;
	map<Variable*,Variable*> mvv;
	for(auto it = t->freeVars().cbegin(); it != t->freeVars().cend(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		_varmap[v] = mvd.find(*it)->second;
	}
	_origterm = t->clone(mvv);
}

void TermGrounder::printOrig() const {
	clog << "Grounding term " << toString(_origterm);
	if(not _origterm->freeVars().empty()) {
		clog << " with instance ";
		for(auto it = _origterm->freeVars().cbegin(); it != _origterm->freeVars().cend(); ++it) {
			clog << toString(*it) << " = ";
			const DomainElement* e = _varmap.find(*it)->second->get();
			clog << toString(e) << ' ';
		}
	}
	clog << "\n";
}

GroundTerm DomTermGrounder::run() const {
	//SortTable* _domain = new SortTable(new EnumeratedInternalSortTable());
	//_domain->add(_value);
	return GroundTerm(_value);
}

GroundTerm VarTermGrounder::run() const {
	return GroundTerm(_value->get());
}

GroundTerm FuncTermGrounder::run() const {
	if(_verbosity > 2) {
		printOrig();
	}
	bool calculable = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		if(GlobalData::instance()->terminateRequested()){
			throw IdpException("Terminate requested");
		}
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n].isVariable) {
			calculable = false;
		} else {
			Assert(groundsubterms[n]._domelement!=NULL);
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
	// Assert(isCPSymbol(_function->symbol())) && some of the ground subterms are CP terms.
	VarId varid = _termtranslator->translate(_function,groundsubterms);
	if(_verbosity > 2) {
		clog << "Result = " << _termtranslator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

CPTerm* createSumTerm(SumType type, const VarId& left, const VarId& right) {
	if(type == ST_MINUS) {
		vector<VarId> varids(2); //XXX vector<VarId> varids[] = { left, right }; ?
		varids[0] = left;
		varids[1] = right;
		vector<int> weights(2);
		weights[0] = 1;
		weights[1] = -1;
		return new CPWSumTerm(varids,weights);
	} else {
		return new CPSumTerm(left,right);
	}
}

GroundTerm SumTermGrounder::run() const {
	if(_verbosity > 2) {
		printOrig();
	}
	// Run subtermgrounders
	const GroundTerm& left = _lefttermgrounder->run();
	const GroundTerm& right = _righttermgrounder->run();
	SortTable* leftdomain = _lefttermgrounder->getDomain();
	SortTable* rightdomain = _righttermgrounder->getDomain();

	// Compute domain for the sum term
	if(not _domain || not _domain->approxFinite()) {
		if(not left.isVariable) {
			leftdomain = new SortTable(new EnumeratedInternalSortTable());
			leftdomain->add(left._domelement);
		}
		if(not right.isVariable) {
			rightdomain = new SortTable(new EnumeratedInternalSortTable());
			rightdomain->add(right._domelement);
		}
		if(leftdomain && rightdomain && leftdomain->approxFinite() && rightdomain->approxFinite()) {
			int leftmin = leftdomain->first()->value()._int;
			int rightmin = rightdomain->first()->value()._int;
			int leftmax = leftdomain->last()->value()._int;
			int rightmax = rightdomain->last()->value()._int;
			int min, max; 
			if(_type == ST_PLUS) {
				min = leftmin + rightmin;
				max = leftmax + rightmax;
			} else if (_type == ST_MINUS) {
				min = leftmin - rightmin;
				max = leftmax - rightmax;
			}
			if(max < min) { swap(min,max); }
			_domain = new SortTable(new IntRangeInternalSortTable(min,max));
		} else {
			if(leftdomain && not leftdomain->approxFinite()) {
				clog << "Left domain is infinite..." << endl;
			}
			if(rightdomain && not rightdomain->approxFinite()) {
				clog << "Right domain is infinite..." << endl;
			}
			//TODO one of the domains is unknown or infinite...
			//TODO one case when left or right is a domain element!
			thrownotyetimplemented("One of the domains in a sumtermgrounder is infinite.");
		}
	}

	VarId varid;
	if(left.isVariable) {
		if(right.isVariable) {
			CPTerm* sumterm = createSumTerm(_type,left._varid,right._varid);
			varid = _termtranslator->translate(sumterm,_domain);
		} else {
			Assert(not right.isVariable);
			VarId rightvarid = _termtranslator->translate(right._domelement);
			// Create tseitin
			CPTsBody* cpelement = _termtranslator->cprelation(rightvarid);
			Lit tseitin = _grounding->translator()->translate(cpelement->left(),cpelement->comp(),cpelement->right(),TsType::EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			CPTerm* sumterm = createSumTerm(_type,left._varid,rightvarid);
			varid = _termtranslator->translate(sumterm,_domain);
		}
	} else {
		Assert(not left.isVariable);

		if(right.isVariable) {
			VarId leftvarid = _termtranslator->translate(left._domelement);
			// Create tseitin
			CPTsBody* cpelement = _termtranslator->cprelation(leftvarid);
			int tseitin = _grounding->translator()->translate(cpelement->left(),cpelement->comp(),cpelement->right(),TsType::EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			CPTerm* sumterm = createSumTerm(_type,leftvarid,right._varid);
			varid = _termtranslator->translate(sumterm,_domain);
		} else { // Both subterms are domain elements, so lookup the result in the function table.
			Assert(not right.isVariable);
			Assert(_functable);
			ElementTuple args(2); args[0] = left._domelement; args[1] = right._domelement;
			const DomainElement* result = (*_functable)[args];
			Assert(result);
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
		clog << "Result = " << _termtranslator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

GroundTerm AggTermGrounder::run() const {
//TODO Should this grounder return a VarId in some cases?
	if(_verbosity > 2) {
		printOrig();
	}
	int setnr = _setgrounder->run();
	const TsSet& tsset = _translator->groundset(setnr);
	Assert(not tsset.empty());
	double value = applyAgg(_type,tsset.trueweights());
	const DomainElement* result = createDomElem(value);
	if(_verbosity > 2) {
		clog << "Result = " << *result << "\n";
	}
	// FIXME if grounding aggregates, with an upper and lower bound, should not return a domelem from the subgrounder, but a vardomain or something?
	return GroundTerm(result);
}
