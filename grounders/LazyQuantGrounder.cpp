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

bool LazyQuantGrounder::requestGroundMore() {
	return groundMore();
}

bool LazyQuantGrounder::groundMore() const{
	if(_verbosity > 2) printorig();

	// add one more grounding to the formula => with correct sign depending on negateclause!
	// if value is decided, allow to erase the formula

	// TODO check that if we come here, "next" SHOULD already have been called AND been succesful (otherwise the formula was fully ground and we should not come here again), check this!

	Lit l = _subgrounder->run();
	if(decidesClause(l)) {
		Lit valuelit = getDecidedValue();
		valuelit = negatedclause_?-valuelit:valuelit;
		groundtheory_->notifyLazyClauseHasValue(valuelit, id());
	}else if(isNotRedundantInClause(l)){
		l = negatedclause_ ? -l : l;
		groundtheory_->addLitToLazyClause(l, id());
	}
	bool fullyground = not _generator->next();
	return fullyground;
}

void LazyQuantGrounder::notifyTheoryOccurence() const{
	groundtheory_->polAdd(tseitin, firstlit, id(), const_cast<LazyQuantGrounder*>(this));
}

void LazyQuantGrounder::run(litlist& clause, bool negateclause) const {
	if(_verbosity > 2) printorig();

	negatedclause_ = negateclause;

	if(not _generator->first()) {
		return;
	}

	firstlit = _subgrounder->run();
	if(decidesClause(firstlit)) {
		firstlit = getDecidedValue();
		firstlit = negatedclause_?-firstlit:firstlit;
		clause.push_back(firstlit);
	}else if(not _generator->next()){
		firstlit = negatedclause_ ? -firstlit : firstlit;
		clause.push_back(firstlit);
	}else{
		tseitin = createTseitin();
		clause.push_back(tseitin);
	}
}
