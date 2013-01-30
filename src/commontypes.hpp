/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef COMMONTYPES_HPP
#define COMMONTYPES_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <queue>
#include "utils/NumericLimits.hpp"

/**
 *	\file This file contains some frequently used typedefs and enumerations
 */

typedef unsigned int Atom;
typedef int Lit;
typedef double Weight;
typedef std::vector<Lit> litlist;
typedef std::vector<Weight> weightlist;

struct VarId {
	unsigned int id;
};
struct DefId {
	unsigned int id;

	DefId() : id(0) {
	}
	DefId(unsigned int id) : id(id) {
	}
};
struct SetId {
	unsigned int id;

	SetId() : id(0) {
	}
	SetId(unsigned int id) : id(id) {
	}
};
typedef std::vector<VarId> varidlist;

class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;

enum class TruthValue {
	True, False, Unknown
};

/**
 * The different aggregate functions
 *	- AGGCARD:	card
 *	- AGGSUM:	sum
 *	- AGGPROD:	prod
 *	- AGGMIN:	min
 *	- AGGMAX:	max
 */
enum AggFunction {
	CARD,
	SUM,
	PROD,
	MIN,
	MAX
};

/**
 * Enumeration for the possible ways to define a tseitin atom in terms of the subformula it replaces.
 *		- TsType::EQ:		tseitin <=> subformula
 *		- TsType::RULE:		tseitin <- subformula
 *		- TsType::IMPL:		tseitin => subformula
 *		- TsType::RIMPL:	tseitin <= subformula
 */
enum TsType {
	EQ,
	RULE,
	IMPL,
	RIMPL
};

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
enum class CompType {
	EQ,
	NEQ,
	LT,
	GT,
	LEQ,
	GEQ
};

/** The context of a subformula */
enum class Context {
	POSITIVE,
	NEGATIVE,
	BOTH
};

/** The sign of a formula (NEG is a negation in front) */
enum class SIGN {
	NEG,
	POS
};

enum class QUANT {
	UNIV,
	EXIST
};

enum class TruthType {
	POSS_TRUE,
	POSS_FALSE,
	CERTAIN_TRUE,
	CERTAIN_FALSE
};

// NOTE: int and double domain elements are DISJOINT! It is never allowed to create a double domainelement if it in fact refers to an int, so have to check for this
enum class NumType {
	CERTAINLYINT,
	POSSIBLYINT
};

#endif
