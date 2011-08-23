/************************************
	commontypes.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COMMONTYPES_HPP
#define COMMONTYPES_HPP

#include <limits>
#include <vector>

/**
 *	\file This file contains some frequently used typedefs and enumerations
 */

/**
 * The different aggregate functions
 *	- AGGCARD:	card
 *	- AGGSUM:	sum
 *	- AGGPROD:	prod
 *	- AGGMIN:	min
 *	- AGGMAX:	max
 */
enum class AggFunction { CARD, SUM, PROD, MIN, MAX };

/**
 * Enumeration for the possible ways to define a tseitin atom in terms of the subformula it replaces.
 *		- TsType::EQ:		tseitin <=> subformula
 *		- TsType::RULE:	tseitin <- subformula
 *		- TsType::IMPL:	tseitin => subformula
 *		- TsType::RIMPL:	tseitin <= subformula
 */
enum class TsType { EQ, RULE, IMPL, RIMPL };

/**
 * The different comparison operators
 *	- CompType::EQ:	=
 *	- CompType::NEQ:	~=
 *	- CompType::LT:	<
 *	- CompType::GT:	>
 *	- CompType::LEQ:	=<
 *	- CompType::GEQ:	>=
 */
enum class CompType { EQ, NEQ, LT, GT, LEQ, GEQ };

/**
 * The context of a subformula
 */
enum class Context { POSITIVE, NEGATIVE, BOTH };

// The sign of a formula (NEG is a negation in front)
enum class SIGN{ NEG, POS};

enum class QUANT { UNIV, EXIST};

typedef int Lit;
typedef std::vector<Lit> litlist;

const Lit _true(std::numeric_limits<int>::max());
const Lit _false(0);

#endif
