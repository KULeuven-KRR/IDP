#include "LazyInst.hpp"
#include "LazyDisjunctiveGrounders.hpp"

void LazyInstantiation::notifyTheoryOccurrence(TsType type){
	grounder->notifyTheoryOccurrence(this, type);
}

void LazyInstantiation::notifyGroundingRequested(int ID, bool groundall, bool& stilldelayed){
	grounder->notifyGroundingRequested(ID, groundall, this, stilldelayed);
}
