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

enum class InvarType {SingleStateInvar, BistateInvar};

struct SplitLTCInvariant {
	/**
	 * The kind of invariant this is. For a single-state invariant, this formula contains two constraints:
	 * * basestep, which contains a formula for proving the base step of the induction
	 * * inductionstep, which contains a formula for proving the induction step of the formula.
	 *
	 * For a bistate invariant, we should only prove one claim, this claim can be found in
	 * * bistateInvar
	 */
	InvarType invartype;


	/**
	 * Only relevant if this invariant is a single-state invariant
	 * Theory over vocabulary stateVoc. Contains:
	 * * one constraint: P(Start)
	 */
	Formula* baseStep;
	/**
	 * Only relevant if this invariant is a single-state invariant
	 * Theory over vocabulary biStateVoc. Contains:
	 * * one constraint: P(now) => P(next)
	 */
	Formula* inductionStep;


	/**
	 * Only relevant if this invariant is a bistate invariant
	 * Theory that contains the translation of the invariant to the bistatevocabulary
	 */
	Formula* bistateInvar;
};
