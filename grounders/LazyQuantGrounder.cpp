/************************************
 LazyQuantGrounder.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "grounders/LazyQuantGrounder.hpp"
#include "generator.hpp"

using namespace std;

Lit LazyQuantGrounder::createTseitin() const{
	Lit tseitin = _translator->translate(this);
	return isNegative()?-tseitin:tseitin;
}

void LazyQuantGrounder::groundMore() const {
	if(_verbosity > 2) printorig();

	// add one more grounding to the formula => with correct sign depending on negateclause!
	// if value is decided, allow to erase the formula

	// TODO if we come here, "next" SHOULD already have been called AND been succesful (otherwise the formula was fully ground and we should not come here again), check this!

	Lit l = _subgrounder->run();
	if(decidesClause(l)) {
		Lit valuelit = getDecidedValue();
		valuelit = negatedclause_?-valuelit:valuelit;
		// TODO notify formula that it is certainly false/true
	}else if(isNotRedundantInClause(l)){
		l = negatedclause_ ? -l : l;
		// TODO add to current formula

	}
	if(not _generator->next()){
		// TODO notify fullyground
	}
}

void LazyQuantGrounder::run(litlist& clause, bool negateclause) const {
	if(_verbosity > 2) printorig();

	if(_generator->first()) {
		return;
	}

	groundMore();

	negatedclause_ = negateclause;

	clause.push_back(createTseitin());
}
