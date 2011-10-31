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
enum AggFunction { CARD, SUM, PROD, MIN, MAX };

/**
 * Enumeration for the possible ways to define a tseitin atom in terms of the subformula it replaces.
 *		- TsType::EQ:		tseitin <=> subformula
 *		- TsType::RULE:		tseitin <- subformula
 *		- TsType::IMPL:		tseitin => subformula
 *		- TsType::RIMPL:	tseitin <= subformula
 */
enum TsType { EQ, RULE, IMPL, RIMPL };

// TODO when better gcc compiler, add enum class here again (currently lacking default operators)

/**
 * The different comparison operators
 *	- CompType::EQ:		=
 *	- CompType::NEQ:	~=
 *	- CompType::LT:		<
 *	- CompType::GT:		>
 *	- CompType::LEQ:	=<
 *	- CompType::GEQ:	>=
 */
enum class CompType { EQ, NEQ, LT, GT, LEQ, GEQ};

/**
 * The context of a subformula
 */
enum class Context { POSITIVE, NEGATIVE, BOTH };

// The sign of a formula (NEG is a negation in front)
enum class SIGN{ NEG, POS};

enum class QUANT { UNIV, EXIST};

enum class TruthType { CT, CF, PT, PF };

enum class NumType { DOUBLE, INT };

typedef int Lit;
typedef std::vector<Lit> litlist;

// TODO dynamically initialized global: no init order, so cannot safely use in other globals, might be unsafe?
const Lit _true(std::numeric_limits<int>::max());
const Lit _false(0);

#endif
