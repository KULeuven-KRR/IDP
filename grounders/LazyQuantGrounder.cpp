/************************************
 LazyQuantGrounder.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "grounders/LazyQuantGrounder.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "generator.hpp"
#include "grounders/GroundUtils.hpp"

#include <iostream>

using namespace std;

unsigned int LazyQuantGrounder::maxid = 1;

void LazyQuantGrounder::requestGroundMore(ResidualAndFreeInst * instance) {
	notifyTheoryOccurence(instance);
}

void LazyQuantGrounder::groundMore() const{
	if(_verbosity > 2) printorig();

	// add one more grounding to the formula => with correct sign depending on negateclause!
	// if value is decided, allow to erase the formula

	// TODO check that if we come here, "next" SHOULD already have been called AND been succesful (otherwise the formula was fully ground and we should not come here again), check this!

	grounding = true;

	while(queuedtseitinstoground.size()>0){
		ResidualAndFreeInst* instance = queuedtseitinstoground.front();
		queuedtseitinstoground.pop();

		vector<const DomainElement*> originstantiation;
		overwriteVars(originstantiation, instance->freevarinst);
		Lit lit = _subgrounder->run();
		restoreOrigVars(originstantiation, instance->freevarinst);

		if(decidesClause(lit)) {
			lit = getDecidedValue();
			lit = negatedclause_?-lit:lit;
		}else if(isNotRedundantInClause(lit)){
			lit = negatedclause_ ? -lit : lit;
		}
		GroundClause clause;
		clause.push_back(lit);

		Lit oldtseitin = instance->residual;
		// FIXME notify lazy should check whether the tseitin already has a value and request more grounding immediately!
		if(_generator->next()){
			Lit newtseitin = _translator->nextNumber();
			clause.push_back(newtseitin);
			instance->residual = newtseitin;
			groundtheory_->notifyLazyResidual(instance, this); // set on not-decide and add to watchlist
		}
		groundtheory_->add(oldtseitin, _context._tseitin, clause);
	}

	grounding = false;
}

void LazyQuantGrounder::run(litlist& clause, bool negateclause) const {
	if(_verbosity > 2) printorig();

	negatedclause_ = negateclause;

	if(not _generator->first()) {
		return;
	}

	// TODO waar allemaal rekening houden met welke signs en contexten?

	ResidualAndFreeInst* inst = new ResidualAndFreeInst();

	clog <<"known free vars: \n\t";
	printorig();
	clog <<"The provided free vars: \n\t";
	for(auto var=freevars.begin(); var!=freevars.end(); ++var){
		clog <<(*var)->to_string() <<", ";
	}
	clog <<"\n\n\n";

	for(auto var=freevars.begin(); var!=freevars.end(); ++var){
		auto tuple = _realvarmap.at(*var);
		inst->freevarinst.push_back(dominst(tuple, *tuple));
	}

	_translator->translate(this, inst, _context._tseitin);
	if(isNegative()){
		inst->residual = -inst->residual;
	}
	clause.push_back(inst->residual);
}

void LazyQuantGrounder::notifyTheoryOccurence(ResidualAndFreeInst * instance) const{
	// restructured code to prevent recursion // FIXME duplication and const issues!
	queuedtseitinstoground.push(instance);
	if(not grounding){
		groundMore();
	}
}
