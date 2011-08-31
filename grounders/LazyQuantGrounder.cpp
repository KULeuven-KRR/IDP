/************************************
 LazyQuantGrounder.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "grounders/LazyQuantGrounder.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "generator.hpp"

using namespace std;

unsigned int LazyQuantGrounder::maxid = 1;

Lit LazyQuantGrounder::createTseitin() const{
	Lit tseitin = _translator->translate(this, _context._tseitin);
	return isNegative()?-tseitin:tseitin;
}

void LazyQuantGrounder::requestGroundMore(const Lit& tseitin) {
	notifyTheoryOccurence(tseitin);
}

void LazyQuantGrounder::groundMore() const{
	if(_verbosity > 2) printorig();

	// add one more grounding to the formula => with correct sign depending on negateclause!
	// if value is decided, allow to erase the formula

	// TODO check that if we come here, "next" SHOULD already have been called AND been succesful (otherwise the formula was fully ground and we should not come here again), check this!

	grounding = true;

	while(queuedtseitinstoground.size()>0){
		Lit oldtseitin = queuedtseitinstoground.front();
		queuedtseitinstoground.pop();
		Lit lit = _subgrounder->run();
		if(decidesClause(lit)) {
			lit = getDecidedValue();
			lit = negatedclause_?-lit:lit;
		}else if(isNotRedundantInClause(lit)){
			lit = negatedclause_ ? -lit : lit;
		}
		GroundClause clause;
		clause.push_back(lit);

		// FIXME notify lazy should check whether the tseitin already has a value and request more grounding immediately!
		if(_generator->next()){
			Lit newtseitin = _translator->nextNumber();
			clause.push_back(newtseitin);
			groundtheory_->notifyLazyResidual(newtseitin, this); // set on not-decide and add to watchlist
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

	clause.push_back(createTseitin());
}

void LazyQuantGrounder::notifyTheoryOccurence(const Lit& tseitin) const{
	// restructured code to prevent recursion // FIXME duplication and const issues!
	queuedtseitinstoground.push(tseitin);
	if(not grounding){
		groundMore();
	}
}
