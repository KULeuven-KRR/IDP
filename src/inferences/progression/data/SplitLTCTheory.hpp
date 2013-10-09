#pragma once

#include <string>

class Theory;
class Vocabulary;

struct SplitLTCTheory {
	/**
	 * Theory over vocabulary biStateVoc. Contains:
	 * * All static axioms
	 * * All bistate axioms, replacing predicates with t by their _0 variant and with t+1 by their _1 variant
	 * * All single-state axioms with _1
	 */
	Theory* bistateTheory;
	/**
	 * Theory over vocabulary stateVoc. Contains:
	 * * All axioms containing start
	 * * All static axioms
	 * * All single-state axioms instatiated for start
	 */
	Theory* initialTheory;
};
