/************************************
	builtin.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BUILTIN_H
#define BUILTIN_H

#include "vocabulary.hpp"
#include "structure.hpp"

/* 
 * The standard builtin vocabulary
 *		sorts:			nat, int, float, char, string
 *		predicates:		'=', '<', '>', SUCC, 
 *		functions:		+, -, *, /, ^, %, abs, -/1, MIN, MAX
 */
class StdBuiltin : public Vocabulary {
	public:
		// Constructor
		StdBuiltin();
};

#endif
