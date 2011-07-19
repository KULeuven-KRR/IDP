/************************************
	commontypes.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COMMONTYPES_HPP
#define COMMONTYPES_HPP

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
enum AggFunction { AGG_CARD, AGG_SUM, AGG_PROD, AGG_MIN, AGG_MAX };

/**
 * Enumeration for the possible ways to define a tseitin atom in terms of the subformula it replaces.
 *		- TS_EQ:		tseitin <=> subformula
 *		- TS_RULE:	tseitin <- subformula
 *		- TS_IMPL:	tseitin => subformula
 *		- TS_RIMPL:	tseitin <= subformula
 */
enum TsType { TS_EQ, TS_RULE, TS_IMPL, TS_RIMPL };

/**
 * The different comparison operators
 *	- CT_EQ:	=
 *	- CT_NEQ:	~=
 *	- CT_LT:	<
 *	- CT_GT:	>
 *	- CT_LEQ:	=<
 *	- CT_GEQ:	>=
 */
enum CompType { CT_EQ, CT_NEQ, CT_LT, CT_GT, CT_LEQ, CT_GEQ };

/**
 * The context of a subformula
 */
enum PosContext { PC_POSITIVE, PC_NEGATIVE, PC_BOTH };

#endif
