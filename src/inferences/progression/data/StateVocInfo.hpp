#pragma once

#include <map>

class Vocabulary;
class Sort;
class PFSymbol;
class Function;

struct LTCVocInfo {
	/**
	 * A vocabulary containing no time-related stuff: every predicate/function is projected to leave out time-arguments
	 */
	Vocabulary* stateVoc;

	/**
	 * Similar to stateVoc, but all predicates/functions that are time-related, now contain two predicates:
	 * * One equals the predicate in statevoc representing "this timepoint"
	 * * One with equal name, but ending on _next, representing "next timepoint"
	 */
	Vocabulary* biStateVoc;

	const Sort* time;
	Function* start;
	Function* next;

	//Maps containing the transformations
	std::map<PFSymbol*, PFSymbol*> LTC2State;
	std::map<PFSymbol*, PFSymbol*> State2LTC;

	std::map<PFSymbol*, PFSymbol*> LTC2NextState;
	std::map<PFSymbol*, PFSymbol*> NextState2LTC;

	std::map<PFSymbol*, std::size_t> IndexOfTime;

};
