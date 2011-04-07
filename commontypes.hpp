/************************************
	commontypes.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COMMONTYPES_HPP
#define COMMONTYPES_HPP

/** Enumeration types **/
enum ElementType { ELINT, ELDOUBLE, ELSTRING, ELCOMPOUND };
enum AggType { AGGCARD, AGGSUM, AGGPROD, AGGMIN, AGGMAX };

/*
 * Enumeration for the possible ways to define a tseitin atom in terms of the subformula it replaces.
 *		TS_EQ:		tseitin <=> subformula
 *		TS_RULE:	tseitin <- subformula
 *		TS_IMPL:	tseitin => subformula
 *		TS_RIMPL:	tseitin <= subformula
 */
enum TsType { TS_EQ, TS_RULE, TS_IMPL, TS_RIMPL };

/** Domain element **/
struct compound;
typedef compound* domelement;

#endif
